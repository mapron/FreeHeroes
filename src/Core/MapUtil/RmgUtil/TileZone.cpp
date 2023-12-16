/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TileZone.hpp"

#include "TemplateUtils.hpp"

#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes {

void TileZone::setSegments(MapTileRegionWithEdgeList list)
{
    m_innerAreaSegments.resize(list.size());
    for (size_t i = 0; i < list.size(); ++i) {
        m_innerAreaSegments[i]         = Segment(list[i]);
        m_innerAreaSegments[i].m_index = i;
    }
}

MapTileRegionWithEdgeList TileZone::getSegments() const
{
    MapTileRegionWithEdgeList result;
    result.resize(m_innerAreaSegments.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = m_innerAreaSegments[i];
    }
    return result;
}

void TileZone::updateSegmentIndex()
{
    for (auto* tile : m_innerAreaUsable.m_innerArea)
        tile->m_segmentMedium = nullptr;

    m_innerAreaSegmentsUnited.clear();
    for (auto& seg : m_innerAreaSegments) {
        m_innerAreaSegmentsUnited.insert(seg.m_innerArea);
        for (auto* tile : seg.m_innerArea)
            tile->m_segmentMedium = &seg;
    }
}

TileZone::TileIntMapping TileZone::makeMoveCosts(bool onlyUsable) const
{
    TileIntMapping costs;
    auto&          area = onlyUsable ? m_innerAreaUsable.m_innerArea : m_area.m_innerArea;
    for (auto tile : area)
        costs[tile] = 100;
    for (const auto& [level, rarea] : m_roads.m_byLevel) {
        int cost = 100;
        if (level == RoadLevel::Towns)
            cost = 20;
        if (level == RoadLevel::Exits)
            cost = 40;
        if (level == RoadLevel::InnerPoints || level == RoadLevel::BorderPoints)
            cost = 70;
        for (auto* tile : rarea)
            costs[tile] = cost;
    }
    return costs;
}

TileZone::WeightTileMap TileZone::computeDistances(const TileIntMapping& costs, std::set<MapTilePtr> completed, std::set<MapTilePtr> remaining, int maxCost, int iters)
{
    std::set<MapTilePtr> edgeSet = completed;

    TileZone::TileIntMapping resultDistance;

    WeightTileMap edge;
    for (auto tile : completed)
        edge[0].push_back(tile);

    auto calcCost = [&costs](MapTilePtr one, MapTilePtr two, bool diag) -> int {
        const int costOne = costs.at(one);
        const int costTwo = costs.at(two);
        const int cost    = std::max(costOne, costTwo);
        return diag ? cost * 141 / 100 : cost;
    };

    auto consumePrevEdge = [maxCost, &edge, &edgeSet, &resultDistance, &remaining, &completed, &calcCost]() -> bool {
        if (edge.empty())
            return false;

        const int            lowestCost      = edge.begin()->first;
        const MapTilePtrList lowestCostTiles = edge.begin()->second;
        edge.erase(edge.begin());

        if (maxCost > 0 && lowestCost > maxCost)
            return true;

        for (auto tile : lowestCostTiles) {
            resultDistance[tile] = lowestCost;
            completed.insert(tile);
            edgeSet.erase(tile);
            remaining.erase(tile);
        }
        for (bool diag : { false, true }) {
            for (auto tile : lowestCostTiles) {
                auto& nlist = diag ? tile->m_diagNeighbours : tile->m_orthogonalNeighbours;
                for (auto ntile : nlist) {
                    if (completed.contains(ntile) || edgeSet.contains(ntile) || !remaining.contains(ntile))
                        continue;
                    const int cost = calcCost(tile, ntile, diag) + lowestCost;
                    edgeSet.insert(ntile);
                    edge[cost].push_back(ntile);
                }
            }
        }

        return true;
    };

    for (int i = 0; i < iters; ++i) {
        if (!consumePrevEdge())
            break;
    }

    WeightTileMap resultByDistance;

    for (const auto& [tile, w] : resultDistance)
        resultByDistance[w].push_back(tile);

    return resultByDistance;
}

}
