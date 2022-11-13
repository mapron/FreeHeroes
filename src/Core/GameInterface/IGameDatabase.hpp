/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "GameConstants.hpp"

#include <concepts>
#include <memory>
#include <string>
#include <vector>

namespace FreeHeroes::Core {

template<class _Ty, class... _Types>
concept is_any_of = std::disjunction_v<std::is_same<_Ty, _Types>...>;

template<class T>
concept GameDatabaseObject = is_any_of<T,
                                       LibraryArtifact,
                                       LibraryUnit,
                                       LibraryFaction,
                                       LibraryHeroSpec,
                                       LibraryHero,
                                       LibrarySecondarySkill,
                                       LibrarySpell,
                                       LibraryTerrain,
                                       LibraryResource,
                                       LibraryMapObject,
                                       LibraryGameRules>;

class IGameDatabase {
public:
    virtual ~IGameDatabase() = default;

    template<typename T>
    class ContainerInterface {
        using Ptr      = T*;
        using ConstPtr = const T*;

    public:
        virtual ~ContainerInterface()                                          = default;
        virtual ConstPtr                     find(const std::string& id) const = 0;
        virtual const std::vector<ConstPtr>& records() const                   = 0;
        virtual std::vector<std::string>     legacyOrderedIds() const          = 0;
    };

    using LibraryUnitContainerPtr           = const ContainerInterface<LibraryUnit>*;
    using LibraryHeroContainerPtr           = const ContainerInterface<LibraryHero>*;
    using LibraryArtifactContainerPtr       = const ContainerInterface<LibraryArtifact>*;
    using LibrarySecondarySkillContainerPtr = const ContainerInterface<LibrarySecondarySkill>*;
    using LibraryFactionContainerPtr        = const ContainerInterface<LibraryFaction>*;
    using LibrarySpellContainerPtr          = const ContainerInterface<LibrarySpell>*;
    using LibraryResourceContainerPtr       = const ContainerInterface<LibraryResource>*;
    using LibraryTerrainContainerPtr        = const ContainerInterface<LibraryTerrain>*;
    using LibraryMapObjectContainerPtr      = const ContainerInterface<LibraryMapObject>*;
    using LibraryHeroSpecContainerPtr       = const ContainerInterface<LibraryHeroSpec>*;

    virtual LibraryUnitContainerPtr           units() const      = 0;
    virtual LibraryHeroContainerPtr           heroes() const     = 0;
    virtual LibraryArtifactContainerPtr       artifacts() const  = 0;
    virtual LibrarySecondarySkillContainerPtr secSkills() const  = 0;
    virtual LibraryFactionContainerPtr        factions() const   = 0;
    virtual LibrarySpellContainerPtr          spells() const     = 0;
    virtual LibraryResourceContainerPtr       resources() const  = 0;
    virtual LibraryTerrainContainerPtr        terrains() const   = 0;
    virtual LibraryMapObjectContainerPtr      mapObjects() const = 0;
    virtual LibraryHeroSpecContainerPtr       heroSpecs() const  = 0;

    virtual LibraryGameRulesConstPtr gameRules() const = 0;

    template<typename T>
    inline const ContainerInterface<T>* container() const
    {
        static_assert(sizeof(T) == 3);
        return nullptr;
    }
};
// clang-format off
template <> inline IGameDatabase::LibraryUnitContainerPtr           IGameDatabase::container() const { return units();}
template <> inline IGameDatabase::LibraryHeroContainerPtr           IGameDatabase::container() const { return heroes();}
template <> inline IGameDatabase::LibraryArtifactContainerPtr       IGameDatabase::container() const { return artifacts();}
template <> inline IGameDatabase::LibrarySecondarySkillContainerPtr IGameDatabase::container() const { return secSkills();}
template <> inline IGameDatabase::LibraryFactionContainerPtr        IGameDatabase::container() const { return factions();}
template <> inline IGameDatabase::LibrarySpellContainerPtr          IGameDatabase::container() const { return spells();}
template <> inline IGameDatabase::LibraryResourceContainerPtr       IGameDatabase::container() const { return resources();}
template <> inline IGameDatabase::LibraryTerrainContainerPtr        IGameDatabase::container() const { return terrains();}
template <> inline IGameDatabase::LibraryMapObjectContainerPtr      IGameDatabase::container() const { return mapObjects();}
template <> inline IGameDatabase::LibraryHeroSpecContainerPtr       IGameDatabase::container() const { return heroSpecs();}
// clang-format on

class IGameDatabaseContainer {
public:
    virtual ~IGameDatabaseContainer() = default;

    [[nodiscard]] virtual const IGameDatabase* getDatabase(GameVersion version) const = 0;
    [[nodiscard]] virtual const IGameDatabase* getDatabase(const std::string& version) const
    {
        if (version == g_database_HOTA)
            return getDatabase(GameVersion::HOTA);
        if (version == g_database_SOD)
            return getDatabase(GameVersion::SOD);
        return nullptr;
    }
};

}
