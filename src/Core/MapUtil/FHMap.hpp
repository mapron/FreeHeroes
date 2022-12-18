/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <compare>
#include <set>
#include <optional>

#include "PropertyTree.hpp"
#include "GameConstants.hpp"

#include "AdventureArmy.hpp"

#include "FHTileMap.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

enum class FHPlayerId
{
    Invalid = -2,
    None    = -1,
    Red     = 0,
    Blue,
    Tan,
    Green,
    Orange,
    Purple,
    Teal,
    Pink,
};

struct FHPlayer {
    bool m_humanPossible{ false };
    bool m_aiPossible{ false };

    std::vector<Core::LibraryFactionConstPtr> m_startingFactions;
};

struct FHCommonObject {
    FHPos m_pos{ g_invalidPos };
    int   m_order = 0;
};

struct FHPlayerControlledObject : public FHCommonObject {
    FHPlayerId m_player = FHPlayerId::Invalid;
};

struct FHHeroData {
    bool m_hasExp        = false;
    bool m_hasSecSkills  = false;
    bool m_hasPrimSkills = false;
    bool m_hasCustomBio  = false;
    bool m_hasSpells     = false;

    Core::AdventureArmy m_army;
};

struct FHHero : public FHPlayerControlledObject {
    bool                      m_isMain{ false };
    Core::LibraryHeroConstPtr m_id              = nullptr;
    uint32_t                  m_questIdentifier = 0;
};

struct FHTown : public FHPlayerControlledObject {
    bool                         m_isMain{ false };
    Core::LibraryFactionConstPtr m_factionId = nullptr;
    bool                         m_hasFort{ false };
    uint32_t                     m_questIdentifier = 0;
    bool                         m_spellResearch{ false };
    std::string                  m_defFile; // just for the sake of roundtrip tests.
};

struct FHDwelling : public FHPlayerControlledObject {
    Core::LibraryDwellingConstPtr m_id         = nullptr;
    int                           m_defVariant = 0;
};

struct FHResource : public FHCommonObject {
    enum class Type
    {
        Resource,
        TreasureChest,
    };
    uint32_t                      m_amount = 0;
    Core::LibraryResourceConstPtr m_id     = nullptr;
    Type                          m_type   = Type::Resource;
};

struct FHArtifact : public FHCommonObject {
    Core::LibraryArtifactConstPtr m_id = nullptr;
};

struct FHMonster : public FHCommonObject {
    Core::LibraryUnitConstPtr m_id    = nullptr;
    uint32_t                  m_count = 0;

    int m_agressionMin = 1;
    int m_agressionMax = 10;

    uint32_t m_questIdentifier = 0;
};

struct FHBank : public FHCommonObject {
    Core::LibraryMapBankConstPtr m_id         = nullptr;
    int                          m_defVariant = 0;
    std::vector<int>             m_guardsVariants; // empty = full random
};

struct FHObstacle : public FHCommonObject {
    Core::LibraryMapObstacleConstPtr m_id = nullptr;
};

struct FHMap {
    using PlayersMap = std::map<FHPlayerId, FHPlayer>;

    Core::GameVersion m_version = Core::GameVersion::Invalid;
    uint64_t          m_seed{ 0 };

    FHTileMap m_tileMap;

    std::string m_name;
    std::string m_descr;
    uint8_t     m_difficulty = 0;

    PlayersMap          m_players;
    std::vector<FHHero> m_wanderingHeroes;
    std::vector<FHTown> m_towns;
    std::vector<FHZone> m_zones;

    struct Objects {
        std::vector<FHResource> m_resources;
        std::vector<FHArtifact> m_artifacts;
        std::vector<FHMonster>  m_monsters;
        std::vector<FHDwelling> m_dwellings;
        std::vector<FHBank>     m_banks;
        std::vector<FHObstacle> m_obstacles;
    } m_objects;

    std::vector<FHRiver> m_rivers;
    std::vector<FHRoad>  m_roads;

    Core::LibraryTerrainConstPtr m_defaultTerrain = nullptr;

    std::vector<Core::LibraryHeroConstPtr>           m_disabledHeroes;
    std::vector<Core::LibraryArtifactConstPtr>       m_disabledArtifacts;
    std::vector<Core::LibrarySpellConstPtr>          m_disabledSpells;
    std::vector<Core::LibrarySecondarySkillConstPtr> m_disabledSkills;

    std::vector<FHHeroData> m_customHeroes;

    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data, const Core::IGameDatabase* database);
};

}
