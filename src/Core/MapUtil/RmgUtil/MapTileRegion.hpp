/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FlatSet.hpp"

#include "MapUtilExport.hpp"

namespace FreeHeroes {
struct MapTile;
using MapTilePtr = MapTile*;

class MAPUTIL_EXPORT MapTileRegion : public FlatSet<MapTilePtr> {
public:
    using FlatSet<MapTilePtr>::FlatSet;
    /*implicit*/ MapTileRegion(const FlatSet<MapTilePtr>& data)
        : FlatSet<MapTilePtr>(data)
    {}
    /*implicit*/ MapTileRegion(FlatSet<MapTilePtr>&& data)
        : FlatSet<MapTilePtr>(std::move(data))
    {}
};

}
