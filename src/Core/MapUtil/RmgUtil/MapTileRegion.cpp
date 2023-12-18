/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapTileRegion.hpp"
#include "MapTileContainer.hpp"
#include "MapTileRegionSegmentation.hpp"

#include <iostream>

namespace FreeHeroes {

MapTileRegionList MapTileRegion::splitByFloodFill(bool useDiag, MapTilePtr hint) const
{
    return MapTileRegionSegmentation::splitByFloodFill(*this, useDiag, hint);
}

MapTileRegionList MapTileRegion::splitByMaxArea(size_t maxArea, size_t iterLimit) const
{
    return MapTileRegionSegmentation::splitByMaxArea(*this, maxArea, iterLimit);
}

MapTileRegionList MapTileRegion::splitByK(size_t k, size_t iterLimit) const
{
    return MapTileRegionSegmentation::splitByK(*this, k, iterLimit);
}

MapTileRegionList MapTileRegion::splitByKExt(const KMeansSegmentationSettings& settingsList, size_t iterLimit) const
{
    return MapTileRegionSegmentation::splitByKExt(*this, settingsList, iterLimit);
}

MapTileRegionList MapTileRegion::splitByGrid(int width, int height, size_t threshold) const
{
    auto grid = MapTileRegionSegmentation::splitByGrid(*this, width, height);
    return MapTileRegionSegmentation::reduceGrid(std::move(grid), threshold);
}

MapTilePtr MapTileRegion::makeCentroid(bool ensureInbounds) const
{
    const MapTileRegion& region = *this;
    if (region.empty())
        return nullptr;

    MapTileContainer* tileContainer = region[0]->m_container;

    int64_t   sumX = 0, sumY = 0;
    int64_t   size = region.size();
    const int z    = region[0]->m_pos.m_z;
    for (const auto* cell : region) {
        sumX += cell->m_pos.m_x;
        sumY += cell->m_pos.m_y;
    }
    sumX /= size;
    sumY /= size;
    const auto pos      = FHPos{ static_cast<int>(sumX), static_cast<int>(sumY), z };
    MapTilePtr centroid = tileContainer->m_tileIndex.at(pos);
    if (ensureInbounds && !region.contains(centroid))
        centroid = findClosestPoint(centroid->m_pos);

    /// let's make centroid tile as close to center of mass as possible

    auto estimateAvgDistance = [&region](MapTilePtr centerTile) -> int64_t {
        int64_t result = 0;
        for (const auto* cell : region) {
            result += posDistance(centerTile, cell, 100);
        }
        return result;
    };
    int64_t result = estimateAvgDistance(centroid);

    for (MapTilePtr tile : centroid->m_allNeighboursWithDiag) {
        if (ensureInbounds && !region.contains(tile))
            continue;
        auto altResult = estimateAvgDistance(tile);
        if (altResult < result) {
            result   = altResult;
            centroid = tile;
        }
    }

    return centroid;
}

MapTilePtr MapTileRegion::findClosestPoint(FHPos pos) const
{
    if (empty())
        return nullptr;

    MapTileContainer* tileContainer = (*this)[0]->m_container;
    MapTilePtr        tile          = tileContainer->find(pos); // can be nullptr
    if (tile && contains(tile))
        return tile;

    auto it = std::min_element(cbegin(), cend(), [pos](MapTilePtr l, MapTilePtr r) {
        return posDistance(pos, l->m_pos, 100) < posDistance(pos, r->m_pos, 100);
    });
    return (*it);
}

MapTileRegion MapTileRegion::makeInnerEdge(bool useDiag) const
{
    return makeInnerAndOuterEdge({ .m_useDiag = useDiag, .m_makeInner = true }).m_inner;
}

MapTileRegion MapTileRegion::makeOuterEdge(bool useDiag) const
{
    return makeInnerAndOuterEdge({ .m_useDiag = useDiag, .m_makeOuter = true }).m_outer;
}

EdgeSegmentationResults MapTileRegion::makeInnerAndOuterEdge(EdgeSegmentationParams params) const
{
    //Mernel::ProfilerScope scope("makeInnerAndOuterEdge");
    const int64_t           diameter       = intSqrt(static_cast<int64_t>(size()));
    const size_t            perimeterInner = diameter * 4;
    const size_t            perimeterOuter = (diameter + 2) * 4;
    EdgeSegmentationResults result;
    if (params.m_makeInner)
        result.m_inner.reserve(perimeterInner);
    if (params.m_makeOuter)
        result.m_outer.reserve(perimeterOuter);
    if (params.m_makeCenter)
        result.m_center.reserve(size());
    for (MapTilePtr cell : *this) {
        if (params.m_useDiag) {
            if (contains(cell->m_neighborB)
                && contains(cell->m_neighborT)
                && contains(cell->m_neighborR)
                && contains(cell->m_neighborL)
                && contains(cell->m_neighborTL)
                && contains(cell->m_neighborTR)
                && contains(cell->m_neighborBL)
                && contains(cell->m_neighborBR)) {
                if (params.m_makeCenter)
                    result.m_center.insert(cell);
                continue;
            }
        } else {
            if (contains(cell->m_neighborB)
                && contains(cell->m_neighborT)
                && contains(cell->m_neighborR)
                && contains(cell->m_neighborL)) {
                if (params.m_makeCenter)
                    result.m_center.insert(cell);
                continue;
            }
        }
        if (params.m_makeInner)
            result.m_inner.insert(cell);
        if (params.m_makeOuter) {
            for (auto* ncell : cell->neighboursList(params.m_useDiag)) {
                if (!contains(ncell))
                    result.m_outer.insert(ncell);
            }
        }
    }

    return result;
}

void MapTileRegion::eraseExclaves(bool useDiag)
{
    if (empty())
        return;
    auto           parts   = splitByFloodFill(useDiag);
    auto           it      = std::max_element(parts.begin(), parts.end(), [](const MapTileRegion& l, const MapTileRegion& r) {
        return std::tuple{ l.size(), &l } < std::tuple{ r.size(), &r };
    });
    MapTileRegion& largest = *it;
    *this                  = std::move(largest);
}

}
