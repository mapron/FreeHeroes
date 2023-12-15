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
class MapTileContainer;
struct KMeansSegmentationSettings;

class MAPUTIL_EXPORT MapTileRegion : public FlatSet<MapTilePtr> {
public:
    using FlatSet<MapTilePtr>::FlatSet;
    /*implicit*/ MapTileRegion(const FlatSet<MapTilePtr>& data)
        : FlatSet<MapTilePtr>(data)
    {}
    /*implicit*/ MapTileRegion(FlatSet<MapTilePtr>&& data)
        : FlatSet<MapTilePtr>(std::move(data))
    {}

    MapTileRegionList splitByFloodFill(bool useDiag, MapTilePtr hint = nullptr) const;
    MapTileRegionList splitByMaxArea(size_t maxArea, size_t iterLimit = 100) const;
    MapTileRegionList splitByK(size_t k, size_t iterLimit = 100) const;
    MapTileRegionList splitByKExt(const KMeansSegmentationSettings& settingsList, size_t iterLimit = 100) const;
    MapTileRegionList splitByGrid(int width, int height, size_t threshold) const;

    MapTilePtr makeCentroid(bool ensureInbounds) const;

    MapTilePtr findClosestPoint(FHPos pos) const;

    MapTileRegion                           makeInnerEdge(bool useDiag) const { return makeInnerAndOuterEdge(useDiag).first; }
    MapTileRegion                           makeOuterEdge(bool useDiag) const { return makeInnerAndOuterEdge(useDiag).second; }
    std::pair<MapTileRegion, MapTileRegion> makeInnerAndOuterEdge(bool useDiag) const;
};

}
