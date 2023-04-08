/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplateSettingsWidget.hpp"

#include "ui_TemplateSettingsWidget.h"

#include "FHTemplateReflection.hpp"
#include "LibraryModels.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryWrappersMetatype.hpp"

#include <QTimer>

namespace FreeHeroes {

TemplateSettingsWidget::TemplateSettingsWidget(const Gui::LibraryModelsProvider* modelsProvider,
                                               QWidget*                          parent)
    : QFrame(parent)
    , m_ui(std::make_unique<Ui::TemplateSettingsWidget>())
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

    connect(m_ui->pushButtonReset, &QAbstractButton::clicked, this, [this] {
        m_userSettings->m_players.clear();
        updateUI();
    });
}

TemplateSettingsWidget::~TemplateSettingsWidget() = default;

void TemplateSettingsWidget::updateUI()
{
    int totalEnabled = 0;
    for (auto [id, w] : m_mapping)
        totalEnabled += m_userSettings->m_players[id].m_enabled;
    if (!totalEnabled) {
        for (auto [id, w] : m_mapping) {
            if (w == m_ui->playerWidgetRed || w == m_ui->playerWidgetBlue)
                m_userSettings->m_players[id].m_enabled = true;
        }
    }

    for (auto [id, w] : m_mapping) {
        w->setConfig(m_userSettings->m_players[id]);
    }
}

void TemplateSettingsWidget::save()
{
    for (auto [id, w] : m_mapping) {
        m_userSettings->m_players[id] = w->getConfig();
    }
}

}
