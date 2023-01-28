/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <compare>
#include <set>
#include <optional>

#include "MernelPlatform/PropertyTree.hpp"
#include "GameConstants.hpp"

#include "AdventureArmy.hpp"
#include "Reward.hpp"

#include "LibraryObjectDef.hpp"
#include "LibrarySecondarySkill.hpp"

#include "FHTileMap.hpp"

#include "MapUtilExport.hpp"

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

    bool m_generateHeroAtMainTown{ false };

    std::vector<Core::LibraryFactionConstPtr> m_startingFactions;
};

struct FHCommonObject {
    FHPos                m_pos{ g_invalidPos };
    int                  m_order = 0;
    Core::ObjectDefIndex m_defIndex;
};

struct FHCommonVisitable : public FHCommonObject {
    Core::LibraryMapVisitableConstPtr m_visitableId = nullptr;
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
    bool       m_isMain{ false };
    FHHeroData m_data;

    uint32_t m_questIdentifier = 0;
};

struct FHTown : public FHPlayerControlledObject {
    bool                         m_isMain{ false };
    Core::LibraryFactionConstPtr m_factionId = nullptr;
    bool                         m_hasFort{ false };
    uint32_t                     m_questIdentifier = 0;
    bool                         m_spellResearch{ false };
};

struct FHDwelling : public FHPlayerControlledObject {
    Core::LibraryDwellingConstPtr m_id = nullptr;
};

struct FHMine : public FHPlayerControlledObject {
    Core::LibraryResourceConstPtr m_id = nullptr;
};

struct FHResource : public FHCommonObject {
    enum class Type
    {
        Resource,
        TreasureChest,
        CampFire,
    };
    uint32_t                      m_amount = 0;
    Core::LibraryResourceConstPtr m_id     = nullptr;
    Type                          m_type   = Type::Resource;

    Core::LibraryMapVisitableConstPtr m_visitableId = nullptr;
};

struct FHRandomResource : public FHCommonObject {
    uint32_t m_amount = 0;
};

struct FHArtifact : public FHCommonObject {
    Core::LibraryArtifactConstPtr m_id = nullptr;
};

struct FHRandomArtifact : public FHCommonObject {
    enum class Type
    {
        Invalid,
        Any,
        Treasure,
        Minor,
        Major,
        Relic,
    };

    Type m_type = Type::Invalid;
};

struct FHPandora : public FHCommonObject {
    Core::Reward m_reward;
};

struct FHMonster : public FHCommonObject {
    Core::LibraryUnitConstPtr m_id    = nullptr;
    uint32_t                  m_count = 0;

    int m_agressionMin = 1;
    int m_agressionMax = 10;

    bool m_joinOnlyForMoney = false;
    int  m_joinPercent      = 100;

    uint32_t m_questIdentifier = 0;
    int64_t  m_guardValue      = 0;

    enum class UpgradedStack
    {
        Invalid,
        Random,
        Yes,
        No
    };

    UpgradedStack m_upgradedStack = UpgradedStack::Random;
};

struct FHBank : public FHCommonObject {
    Core::LibraryMapBankConstPtr m_id = nullptr;
    enum class UpgradedStack
    {
        Invalid,
        Random,
        Yes,
        No
    };

    UpgradedStack m_upgradedStack = UpgradedStack::Random;
    int           m_guardsVariant = -1; // -1 = full random

    std::vector<Core::LibraryArtifactConstPtr> m_artifacts; // empty = full random
};

struct FHObstacle : public FHCommonObject {
    Core::LibraryMapObstacleConstPtr m_id = nullptr;
};
struct FHVisitable : public FHCommonVisitable {
};

struct FHShrine : public FHCommonVisitable {
    Core::LibrarySpellConstPtr m_spellId     = nullptr;
    int                        m_randomLevel = -1;
};

struct FHSkillHut : public FHCommonVisitable {
    std::vector<Core::LibrarySecondarySkillConstPtr> m_skillIds;
};
struct FHQuest {
    enum class Type
    {
        Invalid      = 0,
        GetHeroLevel = 1,
        GetPrimaryStat,
        KillHero,
        KillCreature,
        BringArtifacts,
        BringCreatures,
        BringResource,
        BeHero,
        BePlayer,
    };
    Type m_type = Type::Invalid;

