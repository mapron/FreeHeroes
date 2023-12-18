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
struct EdgeSegmentationResults;

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

    struct EdgeSegmentationParams {
        bool m_useDiag    = false;
        bool m_makeInner  = false;
        bool m_makeOuter  = false;
        bool m_makeCenter = false;
    };

    MapTileRegion           makeInnerEdge(bool useDiag) const;
    MapTileRegion           makeOuterEdge(bool useDiag) const;
    EdgeSegmentationResults makeInnerAndOuterEdge(EdgeSegmentationParams params) const;

    void eraseExclaves(bool useDiag);
};

struct EdgeSegmentationResults {
    MapTileRegion m_inner;
    MapTileRegion m_outer;
    MapTileRegion m_center;
};

}
