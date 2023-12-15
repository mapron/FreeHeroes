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

std::pair<MapTileRegion, MapTileRegion> MapTileRegion::makeInnerAndOuterEdge(bool useDiag) const
{
    //Mernel::ProfilerScope scope("makeInnerAndOuterEdge");
    const int64_t diameter       = intSqrt(static_cast<int64_t>(size()));
    const size_t  perimeterInner = diameter * 4;
    const size_t  perimeterOuter = (diameter + 2) * 4;
    MapTileRegion inner;
    MapTileRegion outer;
    inner.reserve(perimeterInner);
    outer.reserve(perimeterOuter);
    for (MapTilePtr cell : *this) {
        if (useDiag) {
            if (contains(cell->m_neighborB)
                && contains(cell->m_neighborT)
                && contains(cell->m_neighborR)
                && contains(cell->m_neighborL)
                && contains(cell->m_neighborTL)
                && contains(cell->m_neighborTR)
                && contains(cell->m_neighborBL)
                && contains(cell->m_neighborBR))
                continue;
        } else {
            if (contains(cell->m_neighborB)
                && contains(cell->m_neighborT)
                && contains(cell->m_neighborR)
                && contains(cell->m_neighborL))
                continue;
        }
        inner.insert(cell);
        for (auto* ncell : cell->neighboursList(useDiag)) {
            if (!contains(ncell))
                outer.insert(ncell);
        }
    }
    /*
       for (MapTilePtr cell : m_innerEdge) {
        if (m_diagonalGrowth) {
            if (m_innerArea.contains(cell->m_neighborB)
                && m_innerArea.contains(cell->m_neighborT)
                && m_innerArea.contains(cell->m_neighborR)
                && m_innerArea.contains(cell->m_neighborL)
                && m_innerArea.contains(cell->m_neighborTL)
                && m_innerArea.contains(cell->m_neighborTR)
                && m_innerArea.contains(cell->m_neighborBL)
                && m_innerArea.contains(cell->m_neighborBR))
                forErase.push_back(cell);
        } else {
            if (m_innerArea.contains(cell->m_neighborB)
                && m_innerArea.contains(cell->m_neighborT)
                && m_innerArea.contains(cell->m_neighborR)
                && m_innerArea.contains(cell->m_neighborL))
                forErase.push_back(cell);
        }
    }
*/
    return std::pair{ std::move(inner), std::move(outer) };
}

}