    std::vector<Core::LibraryArtifactConstPtr> m_artifacts;
    std::vector<Core::UnitWithCount>           m_units;
    Core::ResourceAmount                       m_resources;
    Core::HeroPrimaryParams                    m_primary;
    int                                        m_level = 0;

    uint32_t m_targetQuestId = 0;
};

struct FHQuestHut : public FHCommonVisitable {
    Core::Reward m_reward;
    FHQuest      m_quest;
};

struct FHScholar : public FHCommonVisitable {
    enum Type
    {
        Primary,
        Secondary,
        Spell,
        Random,
    };
    Type m_type = Type::Random;

    Core::HeroPrimaryParamType          m_primaryType = Core::HeroPrimaryParamType::Attack;
    Core::LibrarySecondarySkillConstPtr m_skillId     = nullptr;
    Core::LibrarySpellConstPtr          m_spellId     = nullptr;
};
struct FHRngZoneTown {
    FHTown m_town;
    bool   m_playerControlled = false;
    bool   m_useZoneFaction   = false;
};

struct FHScoreSettings {
    enum class Attr
    {
        Invalid,

        Army,
        ArtStat,
        ArtSupport,
        Gold,
        Resource,
        ResourceGen,
        Experience,
        SpellOffensive,
        SpellCommon,
        SpellAll,
        Misc,
    };
    struct ScoreScope {
        int64_t m_target    = 0;
        int64_t m_minSingle = -1;
        int64_t m_maxSingle = -1;
    };

    using AttrMap = std::map<Attr, ScoreScope>;

    AttrMap m_guarded;
    AttrMap m_unguarded;
    int     m_armyFocusPercent = 80;
};

struct FHRngZone {
    FHPlayerId                   m_player          = FHPlayerId::Invalid;
    Core::LibraryFactionConstPtr m_mainTownFaction = nullptr;
    Core::LibraryFactionConstPtr m_rewardsFaction  = nullptr;
    Core::LibraryTerrainConstPtr m_terrain         = nullptr;

    std::vector<FHRngZoneTown> m_towns;
    FHPos                      m_centerAvg;
    FHPos                      m_centerDispersion;
    int                        m_relativeSizeAvg        = 100;
    int                        m_relativeSizeDispersion = 0;

    FHScoreSettings m_score;

    int m_cornerRoads = 0;

    bool m_isNormal = false;
};
struct FHRngConnection {
    std::string m_from;
    std::string m_to;

    std::string m_mirrorGuard;
    int64_t     m_guard = 0;
};

struct FHDebugTile {
    FHPos m_pos;
    int   m_valueA = 0;
    int   m_valueB = 0;
    int   m_valueC = 0;
};

struct FHRngOptions {
    int  m_roughTilePercentage      = 12;
    int  m_rotationDegreeDispersion = 0;
    bool m_allowFlip                = false;
};

struct FHRngUserSettings {
    enum class HeroGeneration
    {
        None,
        RandomAnyFaction,
        RandomStartingFaction,
        FixedAny,
        FixedStarting,
    };

    struct UserPlayer {
        Core::LibraryFactionConstPtr m_faction         = nullptr;
        Core::LibraryHeroConstPtr    m_startingHero    = nullptr;
        Core::LibraryHeroConstPtr    m_extraHero       = nullptr;
        HeroGeneration               m_startingHeroGen = HeroGeneration::RandomStartingFaction;
        HeroGeneration               m_extraHeroGen    = HeroGeneration::None;
    };
    using PlayersMap = std::map<FHPlayerId, UserPlayer>;

    PlayersMap m_players;

    FHRoadType m_defaultRoad     = FHRoadType::Invalid;
    int        m_difficultyScale = 100;
    int        m_mapSize         = 144;
};

struct MAPUTIL_EXPORT FHMap {
    using PlayersMap       = std::map<FHPlayerId, FHPlayer>;
    using DefMap           = std::map<Core::LibraryObjectDefConstPtr, Core::LibraryObjectDef>;
    using RngZoneMap       = std::map<std::string, FHRngZone>;
    using RngConnectionMap = std::map<std::string, FHRngConnection>;

    Core::GameVersion m_version = Core::GameVersion::Invalid;
    uint64_t          m_seed{ 0 };

