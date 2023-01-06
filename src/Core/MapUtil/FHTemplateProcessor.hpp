/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "FHMap.hpp"

#include "FHTemplateZone.hpp"

#include <stdexcept>

namespace FreeHeroes {

class FHTemplateProcessor {
public:
    FHTemplateProcessor(FHMap&                     map,
                        const Core::IGameDatabase* database,
                        Core::IRandomGenerator*    rng,
                        std::ostream&              logOutput);

    enum class Stage
    {
        Invalid,
        ZoneCenterPlacement,
        ZoneTilesInitial,
        ZoneTilesExpand,
        ZoneTilesRefinement,
        TownsPlacement,
        RoadsPlacement,
        Borders,
    };

    void run(const std::string& stopAfterStage);

private:
    void runCurrentStage();
    void runZoneCenterPlacement();
    void runZoneTilesInitial();
    void runZoneTilesExpand();
    void runZoneTilesRefinement();
    void runTownsPlacement();
    void runRoadsPlacement();
    void runBorders();

    void placeTerrainZones();
    void placeDebugInfo();

    Core::LibraryFactionConstPtr getRandomFaction();

private:
    MapCanvas              m_mapCanvas;
    std::vector<TileZone>  m_tileZones;
    std::vector<TileZone*> m_tileZonesPtrs;

    int64_t m_totalRelativeArea = 0;

    Stage m_currentStage = Stage::Invalid;
    Stage m_stopAfter    = Stage::Invalid;

    FHMap&                           m_map;
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::string                      m_indent;
    std::ostream&                    m_logOutput;

    std::vector<Core::LibraryFactionConstPtr> m_playableFactions;
};

}
