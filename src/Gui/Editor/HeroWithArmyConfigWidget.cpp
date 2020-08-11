/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "HeroWithArmyConfigWidget.hpp"

#include "ui_HeroWithArmyConfigWidget.h"

#include "ArtifactSelectorWidget.hpp"
#include "SkillSelectorWidget.hpp"
#include "LibraryEditorModels.hpp"

// Gui
#include "SignalsBlocker.hpp"
#include "ResizePixmap.hpp"
#include "FormatUtils.hpp"
#include "LibraryWrappersMetatype.hpp"
#include "AdventureWrappers.hpp"

// Core
#include "IRandomGenerator.hpp"
#include "AdventureArmy.hpp"
#include "LibraryFaction.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryHeroSpec.hpp"

// Platform
#include "Profiler.hpp"

namespace FreeHeroes::Gui {

using namespace Core;



struct HeroWithArmyConfigWidget::Impl {
};

HeroWithArmyConfigWidget::HeroWithArmyConfigWidget(QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>())
    , m_ui(std::make_unique<Ui::HeroWithArmyConfigWidget>())
{
    ProfilerScope scope("HeroWithArmyConfigWidget()");
    {
        ProfilerScope scope("setupUi");
        m_ui->setupUi(this);
    }
    ProfilerScope scope2("remain");

    QList<QSpinBox*> primarySpinBoxes{m_ui->spinBoxAttack,
        m_ui->spinBoxDefense,
        m_ui->spinBoxSp  ,
        m_ui->spinBoxInt  };

    for (auto * box : primarySpinBoxes) {
        connect(box, qOverload<int>(&QSpinBox::valueChanged), this, &HeroWithArmyConfigWidget::primaryStatEdited);
    }
    connect(m_ui->spinBoxLevel, qOverload<int>(&QSpinBox::valueChanged), this, &HeroWithArmyConfigWidget::levelEdited);
    connect(m_ui->spinBoxExp, qOverload<int>(&QSpinBox::valueChanged), this, &HeroWithArmyConfigWidget::expEdited);


    QList<QPushButton*> modeButtons{
        m_ui->pushButtonSelectArmy,
        m_ui->pushButtonSelectPrimary,
        m_ui->pushButtonSelectAppearence,
        m_ui->pushButtonSelectSecondary,
        m_ui->pushButtonSelectArtifactsWearing,
        m_ui->pushButtonSelectArtifactsBag,
        m_ui->pushButtonSelectSpells,
        m_ui->pushButtonSelectOverview
    };
    QList<QWidget*> modePages{
        m_ui->pageArmy,
        m_ui->pagePrimary,
        m_ui->pageAppearence,
        m_ui->pageSecondary,
        m_ui->pageArtifactsWearing,
        m_ui->pageArtifactsBag,
        m_ui->pageSpells,
        m_ui->pageOverview
    };
    for (auto * modeButton : modeButtons) {
        connect(modeButton, &QPushButton::clicked, this, [this, modeButton, modeButtons, modePages](){
            const int index = modeButtons.indexOf(qobject_cast<QPushButton*>(sender()));
            for (auto * modeButtonOther : modeButtons) {
                modeButtonOther->setChecked(modeButtonOther == modeButton);
            }
            this->m_ui->stackedWidget->setCurrentWidget(modePages[index]);
        });
    }
    m_ui->stackedWidget->setCurrentWidget(m_ui->pageArmy);


    connect(m_ui->pushButtonShowHeroDialog, &QPushButton::clicked, this, &HeroWithArmyConfigWidget::showHeroDialog);

    connect(m_ui->pushButtonAddAllArtifacts  , &QPushButton::clicked, this, &HeroWithArmyConfigWidget::makeAllArtifacts);
    connect(m_ui->pushButtonClearArtifactsBag, &QPushButton::clicked, this, &HeroWithArmyConfigWidget::makeNoArtifacts);

    connect(m_ui->pushButtonSelectAllSpells, &QPushButton::clicked, this, &HeroWithArmyConfigWidget::makeAllSpells);
    connect(m_ui->pushButtonSelectNoneSpells, &QPushButton::clicked, this, &HeroWithArmyConfigWidget::makeNoSpells);
    connect(m_ui->checkBoxHasSpellbook, &QCheckBox::clicked, this, [this](){
        const bool state = m_ui->checkBoxHasSpellbook->isChecked();
        m_hero->getSource()->hasSpellBook = state;
        m_hero->emitChanges();
    });

    connect(m_ui->comboBoxHeroIdentity, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &HeroWithArmyConfigWidget::heroIndexChanged);
}

HeroWithArmyConfigWidget::~HeroWithArmyConfigWidget() = default;

void HeroWithArmyConfigWidget::refresh()
{
    if (m_hero->getSource()->isValid()) {
        displayHeroSummary();
        displayHeroAppearence();
        displayHeroArtifacts();
        displayHeroSkills();
        displayHeroSpells();
        displayHeroPrimary();
        m_ui->currentStatsWidget->refresh();
    }
}

void HeroWithArmyConfigWidget::setModels(LibraryModelsProvider& modelProvider, Core::IRandomGenerator * randomGenerator)
{

    m_ui->stackEditorPack->setUnitsModel(modelProvider.units());

    m_modelProvider = &modelProvider;
    m_randomGenerator = randomGenerator;

    m_ui->comboBoxHeroIdentity->setIconSize({48, 32});

    HeroesComboModel * comboModel = new HeroesComboModel(modelProvider.heroes(), m_ui->comboBoxHeroIdentity);

    m_ui->comboBoxHeroIdentity->setModel(comboModel);

}

void HeroWithArmyConfigWidget::setSource(GuiAdventureArmy* adventureArmy)
{
    m_adventureArmy = adventureArmy;
    m_hero = adventureArmy->getHero();
    m_ui->currentStatsWidget->setSource(m_hero->getSource());

    m_ui->stackEditorPack->setSource(adventureArmy->getSquad());

    m_hero->createArtifactsModelsIfNeeded(m_modelProvider->artifacts());
    m_hero->createSpellsModelsIfNeeded(m_modelProvider->spells());
    m_hero->createSkillsModelsIfNeeded(m_modelProvider->skills());

    m_ui->tableViewBag->setModel(m_hero->getBagEditModel());
    m_ui->tableViewBag->setColumnWidth(0, 240);
    m_ui->tableViewBag->setColumnWidth(1, 40);

    m_ui->tableViewWearing->setModel(m_hero->getWearingEditModel());
    m_ui->tableViewWearing->setItemDelegateForColumn(1, new ArtifactSelectDelegate(m_modelProvider->artifacts(), this));
    m_ui->tableViewWearing->setTextElideMode(Qt::ElideNone);
    m_ui->tableViewWearing->setColumnWidth(0, 100);
    m_ui->tableViewWearing->setColumnWidth(1, 200);
    {
        QHeaderView *verticalHeader = m_ui->tableViewWearing->verticalHeader();
        verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
        verticalHeader->setDefaultSectionSize(32);
    }
    {
        QHeaderView *verticalHeader = m_ui->tableViewBag->verticalHeader();
        verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
        verticalHeader->setDefaultSectionSize(32);
    }
    m_ui->listViewSpells->setModel(m_hero->getSpellbookEditModel());

    m_ui->tableViewSecondarySkills->setModel(m_hero->getSkillsEditModel());
    m_ui->tableViewSecondarySkills->setItemDelegate(new SkillSelectDelegate(m_modelProvider->skills(), this));
    m_ui->tableViewSecondarySkills->setColumnWidth(0, 150);
    m_ui->tableViewSecondarySkills->setColumnWidth(1, 150);

}


void HeroWithArmyConfigWidget::initHero()
{
    auto heroLibrary = m_ui->comboBoxHeroIdentity->currentData(HeroesModel::SourceObject).value<LibraryHeroConstPtr>();
    Q_ASSERT(heroLibrary);

    m_hero->setHero(heroLibrary);

    if (m_ui->checkBoxUseDefaultArmy->isChecked())
        useHeroDefaultArmy();
}
void HeroWithArmyConfigWidget::primaryStatEdited()
{
    auto hero = m_hero->getSource();
    auto old = hero->currentBasePrimary;
    hero->currentBasePrimary.ad.attack            = m_ui->spinBoxAttack->value();
    hero->currentBasePrimary.ad.defense           = m_ui->spinBoxDefense->value();
    hero->currentBasePrimary.magic.spellPower     = m_ui->spinBoxSp->value();
    hero->currentBasePrimary.magic.intelligence   = m_ui->spinBoxInt->value();

    if (old != hero->currentBasePrimary)
        m_hero->emitChanges();
}

void HeroWithArmyConfigWidget::expEdited()
{
    auto hero = m_hero->getSource();
    const bool changed = hero->experience != m_ui->spinBoxExp->value();
    if (!changed)
        return;

    hero->experience = m_ui->spinBoxExp->value();
    hero->editorParams.levelIsDirty = true;
    m_hero->emitChanges();
}

void HeroWithArmyConfigWidget::levelEdited()
{
    auto hero = m_hero->getSource();
    const bool changed = hero->level != m_ui->spinBoxLevel->value();
    if (!changed)
        return;

    hero->level = m_ui->spinBoxLevel->value();
    hero->editorParams.expIsDirty = true;
    m_hero->emitChanges();
}

void HeroWithArmyConfigWidget::heroIndexChanged()
{
    if (!m_hero)
        return;

    initHero();
}

void HeroWithArmyConfigWidget::displayHeroPrimary()
{
    SignalBlocker lock({
       m_ui->spinBoxAttack  ,
       m_ui->spinBoxDefense ,
       m_ui->spinBoxSp      ,
       m_ui->spinBoxInt     ,
       m_ui->spinBoxLevel   ,
       m_ui->spinBoxExp     ,
    });
    auto hero = m_hero->getSource();
    m_ui->spinBoxAttack ->setValue(hero->currentBasePrimary.ad.attack);
    m_ui->spinBoxDefense->setValue(hero->currentBasePrimary.ad.defense);
    m_ui->spinBoxSp     ->setValue(hero->currentBasePrimary.magic.spellPower);
    m_ui->spinBoxInt    ->setValue(hero->currentBasePrimary.magic.intelligence);
    m_ui->spinBoxLevel  ->setValue(hero->level);
    m_ui->spinBoxExp    ->setValue(hero->experience);
}

void HeroWithArmyConfigWidget::displayHeroSummary()
{
    m_ui->labelExperienceMin->setText(FormatUtils::formatLargeInt(m_hero->getSource()->estimated.experienceStartLevel));
    m_ui->labelExperienceMax->setText(FormatUtils::formatLargeInt(m_hero->getSource()->estimated.experienceNextLevel - 1));

}

void HeroWithArmyConfigWidget::displayHeroAppearence()
{
    m_ui->lineEditCustomName->setText(m_hero->getGuiHero()->getName());
    m_ui->labelPortraitLarge->setPixmap(m_hero->getGuiHero()->getPortraitLarge());
    m_ui->labelPortraitSmall->setPixmap(m_hero->getGuiHero()->getPortraitSmall());

    m_ui->labelSpecialityIcon->setPixmap(m_hero->getGuiHero()->getSpecIcon());

    m_ui->labelSpecialityText->setText(m_hero->getGuiHero()->getSpecName());

    for (int i = 0; i< m_ui->comboBoxHeroIdentity->count(); ++i) {
        if (m_ui->comboBoxHeroIdentity->itemData(i, HeroesModel::GuiObject).value<HeroesModel::WrapperTypePtr>() == m_hero->getGuiHero()) {
            m_ui->comboBoxHeroIdentity->blockSignals(true);
            m_ui->comboBoxHeroIdentity->setCurrentIndex(i);
            m_ui->comboBoxHeroIdentity->blockSignals(false);
            break;
        }
    }

}

void HeroWithArmyConfigWidget::displayHeroArtifacts()
{
    m_hero->refreshArtifactsModels();
}

void HeroWithArmyConfigWidget::displayHeroSkills()
{
    m_hero->refreshSkillsModels();
}

void HeroWithArmyConfigWidget::displayHeroSpells()
{
    m_hero->refreshSpellsModels();
}

void HeroWithArmyConfigWidget::useHeroDefaultArmy()
{
   // return;
   // int foo = 1;
    // @todo: maybe not utilize RNG here, but emit signal to generate army at top level.
    auto advHero = m_adventureArmy->getHero()->getSource();
    Q_ASSERT(advHero);
    if (!advHero)
        return;

    auto libraryHero = advHero->library;
    if (!libraryHero)
        return;

    m_adventureArmy->getSquad()->clearAll();

    int index = 0;
    for (size_t i=0; i < libraryHero->startStacks.size(); ++i) {
        auto unit = libraryHero->startStacks[i];
        auto sizeParams = advHero->getExpectedStackSize(i);
        const int from   = sizeParams.min;
        const int spread =  sizeParams.max - sizeParams.min;
        if (unit.unit->faction->alignment == LibraryFaction::Alignment::Special) {
            auto art = unit.unit->battleMachineArtifact;
            Q_ASSERT(art);
            m_hero->getSource()->artifactsOn[art->slot] = art;
        } else {
            auto guiStack = m_adventureArmy->getSquad()->getStack(index++);
            guiStack->setParams(unit.unit, from + m_randomGenerator->genSmall(spread));
        }
    }
    m_adventureArmy->emitChanges();
}

void HeroWithArmyConfigWidget::makeAllSpells()
{
    if (!m_ui->checkBoxHasSpellbook->isChecked()) {
        m_ui->checkBoxHasSpellbook->click();
    }
    auto * spellModel = m_hero->getSpellbookEditModel();
    for (int row = 0; row < spellModel->rowCount(); row++) {
        auto * spell = spellModel->index(row, 0).data(SpellsModel::SourceObject).value<LibrarySpellConstPtr>();
        m_hero->getSource()->spellbook.insert(spell);
    }
    m_hero->emitChanges();
}

void HeroWithArmyConfigWidget::makeNoSpells()
{
    m_hero->getSource()->spellbook.clear();
    m_hero->emitChanges();
}

void HeroWithArmyConfigWidget::makeAllArtifacts()
{
    auto * bagModel = m_hero->getBagEditModel();
    for (int row = 0; row < bagModel->rowCount(); row++) {
        auto * art = bagModel->index(row, 0).data(ArtifactsModel::SourceObject).value<LibraryArtifactConstPtr>();
        m_hero->getSource()->artifactsBag[art] = 1;
    }
    m_hero->refreshArtifactsModels();
    m_hero->emitChanges();
}

void HeroWithArmyConfigWidget::makeNoArtifacts()
{
    m_hero->getSource()->artifactsBag.clear();
    m_hero->refreshArtifactsModels();
    m_hero->emitChanges();
}

}