    FHTileMap m_tileMap;
    bool      m_tileMapUpdateRequired = true;

    std::string m_name;
    std::string m_descr;
    uint8_t     m_difficulty = 0;
    bool        m_isWaterMap = false;

    PlayersMap               m_players;
    std::vector<FHHero>      m_wanderingHeroes;
    std::vector<FHTown>      m_towns;
    std::vector<FHZone>      m_zones;
    std::vector<FHDebugTile> m_debugTiles;
    RngZoneMap               m_rngZones;
    RngConnectionMap         m_rngConnections;
    FHRngOptions             m_rngOptions;
    FHRngUserSettings        m_rngUserSettings;

    struct Objects {
        std::vector<FHResource>       m_resources;
        std::vector<FHRandomResource> m_resourcesRandom;
        std::vector<FHArtifact>       m_artifacts;
        std::vector<FHRandomArtifact> m_artifactsRandom;
        std::vector<FHMonster>        m_monsters;
        std::vector<FHDwelling>       m_dwellings;
        std::vector<FHBank>           m_banks;
        std::vector<FHObstacle>       m_obstacles;
        std::vector<FHVisitable>      m_visitables;
        std::vector<FHMine>           m_mines;
        std::vector<FHPandora>        m_pandoras;
        std::vector<FHShrine>         m_shrines;
        std::vector<FHSkillHut>       m_skillHuts;
        std::vector<FHScholar>        m_scholars;
        std::vector<FHQuestHut>       m_questHuts;
    } m_objects;

    struct Config {
        bool m_allowSpecialWeeks = true;
        bool m_hasRoundLimit     = false;
        int  m_roundLimit        = 100;
    } m_config;

    std::vector<FHRiver> m_rivers;
    std::vector<FHRoad>  m_roads;

    Core::LibraryTerrainConstPtr m_defaultTerrain = nullptr;

    template<class Ptr>
    struct DisableConfig {
        using Map = std::map<Ptr, bool>;
        Map m_data;

        bool isDisabled(bool isWater, Ptr obj) const
        {
            if (m_data.contains(obj))
                return m_data.at(obj);

            if (!isWater && obj->isWaterContent)
                return true;
            return !obj->isEnabledByDefault;
        }

        void setDisabled(bool isWater, Ptr obj, bool state)
        {
            if (!obj)
                return;

            if (state) {
                if (!obj->isEnabledByDefault) // if object is disabled by default = we don't add to the disabled, it's excess.
                    return;

                if (!isWater && obj->isWaterContent) // if object is for water map, and we have non-water map = we don't add to the disabled, it's excess.
                    return;

                m_data[obj] = true;
            } else {
                if (!isWater && !obj->isWaterContent && obj->isEnabledByDefault) // if object is for regular map, and we have non-water map = we don't add to the enabled, it's excess.
                    return;
                m_data[obj] = false;
            }
        }
    };

    using DisableConfigHeroes          = DisableConfig<Core::LibraryHeroConstPtr>;
    using DisableConfigArtifacts       = DisableConfig<Core::LibraryArtifactConstPtr>;
    using DisableConfigSpells          = DisableConfig<Core::LibrarySpellConstPtr>;
    using DisableConfigSecondarySkills = DisableConfig<Core::LibrarySecondarySkillConstPtr>;
    using DisableConfigBanks           = DisableConfig<Core::LibraryMapBankConstPtr>;

    DisableConfigHeroes          m_disabledHeroes;
    DisableConfigArtifacts       m_disabledArtifacts;
    DisableConfigSpells          m_disabledSpells;
    DisableConfigSecondarySkills m_disabledSkills;
    DisableConfigBanks           m_disabledBanks;

    std::vector<FHHeroData> m_customHeroes;

    std::vector<Core::LibraryObjectDefConstPtr> m_initialObjectDefs; // mostly for round-trip.
    DefMap                                      m_defReplacements;   // mostly for round-trip.

    void toJson(Mernel::PropertyTree& data) const;
    void fromJson(Mernel::PropertyTree data, const Core::IGameDatabase* database);

    void applyRngUserSettings(const Mernel::PropertyTree& data, const Core::IGameDatabase* database);

    void initTiles(const Core::IGameDatabase* database);

    void rescaleToSize(int mapSize);
};

}
