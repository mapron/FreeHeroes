/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplateSettingsWidget.hpp"

#include "ui_TemplateSettingsWidget.h"

#include "FHMapReflection.hpp"
#include "LibraryModels.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryWrappersMetatype.hpp"

#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"

#include "GameDatabasePropertyReader.hpp"
#include "GameDatabasePropertyWriter.hpp"

#include <QTimer>

namespace FreeHeroes {

TemplateSettingsWidget::TemplateSettingsWidget(const Gui::LibraryModelsProvider* modelsProvider,
                                               QWidget*                          parent)
    : QFrame(parent)
    , m_ui(std::make_unique<Ui::TemplateSettingsWidget>())
    , m_userSettings(std::make_unique<FHRngUserSettings>())
    , m_modelsProvider(modelsProvider)
{
    m_ui->setupUi(this, std::tuple{ modelsProvider });

    QList<TemplatePlayerWidget*> widgets{
        m_ui->playerWidgetRed,
        m_ui->playerWidgetBlue,
        m_ui->playerWidgetTan,
        m_ui->playerWidgetGreen,
        m_ui->playerWidgetOrange,
        m_ui->playerWidgetPurple,
        m_ui->playerWidgetTeal,
        m_ui->playerWidgetPink,
    };
    for (int i = 0; i < widgets.size(); i++) {
        TemplatePlayerWidget* w      = widgets[i];
        auto                  player = modelsProvider->players()->data(modelsProvider->players()->index(i, 0), Gui::PlayersModel::GuiObject).value<Gui::GuiPlayerConstPtr>();

        auto pix = player->getIcon();
        w->setPlayerColorText(player->getName(), pix);
        m_mapping[player->getSource()] = w;
    }
    for (int i = 0; i <= 6; i++)
        m_ui->comboBoxMapSize->setItemData(i, 36 * (i + 1));

    connect(m_ui->pushButtonReset, &QAbstractButton::clicked, this, [this] {
        *m_userSettings = {};
        updateUI();
    });
}

TemplateSettingsWidget::~TemplateSettingsWidget()
{
}

void TemplateSettingsWidget::load(const Mernel::std_path& path)
{
    m_path = path;

    if (Mernel::std_fs::exists(m_path)) {
        std::string buffer       = Mernel::readFileIntoBuffer(m_path);
        auto        settingsJson = Mernel::readJsonFromBuffer(buffer);

        auto*                            db = m_modelsProvider->database();
        Core::PropertyTreeReaderDatabase reader(db);
        reader.jsonToValue(settingsJson, *m_userSettings);
    }

    updateUI();
}

void TemplateSettingsWidget::updateUI()
{
    for (auto [id, w] : m_mapping) {
        w->setConfig(m_userSettings->m_players[id]);
    }

    for (int i = 0; i < m_ui->comboBoxMapSize->count(); ++i) {
        if (m_ui->comboBoxMapSize->itemData(i).value<int>() >= m_userSettings->m_mapSize) {
            m_ui->comboBoxMapSize->setCurrentIndex(i);
            break;
        }
    }
}

void TemplateSettingsWidget::save()
{
    for (auto [id, w] : m_mapping) {
        m_userSettings->m_players[id] = w->getConfig();
    }

    m_userSettings->m_mapSize = m_ui->comboBoxMapSize->currentData().toInt();

    Mernel::PropertyTree             jsonData;
    Core::PropertyTreeWriterDatabase writer;
    writer.valueToJson(*m_userSettings, jsonData);

    std::string buffer = Mernel::writeJsonToBuffer(jsonData, true);
    Mernel::writeFileFromBuffer(m_path, buffer);
}

}
