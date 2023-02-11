/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMapObject.hpp"

#include "LibraryHero.hpp"

namespace FreeHeroes {

struct FHRngZoneTown {
    FHTown m_town;
    bool   m_playerControlled = false;
    bool   m_useZoneFaction   = false;
};

struct FHScoreSettings {
    struct ScoreScope {
        int64_t m_target    = 0;
        int64_t m_minSingle = -1;
        int64_t m_maxSingle = -1;
    };

    using AttrMap = std::map<FHScoreAttr, ScoreScope>;

    AttrMap m_guarded;
    AttrMap m_unguarded;
    int     m_armyFocusPercent = 80;

    bool empty() const { return m_guarded.empty() && m_unguarded.empty(); }

    MAPUTIL_EXPORT static std::string attrToString(FHScoreAttr attr);
};

struct FHRngZone {
    Core::LibraryPlayerConstPtr  m_player          = nullptr;
    Core::LibraryFactionConstPtr m_mainTownFaction = nullptr;
    Core::LibraryFactionConstPtr m_rewardsFaction  = nullptr;
    Core::LibraryTerrainConstPtr m_terrain         = nullptr;

    std::vector<FHRngZoneTown> m_towns;
    FHPos                      m_centerAvg;
    FHPos                      m_centerDispersion;
    int                        m_relativeSizeAvg        = 100;
    int                        m_relativeSizeDispersion = 0;

    FHScoreSettings m_score;

    int64_t m_guardMin = 0;
    int64_t m_guardMax = 0;

    int m_cornerRoads = 0;

    bool m_isNormal = false;
};
struct FHRngConnection {
    std::string m_from;
    std::string m_to;

    std::string m_mirrorGuard;
    int64_t     m_guard = 0;
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
        bool                         m_stdStats        = false;

        bool operator==(const UserPlayer&) const noexcept = default;
    };
    using PlayersMap = std::map<Core::LibraryPlayerConstPtr, UserPlayer>;

    struct DifficultySettings {
        int m_minGuardsPercent = 100;
        int m_maxGuardsPercent = 100;

        int m_minArmyPercent = 100;
        int m_maxArmyPercent = 100;

        int m_minGoldPercent = 100;
        int m_maxGoldPercent = 100;

        bool operator==(const DifficultySettings&) const noexcept = default;
    };

    PlayersMap         m_players;
    DifficultySettings m_difficulty;

    FHRoadType m_defaultRoad     = FHRoadType::Cobblestone;
    int        m_difficultyScale = 100;
    int        m_mapSize         = 144;
    bool       m_hasUnderground  = false;
};

struct FHTemplate {
    using RngZoneMap       = std::map<std::string, FHRngZone>;
    using RngConnectionMap = std::map<std::string, FHRngConnection>;

    RngZoneMap        m_zones;
    RngConnectionMap  m_connections;
    FHRngUserSettings m_userSettings;

    int  m_roughTilePercentage      = 12;
    int  m_rotationDegreeDispersion = 0;
    bool m_allowFlip                = false;

    Core::HeroSkillsList m_stdSkillsWarrior;
    Core::HeroSkillsList m_stdSkillsMage;

    void rescaleToSize(int newMapSize, int oldWidth, int oldHeight);
};

}
