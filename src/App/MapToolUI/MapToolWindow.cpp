/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapToolWindow.hpp"

#include "ui_MapToolWindow.h"

#include "MapEditorWidget.hpp"
#include "MapConverter.hpp"
#include "FHMapReflection.hpp"

#include "EnvDetect.hpp"
#include "IRandomGenerator.hpp"
#include "LibraryModels.hpp"
#include "LibraryPlayer.hpp"

#include "GameDatabasePropertyReader.hpp"
#include "GameDatabasePropertyWriter.hpp"

#include "MernelPlatform/AppLocations.hpp"
#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"

#include "GitCommit.hpp"

#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QValidator>

namespace FreeHeroes {

namespace {
const QLatin1String g_reg{ "HKEY_CURRENT_USER\\SOFTWARE\\FreeHeroes" };

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
    , m_userSettings(std::make_unique<FHRngUserSettings>())
{
    m_version = "1.0.3";

    {
        auto buildDate = QLocale("en_US").toDate(QString(__DATE__).simplified(), "MMM d yyyy");
        auto buildTime = QLocale("en_US").toTime(QString(__TIME__).simplified(), "HH:mm:ss");
        m_buildId      = buildDate.toString("yyyyddMM") + ".";
        m_buildId += buildTime.toString("HHmmss") + ".";
        m_buildId += g_gitCommit;
    }

    m_ui->setupUi(this, std::tuple{ modelsProvider });
    m_ui->tabWidget->setCurrentIndex(0);

    m_ui->labelVersion->setText(m_ui->labelVersion->text() + m_version);
    m_ui->labelBuildId->setText(m_ui->labelBuildId->text() + m_buildId);

    m_editor = new MapEditorWidget(
        m_gameDatabaseContainer,
        m_rngFactory,
        m_graphicsLibrary,
        m_modelsProvider,
        this);

    for (int i = 0; i <= 6; i++)
        m_ui->comboBoxMapSize->setItemData(i, 36 * (i + 1));

    loadUserSettings();

    connect(m_ui->pushButtonEditMap, &QAbstractButton::clicked, this, [this] {
        QString fhMap = m_ui->fhMapPath->text();
        m_editor->load(fhMap.toStdString());
        m_editor->show();
    });

    Mernel::AppLocations loc("FreeHeroes");

    QSettings settings(g_reg, QSettings::NativeFormat);
    m_ui->heroes3Path->setText(settings.value("heroes3Path").toString());
    m_ui->mapFilename->setText(settings.value("mapFilename").toString());
    m_ui->mapGamename->setText(settings.value("mapGamename").toString());
    m_ui->lineEditSeed->setText(settings.value("seed").toString());
    m_ui->checkBoxIncrementSeed->setChecked(settings.value("checkBoxIncrementSeed").toBool());
    m_ui->checkBoxGenerateSeed->setChecked(settings.value("checkBoxGenerateSeed", QVariant(true)).toBool());

    Mernel::std_path hotaPath = findHeroes3Installation();

    if (m_ui->heroes3Path->text().isEmpty() && !hotaPath.empty())
        m_ui->heroes3Path->setText(QString::fromStdString(Mernel::path2string(hotaPath)));

    if (m_ui->mapFilename->text().isEmpty())
        m_ui->mapFilename->setText("FreeHeroes_generated");

    if (m_ui->mapGamename->text().isEmpty())
        m_ui->mapGamename->setText("FreeHeroes Generated");

    {
        for (auto&& it : Mernel::std_fs::directory_iterator(loc.getBinDir() / "gameResources" / "templates")) {
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

    connect(m_ui->pushButtonEditH3, &QAbstractButton::clicked, this, [this] {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open HotA installation root"));
        if (dir.isEmpty())
            return;
        m_ui->heroes3Path->setText(dir);
        updatePaths();
    });

    connect(m_ui->mapFilename, &QLineEdit::textChanged, this, [this] {
        updatePaths();
    });

    connect(m_ui->pushButtonGenerate, &QAbstractButton::clicked, this, &MapToolWindow::generateMap);

    if (m_ui->lineEditSeed->text().isEmpty())
        m_ui->pushButtonNewSeed->click();

    m_ui->mapFilename->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9_ -]+"), this));
    m_ui->mapGamename->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9_ -]+"), this));

    updatePaths();

    setWindowTitle(tr("Random map generator") + " - v. " + m_version);
    setWindowIcon(QIcon(":/Application/Logo/64_map.png"));
}

MapToolWindow::~MapToolWindow()
{
    QSettings settings(g_reg, QSettings::NativeFormat);
    settings.setValue("heroes3Path", m_ui->heroes3Path->text());
    settings.setValue("mapFilename", m_ui->mapFilename->text());
    settings.setValue("mapGamename", m_ui->mapGamename->text());
    settings.setValue("seed", m_ui->lineEditSeed->text());

    settings.setValue("checkBoxIncrementSeed", m_ui->checkBoxIncrementSeed->isChecked());
    settings.setValue("checkBoxGenerateSeed", m_ui->checkBoxGenerateSeed->isChecked());

    saveUserSettings();
}

void MapToolWindow::generateMap()
{
    if (m_ui->checkBoxGenerateSeed->isChecked())
        m_ui->pushButtonNewSeed->click();

    if (!saveUserSettings()) {
        m_ui->labelStatus->setText(tr("Failed to save user settings"));
        return;
    }

    m_ui->labelStatus->setText(tr("Generation..."));
    m_ui->pushButtonGenerate->setEnabled(false);

    m_ui->pushButtonGenerate->repaint();
    m_ui->labelStatus->repaint();

    const std::string fhTpl  = m_ui->comboBoxTemplateSelect->currentData().toString().toStdString();
    const std::string fhMap  = m_ui->fhMapPath->text().toStdString();
    const std::string h3mMap = m_ui->h3mMapPath->text().toStdString();

    bool               result = true;
    std::ostringstream os;

    uint64_t   seed     = m_ui->lineEditSeed->text().toULongLong();
    const auto gamename = m_ui->mapGamename->text().toStdString();

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
        if (!gamename.empty()) {
            converter.m_mapFH.m_name  = gamename + " - " + converter.m_mapFH.m_name;
            converter.m_mapFH.m_descr = gamename + " - " + converter.m_mapFH.m_descr + "\n";
            if (m_ui->checkBoxShowSeed->isChecked())
                converter.m_mapFH.m_descr += "\nseed:" + std::to_string(seed);
            converter.m_mapFH.m_descr += "\ngeneratorVersion:" + m_version.toStdString();
            converter.m_mapFH.m_descr += "\nbuildId:" + m_buildId.toStdString();
        }

        converter.run(MapConverter::Task::SaveH3M);
    }
    catch (std::exception& ex) {
        os << ex.what() << "\n";
        QMessageBox::warning(this, tr("Error occured"), tr("Error occured during template generation. \nCheck text area with log at 'extra' tab and contact developers."));
        result = false;
    }
    m_ui->textEditLogOutput->setPlainText(QString::fromStdString(os.str()));

