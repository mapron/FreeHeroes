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

namespace Mernel {
class PropertyTree;
}

namespace FreeHeroes::Core {

template<class _Ty, class... _Types>
concept is_any_of = std::disjunction_v<std::is_same<_Ty, _Types>...>;

template<class T>
concept GameDatabaseObject = is_any_of<T,
                                       LibraryArtifact,
                                       LibraryBuilding,
                                       LibraryDwelling,
                                       LibraryFaction,
                                       LibraryGameRules,
                                       LibraryHero,
                                       LibraryHeroSpec,
                                       LibraryMapBank,
                                       LibraryMapObstacle,
                                       LibraryMapVisitable,
                                       LibraryObjectDef,
                                       LibraryPlayer,
                                       LibraryResource,
                                       LibrarySecondarySkill,
                                       LibrarySpell,
                                       LibraryTerrain,
                                       LibraryUnit>;

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
        virtual std::vector<ConstPtr>        legacyOrderedRecords() const      = 0;
    };

    using LibraryArtifactContainerPtr       = const ContainerInterface<LibraryArtifact>*;
    using LibraryBuildingContainerPtr       = const ContainerInterface<LibraryBuilding>*;
    using LibraryDwellingContainerPtr       = const ContainerInterface<LibraryDwelling>*;
    using LibraryFactionContainerPtr        = const ContainerInterface<LibraryFaction>*;
    using LibraryHeroContainerPtr           = const ContainerInterface<LibraryHero>*;
    using LibraryHeroSpecContainerPtr       = const ContainerInterface<LibraryHeroSpec>*;
    using LibraryMapBankContainerPtr        = const ContainerInterface<LibraryMapBank>*;
    using LibraryMapObstacleContainerPtr    = const ContainerInterface<LibraryMapObstacle>*;
    using LibraryMapVisitableContainerPtr   = const ContainerInterface<LibraryMapVisitable>*;
    using LibraryObjectDefContainerPtr      = const ContainerInterface<LibraryObjectDef>*;
    using LibraryPlayerContainerPtr         = const ContainerInterface<LibraryPlayer>*;
    using LibraryResourceContainerPtr       = const ContainerInterface<LibraryResource>*;
    using LibrarySecondarySkillContainerPtr = const ContainerInterface<LibrarySecondarySkill>*;
    using LibrarySpellContainerPtr          = const ContainerInterface<LibrarySpell>*;
    using LibraryTerrainContainerPtr        = const ContainerInterface<LibraryTerrain>*;
    using LibraryUnitContainerPtr           = const ContainerInterface<LibraryUnit>*;

    virtual LibraryArtifactContainerPtr       artifacts() const     = 0;
    virtual LibraryBuildingContainerPtr       buildings() const     = 0;
    virtual LibraryDwellingContainerPtr       dwellings() const     = 0;
    virtual LibraryFactionContainerPtr        factions() const      = 0;
    virtual LibraryHeroContainerPtr           heroes() const        = 0;
    virtual LibraryHeroSpecContainerPtr       heroSpecs() const     = 0;
    virtual LibraryMapBankContainerPtr        mapBanks() const      = 0;
    virtual LibraryMapObstacleContainerPtr    mapObstacles() const  = 0;
    virtual LibraryMapVisitableContainerPtr   mapVisitables() const = 0;
    virtual LibraryObjectDefContainerPtr      objectDefs() const    = 0;
    virtual LibraryPlayerContainerPtr         players() const       = 0;
    virtual LibraryResourceContainerPtr       resources() const     = 0;
    virtual LibrarySecondarySkillContainerPtr secSkills() const     = 0;
    virtual LibrarySpellContainerPtr          spells() const        = 0;
    virtual LibraryTerrainContainerPtr        terrains() const      = 0;
    virtual LibraryUnitContainerPtr           units() const         = 0;

    virtual LibraryGameRulesConstPtr gameRules() const = 0;

    template<typename T>
    inline const ContainerInterface<T>* container() const
    {
        static_assert(sizeof(T) == 3);
        return nullptr;
    }
};
// clang-format off
template <> inline IGameDatabase::LibraryArtifactContainerPtr       IGameDatabase::container() const { return artifacts();}
template <> inline IGameDatabase::LibraryBuildingContainerPtr       IGameDatabase::container() const { return buildings();}
template <> inline IGameDatabase::LibraryDwellingContainerPtr       IGameDatabase::container() const { return dwellings();}
template <> inline IGameDatabase::LibraryFactionContainerPtr        IGameDatabase::container() const { return factions();}
template <> inline IGameDatabase::LibraryHeroContainerPtr           IGameDatabase::container() const { return heroes();}
template <> inline IGameDatabase::LibraryHeroSpecContainerPtr       IGameDatabase::container() const { return heroSpecs();}
template <> inline IGameDatabase::LibraryMapBankContainerPtr        IGameDatabase::container() const { return mapBanks();}
template <> inline IGameDatabase::LibraryMapObstacleContainerPtr    IGameDatabase::container() const { return mapObstacles();}
template <> inline IGameDatabase::LibraryMapVisitableContainerPtr   IGameDatabase::container() const { return mapVisitables();}
template <> inline IGameDatabase::LibraryObjectDefContainerPtr      IGameDatabase::container() const { return objectDefs();}
template <> inline IGameDatabase::LibraryPlayerContainerPtr         IGameDatabase::container() const { return players();}
template <> inline IGameDatabase::LibraryResourceContainerPtr       IGameDatabase::container() const { return resources();}
template <> inline IGameDatabase::LibrarySecondarySkillContainerPtr IGameDatabase::container() const { return secSkills();}
template <> inline IGameDatabase::LibrarySpellContainerPtr          IGameDatabase::container() const { return spells();}
template <> inline IGameDatabase::LibraryTerrainContainerPtr        IGameDatabase::container() const { return terrains();}
template <> inline IGameDatabase::LibraryUnitContainerPtr           IGameDatabase::container() const { return units();}
// clang-format on

class IGameDatabaseContainer {
public:
    virtual ~IGameDatabaseContainer() = default;

    // load these and only these.
    using DbOrder = std::vector<std::string>;

    [[nodiscard]] virtual const IGameDatabase* getDatabase(const DbOrder& dbIndexFilesList, const Mernel::PropertyTree& customSegmentData) const noexcept = 0;

    [[nodiscard]] virtual const IGameDatabase* getDatabase(const DbOrder& dbIndexFilesList) const noexcept = 0;

    [[nodiscard]] virtual const IGameDatabase* getDatabase(GameVersion version) const noexcept
    {
        if (version == GameVersion::SOD)
            return getDatabase({ std::string(g_database_SOD) });
        if (version == GameVersion::HOTA)
            return getDatabase({ std::string(g_database_HOTA) });

        return nullptr;
    }
};

}
