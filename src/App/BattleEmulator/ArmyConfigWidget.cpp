/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ArmyConfigWidget.hpp"
#include "ui_ArmyConfigWidget.h"

#include "AdventureControl.hpp"

// Gui
#include "AdventureWrappers.hpp"
#include "LibraryModels.hpp"
#include "LibraryEditorModels.hpp"
#include "LibraryWrappersMetatype.hpp"
#include "HeroMainDialog.hpp"
#include "DependencyInjector.hpp"

// Core
#include "IRandomGenerator.hpp"
#include "LibraryFaction.hpp"
#include "LibraryMapBank.hpp"
#include "AdventureArmy.hpp"

// Platform
#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes::BattleEmulator {
using namespace Core;
using namespace Gui;

ArmyConfigWidget::ArmyConfigWidget(const Gui::LibraryModelsProvider* modelProvider,
                                   Core::IRandomGenerator*           randomGenerator,
                                   QWidget*                          parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::ArmyConfigWidget>())
    , m_modelProvider(modelProvider)
    , m_randomGenerator(randomGenerator)
{
    Mernel::ProfilerScope scope("ArmyConfigWidget()");
    m_ui->setupUi(this, std::tuple{ modelProvider, randomGenerator });
    Mernel::ProfilerScope scope2("ArmyConfigWidget() - after ui");

    connect(m_ui->pushButtonGenerateArmy, &QPushButton::clicked, this, &ArmyConfigWidget::generate);

    QList<QRadioButton*> modeSelectors{ m_ui->radioButtonSquad, m_ui->radioButtonHero };
    for (auto* selector : modeSelectors) {
        connect(selector, &QRadioButton::clicked, this, [this] {
            const bool useHero = m_ui->radioButtonHero == sender();
            const bool changed = m_army->getHero()->getSource()->isValid() != useHero;
            if (!changed)
                return;
            if (!useHero)
                m_army->getHero()->setHero(nullptr);
            else
                m_ui->heroWithArmyConfigWidget->initHero();
        });
    }
    connect(m_ui->heroWithArmyConfigWidget, &HeroWithArmyConfigWidget::showHeroDialog, this, &ArmyConfigWidget::showHeroDialog);

    connect(m_ui->pushButtonLevelUpHero, &QPushButton::clicked, this, &ArmyConfigWidget::makeLevelupInternal);
}

ArmyConfigWidget::~ArmyConfigWidget() = default;

void ArmyConfigWidget::setAIControl(bool isAI)
{
    m_ui->checkBoxAIControl->setChecked(isAI);
}

bool ArmyConfigWidget::isAIControl() const
{
    return m_ui->checkBoxAIControl->isChecked();
}

void ArmyConfigWidget::initFromMapObject(LibraryMapBankConstPtr mapObject, int variant)
{
    auto& guards = mapObject->variants[variant].guards;
    for (size_t i = 0; i < m_army->getSquad()->getCount(); ++i) {
        if (i < guards.size()) {
            m_army->getSquad()->getStack(i)->setParams(guards[i].unit, guards[i].count);
        } else {
            m_army->getSquad()->getStack(i)->setCount(0);
        }
    }
}

void ArmyConfigWidget::refresh()
{
    const bool heroExists = m_army->getHero()->getSource()->isValid();
    m_ui->radioButtonHero->setChecked(heroExists);
    m_ui->radioButtonSquad->setChecked(!heroExists);
    m_ui->stackedWidget->setCurrentIndex(heroExists ? 0 : 1);
    if (heroExists)
        m_ui->heroWithArmyConfigWidget->refresh();

    if (m_tmpRefresh)
        m_tmpRefresh();
}

void ArmyConfigWidget::setModels()
{
    m_ui->heroWithArmyConfigWidget->setModels();
    m_ui->monsterSquadConfigWidget->setModels();

    m_unitsFilter = new UnitsFilterModel(this);
    m_unitsFilter->setSourceModel(m_modelProvider->units());

    FactionsFilterModel* factionsFilter = new FactionsFilterModel(true, this);
    factionsFilter->setSourceModel(m_modelProvider->factions());
    FactionsComboModel* factionsCombo = new FactionsComboModel(factionsFilter, this);
    m_ui->comboBoxFactionSelect->setModel(factionsCombo);
}

void ArmyConfigWidget::setSource(Gui::GuiAdventureArmy* army)
{
    m_army = army;
    Q_ASSERT(m_modelProvider);

    m_ui->heroWithArmyConfigWidget->setSource(m_army);
    m_ui->monsterSquadConfigWidget->setSource(m_army);

    m_adventureControl = std::make_unique<AdventureControl>(*m_army);
}

void ArmyConfigWidget::initHero()
{
    m_ui->heroWithArmyConfigWidget->initHero();
}

void ArmyConfigWidget::generate()
{
    using namespace Core;
    const auto faction             = m_ui->comboBoxFactionSelect->currentData(FactionsModel::SourceObject).value<Core::LibraryFactionConstPtr>();
    auto       generateRandomStack = [this, faction](int minLevel, int maxLevel, int& value) -> AdventureStack {
        QList<AdventureStack> alternatives;
        for (int row = 0; row < m_unitsFilter->rowCount(); ++row) {
            auto unit = m_unitsFilter->index(row, 0).data(UnitsModel::SourceObject).value<LibraryUnitConstPtr>();
            if (unit->level < minLevel
                || unit->level > maxLevel
                || (!!faction && unit->faction != faction))
                continue;
            if (unit->value > value)
                continue;

            const int count = value / unit->value;
            alternatives << AdventureStack{ unit, count };
        }
        auto r = alternatives.value(m_randomGenerator->gen(alternatives.size() - 1));
        if (r.library)
            value -= r.count * r.library->value;
        return r;
    };
    int level = 70;

    int        totalValue     = m_ui->spinBoxArmyValue->value();
    QList<int> denominators   = { 9, 9, 9, 8, 7, 5, 5 };
    int        remainingValue = 0;
    for (size_t i = 0; i < m_army->getSquad()->getCount(); ++i) {
        remainingValue += totalValue / denominators.value(i);
        auto advStack = generateRandomStack(level, level + 9, remainingValue);
        m_army->getSquad()->getStack(m_army->getSquad()->getCount() - i - 1)->setParams(advStack.library, advStack.count);
        level -= 10;
    }
}

void ArmyConfigWidget::showHeroDialog()
{
    HeroMainDialog dlg(m_modelProvider, this);
    m_tmpRefresh = [&dlg] {
        dlg.refresh();
    };
    dlg.setSource(m_army, m_adventureControl.get(), m_adventureControl.get());
    dlg.refresh();
    dlg.exec();

    m_tmpRefresh = {};
}

void ArmyConfigWidget::makeLevelupInternal()
{
    if (!m_army->getSource()->hasHero())
        return;
    const int levelUps = m_ui->spinBoxLevelUpsCount->value();
    const int newLevel = std::min(m_army->getHero()->getSource()->level + levelUps, 75);
    emit      makeLevelup(newLevel);
}

}