    {
        std::ostringstream osDiag;
        osDiag << "version: " << m_version.toStdString() << "\n";
        osDiag << "buildId: " << m_buildId.toStdString() << "\n";
        osDiag << "seed: " << seed << "\n";
        osDiag << "template: " << fhTpl << "\n";
        osDiag << "userSettings:\n";
        osDiag << m_userSettingsData << "\n";

        m_ui->textEditDiagInfo->setPlainText(QString::fromStdString(osDiag.str()));
    }

    QTime t = QTime::currentTime();

    m_ui->labelStatus->setText((result ? tr("Success") : tr("Error!")) + " - " + t.toString("mm:ss.zzz"));

    m_ui->pushButtonGenerate->setEnabled(true);
}

void MapToolWindow::updatePaths()
{
    const QString mapFileName = m_ui->mapFilename->text();

    QString    root     = m_ui->heroes3Path->text();
    const auto jsonPath = (Mernel::string2path(root.toStdString()) / "Maps" / (mapFileName.toStdString() + ".json"));
    if (!root.isEmpty())
        m_ui->h3mMapPath->setText(root + "/Maps/" + mapFileName + ".h3m");
    m_ui->fhMapPath->setText(QString::fromStdString(Mernel::path2string(jsonPath)));
}

void MapToolWindow::loadUserSettings()
{
    const Mernel::std_path path = getUserSettingsPath();
    Mernel::PropertyTree   jsonData;
    if (Mernel::std_fs::exists(path)) {
        std::string buffer = Mernel::readFileIntoBuffer(path);
        jsonData           = Mernel::readJsonFromBuffer(buffer);
    }

    Core::PropertyTreeReaderDatabase reader(m_modelsProvider->database());
    if (jsonData.isMap())
        reader.jsonToValue(jsonData, *m_userSettings);

    m_ui->templateSettingsWidget->setUserSettings(m_userSettings.get());
    m_ui->templateSettingsWidget->updateUI();

    m_ui->tabDifficultySettingsWidget->setUserSettings(m_userSettings.get());
    m_ui->tabDifficultySettingsWidget->updateUI();

    for (int i = 0; i < m_ui->comboBoxMapSize->count(); ++i) {
        if (m_ui->comboBoxMapSize->itemData(i).value<int>() >= m_userSettings->m_mapSize) {
            m_ui->comboBoxMapSize->setCurrentIndex(i);
            break;
        }
    }

    m_ui->comboBoxRoadsMain->setCurrentIndex(static_cast<int>(m_userSettings->m_defaultRoad));
    m_ui->comboBoxRoadsInner->setCurrentIndex(static_cast<int>(m_userSettings->m_innerRoad));
    m_ui->comboBoxRoadsBorder->setCurrentIndex(static_cast<int>(m_userSettings->m_borderRoad));
    m_ui->checkBoxUnderground->setChecked(m_userSettings->m_hasUnderground);
}

bool MapToolWindow::saveUserSettings()
{
    const Mernel::std_path path = getUserSettingsPath();

    try {
        Mernel::PropertyTree jsonData;
        m_ui->templateSettingsWidget->save();
        m_ui->tabDifficultySettingsWidget->save();
        m_userSettings->m_mapSize        = m_ui->comboBoxMapSize->currentData().toInt();
        m_userSettings->m_defaultRoad    = static_cast<FHRoadType>(m_ui->comboBoxRoadsMain->currentIndex());
        m_userSettings->m_innerRoad      = static_cast<FHRoadType>(m_ui->comboBoxRoadsInner->currentIndex());
        m_userSettings->m_borderRoad     = static_cast<FHRoadType>(m_ui->comboBoxRoadsBorder->currentIndex());
        m_userSettings->m_hasUnderground = m_ui->checkBoxUnderground->isChecked();

        Core::PropertyTreeWriterDatabase writer;
        writer.valueToJson(*m_userSettings, jsonData);

        m_userSettingsData = Mernel::writeJsonToBuffer(jsonData, true);

        Mernel::writeFileFromBuffer(path, m_userSettingsData);
    }
    catch (...) {
        return false;
    }

    return true;
}

}
