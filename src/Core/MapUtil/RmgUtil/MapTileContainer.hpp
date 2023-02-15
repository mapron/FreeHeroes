/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTileArea.hpp"

#include <set>
#include <unordered_map>

namespace FreeHeroes {

struct MapTileContainer {
    std::vector<MapTile> m_tiles;

    std::unordered_map<FHPos, MapTilePtr> m_tileIndex;
    MapTilePtr                            m_centerTile = nullptr;

    std::set<TileZone*> m_dirtyZones; // zone ids that must be re-read from the map.

    MapTileRegion m_blocked;
    MapTileRegion m_needBeBlocked;
    MapTileRegion m_tentativeBlocked;

    void init(int width, int height, int depth);

    void checkAllTerrains(const MapTileRegion& posPlaced);

    bool fixExclaves();
};
}
