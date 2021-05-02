/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureWrappers.hpp"

#include "AdventureStack.hpp"
#include "AdventureSquad.hpp"
#include "AdventureHero.hpp"
#include "AdventureArmy.hpp"
#include "AdventureModels.hpp"

namespace FreeHeroes::Gui {

GuiAdventureStack::GuiAdventureStack(GuiUnitProvider& unitProvider, Core::AdventureStackMutablePtr source)
    : m_unitProvider(unitProvider)
    , m_source(source)
{
    Q_ASSERT(source);
    m_guiUnit = m_unitProvider.find(m_source->library);
}

void GuiAdventureStack::setUnit(Core::LibraryUnitConstPtr unit)
{
    const bool changed = m_source->library != unit;
    m_source->library  = unit;
    if (unit && !m_source->count) {
        m_source->count = 1;
    }
    m_guiUnit = m_unitProvider.find(m_source->library);
    if (changed)
        emit dataChanged();
}

void GuiAdventureStack::setCount(int count)
{
    const bool changed = m_source->count != count;
    m_source->count    = count;
    if (count == 0) {
        m_source->library = nullptr;
        m_guiUnit         = nullptr;
    }
    if (changed)
        emit dataChanged();
}

void GuiAdventureStack::setParams(Core::LibraryUnitConstPtr unit, int count)
{
    const bool changed = m_source->count != count || m_source->library != unit;
    if (count && unit) {
        m_source->count   = count;
        m_source->library = unit;
        m_guiUnit         = m_unitProvider.find(m_source->library);
    } else {
        m_source->count   = 0;
        m_source->library = nullptr;
        m_guiUnit         = nullptr;
    }
    if (changed)
        emit dataChanged();
}

bool GuiAdventureStack::isValid() const
{
    return m_source->isValid();
}

void GuiAdventureStack::updateGuiState()
{
    m_guiUnit = m_unitProvider.find(m_source->library);
}

void GuiAdventureStack::emitChanges()
{
    emit dataChanged();
}

GuiUnitConstPtr GuiAdventureStack::getGuiUnit() const
{
    return m_source->isValid() ? m_guiUnit : nullptr;
}

GuiAdventureSquad::GuiAdventureSquad(GuiUnitProvider& unitProvider, Core::AdventureSquadMutablePtr source)
{
    Q_ASSERT(source);
    m_source = source;
    for (auto& stack : source->stacks) {
        m_stacks.emplace_back(unitProvider, &stack);
        connect(&m_stacks.back(), &GuiAdventureStack::dataChanged, this, &GuiAdventureSquad::dataChanged);
    }
}

const GuiAdventureStack* GuiAdventureSquad::getStack(size_t index) const
{
    return &m_stacks[index];
}

GuiAdventureStack* GuiAdventureSquad::getStack(size_t index)
{
    return &m_stacks[index];
}

int GuiAdventureSquad::getStackCount(size_t index) const
{
    return m_stacks[index].getSource()->count;
}

Core::LibraryUnitConstPtr GuiAdventureSquad::getStackUnitLibrary(size_t index) const
{
    return m_stacks[index].getSource()->library;
}

void GuiAdventureSquad::clearAll()
{
    this->blockSignals(true);
    for (auto& stack : m_stacks) {
        stack.setUnit(nullptr);
        stack.setCount(0);
    }
    this->blockSignals(false);
    emit dataChanged();
}

void GuiAdventureSquad::updateGuiState()
{
    for (auto& stack : m_stacks) {
        stack.updateGuiState();
    }
}

void GuiAdventureSquad::emitChanges()
{
    this->blockSignals(true);

    for (auto& stack : m_stacks) {
        stack.emitChanges();
    }
    this->blockSignals(false);
    emit dataChanged();
}

void GuiAdventureSquad::externalChange()
{
    updateGuiState();
    emitChanges();
}

size_t GuiAdventureSquad::getCount() const
{
    return m_stacks.size();
}

void GuiAdventureSquad::setFormation(bool compact)
{
    const bool changed = compact != m_source->useCompactFormation;
    if (!changed)
        return;
    m_source->useCompactFormation = compact;
    emit dataChanged();
}

GuiAdventureHero::GuiAdventureHero(GuiHeroProvider& heroProvider, Core::AdventureHeroMutablePtr source)
    : m_heroProvider(heroProvider)
    , m_source(source)
{
    Q_ASSERT(source);
    m_guiHero = m_heroProvider.find(m_source->library);
    Q_ASSERT(m_guiHero || !m_source->library);
}

GuiAdventureHero::~GuiAdventureHero() = default;

void GuiAdventureHero::setHero(Core::LibraryHeroConstPtr hero)
{
    const bool changed = m_source->library != hero;
    if (!changed)
        return;

    m_source->reset(hero);
    m_guiHero = m_heroProvider.find(m_source->library);
    Q_ASSERT(m_guiHero || !m_source->library);
    emit dataChanged();
}

void GuiAdventureHero::resetHeroToDefault()
{
    *m_source = Core::AdventureHero(m_source->library);
    emit dataChanged();
}

void GuiAdventureHero::createArtifactsModelsIfNeeded(ArtifactsModel* artifacts)
{
    if (m_bagEditModel)
        return;

    m_bagEditModel     = new HeroBagEditModel(artifacts, m_source, this);
    m_wearingEditModel = new HeroWearingEditModel(artifacts, m_source, this);
    connect(m_bagEditModel, &HeroBagEditModel::dataChanged, this, &GuiAdventureHero::dataChanged);
    connect(m_wearingEditModel, &HeroWearingEditModel::dataChanged, this, &GuiAdventureHero::dataChanged);
}

void GuiAdventureHero::refreshArtifactsModels()
{
    if (!m_bagEditModel)
        return;
    this->blockSignals(true);
    m_bagEditModel->refresh();
    m_wearingEditModel->refresh();
    this->blockSignals(false);
}

void GuiAdventureHero::createSpellsModelsIfNeeded(SpellsModel* spellsModel)
{
    if (m_spellbookEditModel)
        return;
    m_spellbookEditModel = new HeroSpellbookEditModel(spellsModel, m_source, this);
    connect(m_spellbookEditModel, &HeroSpellbookEditModel::dataChanged, this, &GuiAdventureHero::dataChanged);
}

void GuiAdventureHero::refreshSpellsModels()
{
    if (!m_spellbookEditModel)
        return;
    this->blockSignals(true);
    m_spellbookEditModel->refresh();
    this->blockSignals(false);
}

void GuiAdventureHero::createSkillsModelsIfNeeded(SkillsModel* skillsModel)
{
    if (m_skillsEditModel)
        return;
    m_skillsEditModel = new HeroSkillsEditModel(skillsModel, m_source, this);
    connect(m_skillsEditModel, &HeroSkillsEditModel::dataChanged, this, &GuiAdventureHero::dataChanged);
}

void GuiAdventureHero::refreshSkillsModels()
{
    if (!m_skillsEditModel)
        return;
    this->blockSignals(true);
    m_skillsEditModel->refresh();
    this->blockSignals(false);
}

QString GuiAdventureHero::getName() const
{
    return m_guiHero->getName();
}

QString GuiAdventureHero::getClassName() const
{
    return m_guiHero->getClassName();
}

GuiHeroConstPtr GuiAdventureHero::getGuiHero() const
{
    return m_source->isValid() ? m_guiHero : nullptr;
}

QAbstractItemModel* GuiAdventureHero::getBagEditModel() const
{
    return m_bagEditModel;
}

QAbstractItemModel* GuiAdventureHero::getWearingEditModel() const
{
    return m_wearingEditModel;
}

QAbstractItemModel* GuiAdventureHero::getSpellbookEditModel() const
{
    return m_spellbookEditModel;
}

QAbstractItemModel* GuiAdventureHero::getSkillsEditModel() const
{
    return m_skillsEditModel;
}

void GuiAdventureHero::updateGuiState()
{
    m_guiHero = m_heroProvider.find(m_source->library);
    Q_ASSERT(!m_source->library || m_guiHero);
}

void GuiAdventureHero::emitChanges()
{
    emit dataChanged();
}

void GuiAdventureHero::externalChange()
{
    updateGuiState();
    emitChanges();
}

GuiAdventureArmy::GuiAdventureArmy(GuiUnitProvider&              unitProvider,
                                   GuiHeroProvider&              heroProvider,
                                   Core::AdventureArmyMutablePtr source)
    : m_source(source)
    , m_squad(unitProvider, &source->squad)
    , m_hero(heroProvider, &source->hero)
{
    connect(&m_squad, &GuiAdventureSquad::dataChanged, this, &GuiAdventureArmy::dataChanged);
    connect(&m_hero, &GuiAdventureHero::dataChanged, this, &GuiAdventureArmy::dataChanged);
}

void GuiAdventureArmy::updateGuiState()
{
    m_squad.updateGuiState();
    m_hero.updateGuiState();
}

void GuiAdventureArmy::emitChanges()
{
    this->blockSignals(true);
    m_squad.emitChanges();
    m_hero.emitChanges();
    this->blockSignals(false);
    emit dataChanged();
}

void GuiAdventureArmy::externalChange()
{
    updateGuiState();
    emitChanges();
}

}
