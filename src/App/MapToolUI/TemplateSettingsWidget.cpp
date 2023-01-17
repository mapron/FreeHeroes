/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplateSettingsWidget.hpp"

#include "ui_TemplateSettingsWidget.h"

#include "FHMapReflection.hpp"
#include "LibraryModels.hpp"

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

    std::map<FHPlayerId, QString> colorMap{

        { FHPlayerId::Red, tr("Red") },
        { FHPlayerId::Blue, tr("Blue") },
        { FHPlayerId::Tan, tr("Tan") },
        { FHPlayerId::Green, tr("Green") },
        { FHPlayerId::Orange, tr("Orange") },
        { FHPlayerId::Purple, tr("Purple") },
        { FHPlayerId::Teal, tr("Teal") },
        { FHPlayerId::Pink, tr("Pink") },
    };

    m_mapping = {

        { FHPlayerId::Red, m_ui->playerWidgetRed },
        { FHPlayerId::Blue, m_ui->playerWidgetBlue },
        { FHPlayerId::Tan, m_ui->playerWidgetTan },
        { FHPlayerId::Green, m_ui->playerWidgetGreen },
        { FHPlayerId::Orange, m_ui->playerWidgetOrange },
        { FHPlayerId::Purple, m_ui->playerWidgetPurple },
        { FHPlayerId::Teal, m_ui->playerWidgetTeal },
        { FHPlayerId::Pink, m_ui->playerWidgetPink },
    };

    for (auto [id, w] : m_mapping) {
        w->setPlayerColorText(colorMap.at(id));
    }

    m_ui->comboBoxMapSize->setItemData(0, 36);
    m_ui->comboBoxMapSize->setItemData(1, 72);
    m_ui->comboBoxMapSize->setItemData(2, 108);
    m_ui->comboBoxMapSize->setItemData(3, 144);
    m_ui->comboBoxMapSize->setItemData(4, 180);
    m_ui->comboBoxMapSize->setItemData(5, 216);
    m_ui->comboBoxMapSize->setItemData(6, 252);

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
