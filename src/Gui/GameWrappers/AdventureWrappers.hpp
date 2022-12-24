/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryWrappers.hpp"

#include "AdventureFwd.hpp"

#include <QObject>

#include <deque>

#include "GuiGameWrappersExport.hpp"

class QAbstractItemModel;

namespace FreeHeroes::Gui {

class GUIGAMEWRAPPERS_EXPORT GuiAdventureStack : public QObject {
    Q_OBJECT

public:
    GuiAdventureStack(GuiUnitProvider& unitProvider, Core::AdventureStackMutablePtr source);

    void setUnit(Core::LibraryUnitConstPtr unit);
    void setCount(int count);
    void setParams(Core::LibraryUnitConstPtr unit, int count);

    bool isValid() const;
    void updateGuiState();
    void emitChanges();

    GuiUnitConstPtr getGuiUnit() const;

    Core::AdventureStackConstPtr getSource() const { return m_source; }

signals:
    void dataChanged();

private:
    GuiUnitProvider&               m_unitProvider;
    Core::AdventureStackMutablePtr m_source  = nullptr;
    GuiUnitConstPtr                m_guiUnit = nullptr;
};
using GuiAdventureStackConstPtr = const GuiAdventureStack*;

class GUIGAMEWRAPPERS_EXPORT GuiAdventureSquad : public QObject {
    Q_OBJECT
public:
    GuiAdventureSquad(GuiUnitProvider& unitProvider, Core::AdventureSquadMutablePtr source);

    const GuiAdventureStack* getStack(size_t index) const;
    GuiAdventureStack*       getStack(size_t index);

    int                       getStackCount(size_t index) const;
    Core::LibraryUnitConstPtr getStackUnitLibrary(size_t index) const;

    void clearAll();
    void updateGuiState();
    void emitChanges();
    void externalChange();

    size_t getCount() const;

    void setFormation(bool compact);

    Core::AdventureSquadConstPtr getSource() const { return m_source; }

signals:
    void dataChanged();

private:
    Core::AdventureSquadMutablePtr m_source = nullptr;

    std::deque<GuiAdventureStack> m_stacks;
};

class SpellsModel;
class SkillsModel;
class ArtifactsModel;

class HeroBagEditModel;
class HeroWearingEditModel;
class HeroSpellbookEditModel;
class HeroSkillsEditModel;
class GUIGAMEWRAPPERS_EXPORT GuiAdventureHero : public QObject {
    Q_OBJECT
public:
    GuiAdventureHero(GuiHeroProvider& heroProvider, Core::AdventureHeroMutablePtr source);
    ~GuiAdventureHero();

    void setHero(Core::LibraryHeroConstPtr hero);
    void resetHeroToDefault();

    void createArtifactsModelsIfNeeded(ArtifactsModel* artifacts);
    void refreshArtifactsModels();

    void createSpellsModelsIfNeeded(SpellsModel* spellsModel);
    void refreshSpellsModels();

    void createSkillsModelsIfNeeded(SkillsModel* skillsModel);
    void refreshSkillsModels();

    QString getName() const;
    QString getClassName() const;

    Core::AdventureHeroConstPtr   getSource() const { return m_source; }
    Core::AdventureHeroMutablePtr getSource() { return m_source; }

    GuiHeroConstPtr getGuiHero() const;

    QAbstractItemModel* getBagEditModel() const;
    QAbstractItemModel* getWearingEditModel() const;
    QAbstractItemModel* getSpellbookEditModel() const;
    QAbstractItemModel* getSkillsEditModel() const;

    void updateGuiState();
    void emitChanges();
    void externalChange();

signals:
    void dataChanged();

private:
    GuiHeroProvider&              m_heroProvider;
    Core::AdventureHeroMutablePtr m_source = nullptr;
    GuiHeroConstPtr               m_guiHero;
    HeroBagEditModel*             m_bagEditModel       = nullptr;
    HeroWearingEditModel*         m_wearingEditModel   = nullptr;
    HeroSpellbookEditModel*       m_spellbookEditModel = nullptr;
    HeroSkillsEditModel*          m_skillsEditModel    = nullptr;
};

class GUIGAMEWRAPPERS_EXPORT GuiAdventureArmy : public QObject {
    Q_OBJECT
public:
    GuiAdventureArmy(GuiUnitProvider*              unitProvider,
                     GuiHeroProvider*              heroProvider,
                     Core::AdventureArmyMutablePtr source);

    const GuiAdventureSquad* getSquad() const { return &m_squad; }
    GuiAdventureSquad*       getSquad() { return &m_squad; }

    const GuiAdventureHero* getHero() const { return &m_hero; }
    GuiAdventureHero*       getHero() { return &m_hero; }

    Core::AdventureArmyConstPtr   getSource() const { return m_source; }
    Core::AdventureArmyMutablePtr getSource() { return m_source; }

    void updateGuiState();
    void emitChanges();
    void externalChange();

signals:
    void dataChanged();

private:
    Core::AdventureArmyMutablePtr m_source = nullptr;

    GuiAdventureSquad m_squad;
    GuiAdventureHero  m_hero;
};

}
