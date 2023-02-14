/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMapObject.hpp"

#include "LibraryHero.hpp"
#include "LibraryArtifact.hpp"
#include "LibrarySpell.hpp"

#include "Reward.hpp"

namespace FreeHeroes {

struct FHRngZoneTown {
    FHTown m_town;
    bool   m_playerControlled = false;
    bool   m_useZoneFaction   = false;

    bool operator==(const FHRngZoneTown&) const noexcept = default;
};

struct FHScoreSettings {
    struct ScoreScope {
        int64_t m_target    = 0;
        int64_t m_minSingle = -1;
        int64_t m_maxSingle = -1;

        bool operator==(const ScoreScope&) const noexcept = default;
    };

    using AttrMap = std::map<Core::ScoreAttr, ScoreScope>;

    AttrMap m_score;
    bool    m_isEnabled{ false };
    int     m_guardPercent     = 100;
    int     m_tolerancePercent = 5;

    bool isValidValue(Core::ScoreAttr attr, int64_t value) const noexcept
    {
        auto it = m_score.find(attr);
        if (it == m_score.cend())
            return false;

        const ScoreScope& scope = it->second;

        auto minVal = scope.m_minSingle;
        auto maxVal = scope.m_maxSingle;
        if (minVal != -1 && value < minVal)
            return false;
        if (maxVal != -1 && value > maxVal)
            return false;

        return true;
    }

    bool isValidScore(const Core::MapScore& score) const noexcept
    {
        if (score.empty())
            return true;

        for (const auto& [attr, value] : score) {
            if (!value)
                continue;
            const bool isValid = isValidValue(attr, value);
            if (isValid)
                return true;
        }

        return false;
    }

    bool operator==(const FHScoreSettings&) const noexcept = default;

    MAPUTIL_EXPORT static std::string attrToString(Core::ScoreAttr attr);
};

struct FHRngZone {
    Core::LibraryPlayerConstPtr  m_player          = nullptr;
    Core::LibraryFactionConstPtr m_mainTownFaction = nullptr;
    Core::LibraryFactionConstPtr m_rewardsFaction  = nullptr;
    Core::LibraryTerrainConstPtr m_terrain         = nullptr;

    std::vector<FHRngZoneTown> m_towns;
    FHPos                      m_centerAvg;
    FHPos                      m_centerDispersion;

    int m_relativeSizeAvg        = 100;
    int m_relativeSizeDispersion = 0;
    int m_zoneGuardPercent       = 100;
    int m_zoneGuardDispersion    = 5;

    using ScoreMap = std::map<std::string, FHScoreSettings>;

    ScoreMap m_scoreTargets;

    struct GeneratorCommon {
        bool m_isEnabled = false;

        bool operator==(const GeneratorCommon&) const noexcept = default;
    };
    struct GeneratorBank : public GeneratorCommon {
        struct Record {
            Core::LibraryMapBankConstPtr m_bank      = nullptr;
            int                          m_frequency = -1;
            int                          m_guard     = -1;
            bool                         m_enabled   = true;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorBank&) const noexcept = default;
    };
    struct GeneratorArtifact : public GeneratorCommon {
        struct Record {
            Core::ArtifactFilter m_filter;
            int                  m_frequency = 1000;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorArtifact&) const noexcept = default;
    };
    struct GeneratorResourcePile : public GeneratorCommon {
        struct Record {
            std::vector<int>              m_amounts;
            Core::LibraryResourceConstPtr m_resource  = nullptr;
            int                           m_frequency = 1000;
            int                           m_guard     = 0;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorResourcePile&) const noexcept = default;
    };
    struct GeneratorPandora : public GeneratorCommon {
        struct Record {
            Core::Reward m_reward;
            int          m_frequency = 1000;
            int          m_guard     = -1;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorPandora&) const noexcept = default;
    };

    struct GeneratorShrine : public GeneratorCommon {
        struct Record {
            Core::SpellFilter m_filter;
            int               m_visualLevel = 1;
            int               m_frequency   = 1000;
            int               m_guard       = -1;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorShrine&) const noexcept = default;
    };
    struct GeneratorScroll : public GeneratorCommon {
        struct Record {
            Core::SpellFilter m_filter;
            int               m_frequency = 1000;
            int               m_guard     = -1;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorScroll&) const noexcept = default;
    };
    struct GeneratorDwelling : public GeneratorCommon {
        struct Record {
            int m_level     = 0;
            int m_weeks     = 1;
            int m_castles   = 0;
            int m_frequency = 1000;
            int m_guard     = -1;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorDwelling&) const noexcept = default;
    };

    struct GeneratorVisitable : public GeneratorCommon {
        bool operator==(const GeneratorVisitable&) const noexcept = default;
    };

    struct GeneratorMine : public GeneratorCommon {
        struct Record {
            Core::LibraryResourceConstPtr m_resourceId = nullptr;

            int m_frequency = 1000;
            int m_guard     = 1000;
            int m_value     = 500;

            bool operator==(const Record&) const noexcept = default;
        };
        using Map = std::map<std::string, Record>;
        Map m_records;

        bool operator==(const GeneratorMine&) const noexcept = default;
    };

    struct Generators {
        GeneratorBank         m_banks;
        GeneratorArtifact     m_artifacts;
        GeneratorResourcePile m_resources;
        GeneratorPandora      m_pandoras;
        GeneratorShrine       m_shrines;
        GeneratorScroll       m_scrolls;
        GeneratorDwelling     m_dwellings;
        GeneratorVisitable    m_visitables;
        GeneratorMine         m_mines;

        bool operator==(const Generators&) const noexcept = default;
    };

    Generators m_generators;

    int64_t m_guardMin   = 0;
    int64_t m_guardMax   = 0;
    int64_t m_guardBlock = 0;

    int m_segmentAreaSize = 250;

    bool m_isNormal = false;

    bool operator==(const FHRngZone&) const noexcept = default;
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
