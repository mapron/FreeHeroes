/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TemplateZone.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

class RoadHelper {
public:
    RoadHelper(FHMap&                        map,
               MapCanvas&                    mapCanvas,
               Core::IRandomGenerator* const rng,
               std::ostream&                 logOutput);

    void placeRoads(TileZone& tileZone);

    void placeRoad(TileZone& tileZone, std::vector<FHPos> path);
    void placeRoad(std::vector<FHPos> path);

private:
    std::vector<FHPos> aStarPath(TileZone& zone, MapCanvas::Tile* start, MapCanvas::Tile* end, bool allTiles);

private:
    FHMap&                        m_map;
    MapCanvas&                    m_mapCanvas;
    Core::IRandomGenerator* const m_rng;
    std::ostream&                 m_logOutput;
};

}
