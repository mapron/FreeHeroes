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
               std::ostream&                 logOutput,
               bool                          extraLogging);

    void placeRoads(TileZone& tileZone);
    void placeRoad(const MapTileRegion& region, RoadLevel level);

private:
    void prepareRoad(TileZone& tileZone, const MapTilePtrList& tileList, RoadLevel level);
    void placeRoadPath(std::vector<FHPos> path, RoadLevel level);

    bool           redundantCleanup(TileZone& tileZone);
    MapTilePtrList aStarPath(TileZone& zone, MapTilePtr start, MapTilePtr end) const;

private:
    FHMap&                        m_map;
    MapTileContainer&             m_tileContainer;
    Core::IRandomGenerator* const m_rng;
    std::ostream&                 m_logOutput;
    const bool                    m_extraLogging;
};

}
