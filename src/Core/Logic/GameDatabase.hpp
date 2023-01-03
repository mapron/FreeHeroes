/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"

#include "MernelPlatform/PropertyTree.hpp"

#include <memory>

namespace FreeHeroes::Core {
class GameDatabase : public IGameDatabase {
public:
public:
    GameDatabase(const Mernel::PropertyTree& recordObjectMaps);
    ~GameDatabase();

    LibraryArtifactContainerPtr       artifacts() const override;
    LibraryDwellingContainerPtr       dwellings() const override;
    LibraryFactionContainerPtr        factions() const override;
    LibraryHeroContainerPtr           heroes() const override;
    LibraryHeroSpecContainerPtr       heroSpecs() const override;
    LibraryMapBankContainerPtr        mapBanks() const override;
    LibraryMapObstacleContainerPtr    mapObstacles() const override;
    LibraryMapVisitableContainerPtr   mapVisitables() const override;
    LibraryObjectDefContainerPtr      objectDefs() const override;
    LibraryResourceContainerPtr       resources() const override;
    LibrarySecondarySkillContainerPtr secSkills() const override;
    LibrarySpellContainerPtr          spells() const override;
    LibraryTerrainContainerPtr        terrains() const override;
    LibraryUnitContainerPtr           units() const override;

    LibraryGameRulesConstPtr gameRules() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
