/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapToolWindow.hpp"

#include "ui_MapToolWindow.h"

#include "MapEditorWidget.hpp"
#include "MapConverter.hpp"

#include "EnvDetect.hpp"
#include "IRandomGenerator.hpp"

#include "MernelPlatform/AppLocations.hpp"

#include <QSettings>
#include <QMessageBox>
#include <QDateTime>

namespace FreeHeroes {

namespace {
const QString g_reg = "HKEY_CURRENT_USER\\SOFTWARE\\FreeHeroes";

Mernel::std_path getUserSettingsPath()
{
    Mernel::AppLocations loc("FreeHeroes");
    return loc.getAppdataDir() / "rngUserSettings.json";
}

}
MapToolWindow::MapToolWindow(
    const Core::IGameDatabaseContainer*  gameDatabaseContainer,
    const Core::IRandomGeneratorFactory* rngFactory,
    const Gui::IGraphicsLibrary*         graphicsLibrary,
    const Gui::LibraryModelsProvider*    modelsProvider,

    QWidget* parent)
    : QFrame(parent)
    , m_ui(std::make_unique<Ui::MapToolWindow>())
    , m_gameDatabaseContainer(gameDatabaseContainer)
    , m_rngFactory(rngFactory)
    , m_graphicsLibrary(graphicsLibrary)
    , m_modelsProvider(modelsProvider)
{
    m_ui->setupUi(this, std::tuple{ modelsProvider });
    m_editor = new MapEditorWidget(
        m_gameDatabaseContainer,
        m_rngFactory,
        m_graphicsLibrary,
        m_modelsProvider,
        this);

    m_ui->templateSettingsWidget->load(getUserSettingsPath());

    connect(m_ui->pushButtonEditMap, &QAbstractButton::clicked, this, [this] {
        QString fhMap = m_ui->fhMapPath->text();
        m_editor->load(fhMap.toStdString());
        m_editor->show();
    });

    Mernel::AppLocations loc("FreeHeroes");

    QSettings settings(g_reg, QSettings::NativeFormat);
    m_ui->fhMapPath->setText(settings.value("fhMapPath").toString());
    m_ui->h3mMapPath->setText(settings.value("h3mMapPath").toString());
    m_ui->lineEditSeed->setText(settings.value("seed").toString());
    m_ui->checkBoxIncrementSeed->setChecked(settings.value("checkBoxIncrementSeed").toBool());
    m_ui->checkBoxGenerateSeed->setChecked(settings.value("checkBoxGenerateSeed", QVariant(true)).toBool());

    Mernel::std_path hotaPath = findHeroes3Installation();

    if (m_ui->h3mMapPath->text().isEmpty() && !hotaPath.empty()) {
        auto out = hotaPath / "Maps" / "FreeHeroes_output.h3m";
        m_ui->h3mMapPath->setText(QString::fromStdString(Mernel::path2string(out)));
    }
    if (m_ui->fhMapPath->text().isEmpty() && !hotaPath.empty()) {
        auto out = hotaPath / "Maps" / "FreeHeroes_output.json";
        m_ui->fhMapPath->setText(QString::fromStdString(Mernel::path2string(out)));
    }
    {
        for (auto it : Mernel::std_fs::directory_iterator(loc.getBinDir() / "gameResources" / "templates")) {
            if (it.is_regular_file()) {
                auto path = it.path();
                m_ui->comboBoxTemplateSelect->addItem(QString::fromStdString(Mernel::path2string(path.stem())), QString::fromStdString(Mernel::path2string(path)));
            }
        }
    }

    connect(m_ui->pushButtonNewSeed, &QAbstractButton::clicked, this, [this] {
        uint64_t seed = 0;
        if (m_ui->checkBoxIncrementSeed->isChecked()) {
            seed = m_ui->lineEditSeed->text().toULongLong();
            seed++;
        } else {
            auto rng = m_rngFactory->create();
            rng->makeGoodSeed();
            seed = rng->getSeed();
        }

        m_ui->lineEditSeed->setText(QString::number(seed));
    });

    connect(m_ui->pushButtonGenerate, &QAbstractButton::clicked, this, &MapToolWindow::generateMap);

    if (m_ui->lineEditSeed->text().isEmpty())
        m_ui->pushButtonNewSeed->click();
}

void MapToolWindow::generateMap()
{
    if (m_ui->checkBoxGenerateSeed->isChecked())
        m_ui->pushButtonNewSeed->click();

    m_ui->labelStatus->setText(tr("Generation..."));

    const std::string fhTpl  = m_ui->comboBoxTemplateSelect->currentData().toString().toStdString();
    const std::string fhMap  = m_ui->fhMapPath->text().toStdString();
    const std::string h3mMap = m_ui->h3mMapPath->text().toStdString();

    bool               result = true;
    std::ostringstream os;

    uint64_t seed = m_ui->lineEditSeed->text().toULongLong();

    MapConverter::Settings sett{
        .m_inputs                  = { .m_fhTemplate = Mernel::string2path(fhTpl) },
        .m_outputs                 = { .m_fhMap = Mernel::string2path(fhMap), .m_h3m = { .m_binary = Mernel::string2path(h3mMap) } },
        .m_dumpUncompressedBuffers = false,
        .m_dumpBinaryDataJson      = false,
        .m_seed                    = seed,
        .m_rngUserSettings         = getUserSettingsPath(),
    };

    try {
        MapConverter converter(os,
                               m_gameDatabaseContainer,
                               m_rngFactory,
                               sett);

        converter.run(MapConverter::Task::FHTplToFHMap);
        converter.run(MapConverter::Task::SaveH3M);
    }
    catch (std::exception& ex) {
        os << ex.what() << "\n";
        QMessageBox::warning(this, tr("Error occured"), tr("Error occured during template generation. \nCheck text area with log at 'extra' tab and contact developers."));
        result = false;
    }
    m_ui->textEditLogOutput->setPlainText(QString::fromStdString(os.str()));

    QTime t = QTime::currentTime();

    m_ui->labelStatus->setText((result ? tr("Success") : tr("Error!")) + " - " + t.toString("mm:ss.zzz"));

    m_ui->pushButtonGenerate->setEnabled(true);
}

MapToolWindow::~MapToolWindow()
{
    QSettings settings(g_reg, QSettings::NativeFormat);
    settings.setValue("fhMapPath", m_ui->fhMapPath->text());
    settings.setValue("h3mMapPath", m_ui->h3mMapPath->text());
    settings.setValue("seed", m_ui->lineEditSeed->text());

    settings.setValue("checkBoxIncrementSeed", m_ui->checkBoxIncrementSeed->isChecked());
    settings.setValue("checkBoxGenerateSeed", m_ui->checkBoxGenerateSeed->isChecked());
}

}
