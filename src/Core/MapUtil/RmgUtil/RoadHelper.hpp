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

    void placeRoads(TileZone& tileZone);

    void prepareRoad(TileZone& tileZone, const MapTilePtrList& tileList, RoadLevel level);
    void placeRoad(const MapTilePtrList& tileList, RoadLevel level);
    void placeRoadPath(std::vector<FHPos> path, RoadLevel level);

private:
    MapTileRegion  redundantCleanup(TileZone& tileZone);
    void           correctRoadTypes(TileZone& tileZone, int pass);
    MapTilePtrList aStarPath(TileZone& zone, MapTilePtr start, MapTilePtr end, bool allTiles) const;

private:
    FHMap&                        m_map;
    MapTileContainer&             m_tileContainer;
    Core::IRandomGenerator* const m_rng;
    std::ostream&                 m_logOutput;
};

}
