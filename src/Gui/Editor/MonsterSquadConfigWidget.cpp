/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MonsterSquadConfigWidget.hpp"

#include "ui_MonsterSquadConfigWidget.h"

#include "AdventureWrappers.hpp"
#include "LibraryModels.hpp"

namespace FreeHeroes::Gui {
using namespace Core;

MonsterSquadConfigWidget::MonsterSquadConfigWidget(const LibraryModelsProvider* modelProvider, QWidget* parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::MonsterSquadConfigWidget>())
    , m_modelProvider(modelProvider)
{
    m_ui->setupUi(this);

    QList<QPushButton*> modeButtons{
        m_ui->pushButtonSelectArmy
    };
    QList<QWidget*> modePages{
        m_ui->pageArmy
    };
    for (auto* modeButton : modeButtons) {
        connect(modeButton, &QPushButton::clicked, this, [this, modeButton, modeButtons, modePages]() {
            const int index = modeButtons.indexOf(qobject_cast<QPushButton*>(sender()));
            for (auto* modeButtonOther : modeButtons) {
                modeButtonOther->setChecked(modeButtonOther == modeButton);
            }
            this->m_ui->stackedWidget->setCurrentWidget(modePages[index]);
        });
    }
}

void MonsterSquadConfigWidget::setModels()
{
    m_ui->stackEditorPack->setUnitsModel(m_modelProvider->units());
}

MonsterSquadConfigWidget::~MonsterSquadConfigWidget() = default;

void MonsterSquadConfigWidget::setSource(GuiAdventureArmy* adventureArmy)
{
    m_ui->stackEditorPack->setSource(adventureArmy->getSquad());
}

}
