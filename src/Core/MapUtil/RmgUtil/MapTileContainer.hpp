/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTile.hpp"
#include "MapTileRegion.hpp"

#include "MapUtilExport.hpp"

#include <set>
#include <unordered_map>

namespace FreeHeroes {

class MAPUTIL_EXPORT MapTileContainer {
public:
    int m_width  = 0;
    int m_height = 0;
    int m_depth  = 0;

    MapTileRegion m_all;
    MapTileRegion m_innerEdge;

    std::unordered_map<FHPos, MapTilePtr> m_tileIndex;
    MapTilePtr                            m_centerTile = nullptr;

    void init(int width, int height, int depth);

    bool fixExclaves();

    MapTilePtr find(FHPos pos) const noexcept
    {
        auto it = m_tileIndex.find(pos);
        return it == m_tileIndex.cend() ? nullptr : it->second;
    }

private:
    std::vector<MapTile> m_tiles;
};
}
