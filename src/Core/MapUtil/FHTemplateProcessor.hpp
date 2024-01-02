/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "FHMap.hpp"

#include "RmgUtil/MapGuard.hpp"
#include "RmgUtil/TileZone.hpp"

#include "MapUtilExport.hpp"

#include <stdexcept>
#include <functional>

namespace FreeHeroes {

class MAPUTIL_EXPORT FHTemplateProcessor {
public:
    FHTemplateProcessor(FHMap&                  map,
                        Core::IRandomGenerator* rng,
                        std::ostream&           logOutput,
                        const std::string&      stopAfterStage,
                        const std::string&      debugStage,
                        const std::string&      tileZoneFilter,
                        int                     stopAfterHeat,
                        bool                    extraLogs);

    enum class Stage
    {
        Invalid,
        ZoneCenterPlacement,
        ZoneTilesInitial,
        BorderRoads,
        TownsPlacement,
        CellSegmentation,
        RoadsPlacement,
        SegmentationRefinement,
        HeatMap,
        Rewards,
        CorrectObjectTerrains,
        Obstacles,
        Guards,
        PlayerInfo,
    };
    static std::string stageToString(Stage stage);

    void run();

private:
    void runCurrentStage();
    void runZoneCenterPlacement();
    void runZoneTilesInitial();
    void runBorderRoads();
    void runTownsPlacement();
    void runCellSegmentation();
    void runRoadsPlacement();
    void runSegmentationRefinement();
    void runHeatMap();
    void runRewards();
    void runCorrectObjectTerrains();
    void runObstacles();
    void runGuards();
    void runPlayerInfo();

    void placeTerrainZones();
    void placeDebugInfo();

    Core::LibraryFactionConstPtr           getRandomFaction(bool rewardOnly);
    Core::LibraryFactionConstPtr           getRandomPlayableFaction(const std::set<std::string>& excludedZoneIds);
    Core::LibraryHeroConstPtr              getRandomHero(Core::LibraryFactionConstPtr faction);
    int                                    getPossibleCount(Core::LibraryUnitConstPtr unit, int64_t value) const;
    std::set<Core::LibraryFactionConstPtr> getExcludedFactions(const std::set<std::string>& zoneIds) const;
    bool                                   isFilteredOut(const TileZone& tileZone) const;

private:
    using HeroGeneration = FHRngUserSettings::HeroGeneration;
    struct PlayerInfo {
        Core::LibraryFactionConstPtr m_faction = nullptr;

        bool   m_hasMainTown      = false;
        size_t m_mainTownMapIndex = 0;
        int    m_team             = -1;

        Core::LibraryHeroConstPtr m_startingHero    = nullptr;
        Core::LibraryHeroConstPtr m_extraHero       = nullptr;
        HeroGeneration            m_startingHeroGen = HeroGeneration::RandomStartingFaction;
        HeroGeneration            m_extraHeroGen    = HeroGeneration::None;
    };
    struct CmpPlayers {
        bool operator()(Core::LibraryPlayerConstPtr a, Core::LibraryPlayerConstPtr b) const;
    };

private:
    FHMap&                           m_map;
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::ostream&                    m_logOutput;
    const Stage                      m_stopAfter = Stage::Invalid;
    const Stage                      m_showDebug = Stage::Invalid;
    const std::string                m_tileZoneFilter;
    const int                        m_stopAfterHeat;
    const bool                       m_extraLogging;

private:
    MapTileContainer       m_tileContainer;
    std::vector<TileZone>  m_tileZones;
    std::vector<TileZone*> m_tileZonesPtrs;

    Stage m_currentStage  = Stage::Invalid;
    bool  m_terrainPlaced = false;

    std::string m_indent;

    std::vector<Core::LibraryFactionConstPtr> m_playableFactions;
    std::vector<Core::LibraryFactionConstPtr> m_rewardFactions;
    std::vector<Core::LibraryUnitConstPtr>    m_guardUnits;

    MapGuardList m_guards;
    int64_t      m_userMultiplyGuard = 100;

    std::map<Core::LibraryPlayerConstPtr, PlayerInfo, CmpPlayers> m_playerInfo;

    std::set<Core::LibraryHeroConstPtr> m_heroPool;
};

}
