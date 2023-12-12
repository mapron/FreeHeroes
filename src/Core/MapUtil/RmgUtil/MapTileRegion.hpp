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
class MapTileRegion;
using MapTileRegionList = std::vector<MapTileRegion>;
struct FHPos;

class MAPUTIL_EXPORT MapTileRegion : public FlatSet<MapTilePtr> {
public:
    using FlatSet<MapTilePtr>::FlatSet;
    /*implicit*/ MapTileRegion(const FlatSet<MapTilePtr>& data)
        : FlatSet<MapTilePtr>(data)
    {}
    /*implicit*/ MapTileRegion(FlatSet<MapTilePtr>&& data)
        : FlatSet<MapTilePtr>(std::move(data))
    {}

    struct SplitRegionSettings {
        MapTilePtr m_start  = nullptr;
        int        m_speed  = 100;
        int64_t    m_radius = 100;
    };
    using SplitRegionSettingsList = std::vector<SplitRegionSettings>;

    MapTileRegionList splitByFloodFill(bool useDiag, MapTilePtr hint = nullptr) const;
    MapTileRegionList splitByMaxArea(size_t maxArea, size_t iterLimit = 100) const;
    MapTileRegionList splitByK(size_t k, size_t iterLimit = 100) const;
    MapTileRegionList splitByKExt(const SplitRegionSettingsList& settingsList, size_t iterLimit = 100) const;

    MapTilePtr makeCentroid(bool ensureInbounds) const;

    MapTilePtr findClosestPoint(FHPos pos) const;
};

}
