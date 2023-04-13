/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TileZone.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHMap;
class RoadHelper {
public:
    RoadHelper(FHMap&                        map,
               MapTileContainer&             tileContainer,
               Core::IRandomGenerator* const rng,
               std::ostream&                 logOutput);

    void makeBorders(std::vector<TileZone>& tileZones);

    void placeRoads(TileZone& tileZone);

    void placeRoad(TileZone& tileZone, const MapTilePtrList& tileList, int level);
    void placeRoad(const MapTilePtrList& tileList, int level);
    void placeRoadPath(std::vector<FHPos> path, int level);

public:
    struct Guard {
        int64_t     m_value = 0;
        std::string m_id;
        std::string m_mirrorFromId;
        MapTilePtr  m_pos      = nullptr;
        TileZone*   m_zone     = nullptr;
        bool        m_joinable = false;
    };

    std::vector<Guard> m_guards;

private:
    MapTileRegion  redundantCleanup(TileZone& tileZone);
    MapTilePtrList aStarPath(TileZone& zone, MapTilePtr start, MapTilePtr end, bool allTiles);

private:
    FHMap&                        m_map;
    MapTileContainer&             m_tileContainer;
    Core::IRandomGenerator* const m_rng;
    std::ostream&                 m_logOutput;
};

}
