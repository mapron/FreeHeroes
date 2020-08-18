/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"

#include <memory>

#include "CoreLogicExport.hpp"

namespace FreeHeroes::Core {

class IResourceLibrary;
class CORELOGIC_EXPORT GameDatabase : public IGameDatabase
{
public:
    GameDatabase(const std::string & dataBaseId, const Core::IResourceLibrary & resourceLibrary);
    ~GameDatabase();


   LibraryUnitContainerPtr           units() const override;
   LibraryHeroContainerPtr           heroes() const override;
   LibraryArtifactContainerPtr       artifacts() const override;
   LibrarySecondarySkillContainerPtr secSkills() const override;
   LibraryFactionContainerPtr        factions() const override;
   LibrarySpellContainerPtr          spells() const override;
   LibraryResourceContainerPtr       resources() const override;
   LibraryTerrainContainerPtr        terrains() const override;
   LibraryMapObjectContainerPtr      mapObjects() const override;
   LibraryHeroSpecContainerPtr       heroSpecs() const override;

   LibraryGameRulesConstPtr          gameRules() const override;

private:
    bool load(const std::vector<std_path> & files);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
