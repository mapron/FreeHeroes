/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SegmentHelper.hpp"
#include "AstarGenerator.hpp"
#include "../FHMap.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>

namespace FreeHeroes {

SegmentHelper::SegmentHelper(FHMap&                        map,
                             MapTileContainer&             tileContainer,
                             Core::IRandomGenerator* const rng,
                             std::ostream&                 logOutput)
    : m_map(map)
    , m_tileContainer(tileContainer)
    , m_rng(rng)
    , m_logOutput(logOutput)
{
}

void SegmentHelper::makeBorders(std::vector<TileZone>& tileZones)
{
    using ZKey = std::pair<TileZone*, TileZone*>;
    std::map<ZKey, MapTileRegion> borderTiles;

    auto makeKey = [](TileZone& f, TileZone& s) -> ZKey {
        std::pair key{ &f, &s };
        if (f.m_index > s.m_index)
            key = std::pair{ &s, &f };
        return key;
    };

    auto findZoneById = [&tileZones](const std::string& id) -> TileZone& {
        auto it = std::find_if(tileZones.begin(), tileZones.end(), [&id](const TileZone& zone) { return zone.m_id == id; });
        if (it == tileZones.end())
            throw std::runtime_error("Invalid zone id:" + id);
        return *it;
    };

    for (auto& tileZoneFirst : tileZones) {
        for (auto& tileZoneSecond : tileZones) {
            auto key = makeKey(tileZoneFirst, tileZoneSecond);
            if (borderTiles.contains(key))
                continue;
            MapTileRegion twoSideBorder;
            for (auto* cell : tileZoneFirst.m_area.m_outsideEdge) {
                if (cell->m_zone == &tileZoneSecond)
                    twoSideBorder.insert(cell);
            }
            for (auto* cell : tileZoneSecond.m_area.m_outsideEdge) {
                if (cell->m_zone == &tileZoneFirst)
                    twoSideBorder.insert(cell);
            }
            borderTiles[key] = twoSideBorder;
        }
    }

    MapTileRegion connectionUnblockableCells;

    for (const auto& [connectionId, connections] : m_map.m_template.m_connections) {
        auto&          tileZoneFrom = findZoneById(connections.m_from);
        auto&          tileZoneTo   = findZoneById(connections.m_to);
        auto           key          = makeKey(tileZoneFrom, tileZoneTo);
        MapTileRegion& border       = borderTiles[key];
        if (border.empty()) {
            throw std::runtime_error("No border between '" + connections.m_from + "' and '" + connections.m_to + "'");
        }
        MapTilePtr cell = border.makeCentroid(true); // switch to k-means when we need more than one connection.

        cell->m_zone->m_roadNodesHighPriority.insert(cell);

        const bool guarded = connections.m_guard || !connections.m_mirrorGuard.empty();

        MapTilePtr ncellFound = nullptr;

        for (MapTilePtr ncell : cell->m_orthogonalNeighbours) {
            if (!ncellFound && ncell && ncell->m_zone != cell->m_zone) {
                ncell->m_zone->m_roadNodesHighPriority.insert(ncell);
                if (guarded)
                    ncell->m_zone->m_breakGuardTiles.insert(ncell);
                ncellFound = ncell;
            }
        }
        if (!ncellFound)
            throw std::runtime_error("Failed to get connection neighbour tile");

        MapTilePtr cellFrom = cell->m_zone == &tileZoneFrom ? cell : ncellFound;
        MapTilePtr cellTo   = cell->m_zone == &tileZoneFrom ? ncellFound : cell;

        cellFrom->m_zone->m_namedTiles[connectionId] = cellFrom;
        cellTo->m_zone->m_namedTiles[connectionId]   = cellTo;

        if (guarded) {
            Guard guard;
            guard.m_id           = connectionId;
            guard.m_value        = connections.m_guard;
            guard.m_mirrorFromId = connections.m_mirrorGuard;
            guard.m_pos          = cellFrom;
            guard.m_zone         = nullptr;
            m_guards.push_back(guard);

            cellFrom->m_zone->m_breakGuardTiles.insert(cellFrom);
        }

        MapTileRegion forErase(MapTilePtrList{ cell, ncellFound, cell->m_neighborT, ncellFound->m_neighborT, cell->m_neighborL, ncellFound->m_neighborL });
        border.erase(forErase);

        connectionUnblockableCells.insert(cell);
    }
    MapTileRegion noExpandTiles;
    for (MapTile& tile : m_tileContainer.m_tiles) {
        for (auto* cell : connectionUnblockableCells) {
            if (posDistance(tile.m_pos, cell->m_pos) < 4)
                noExpandTiles.insert(&tile);
        }
    }
    MapTileRegion needBeBlocked;
    MapTileRegion tentativeBlocked;
    for (const auto& [key, border] : borderTiles) {
        needBeBlocked.insert(border);
    }
    for (const auto& [key, border] : borderTiles) {
        for (auto* cell : border) {
            for (MapTilePtr ncell : cell->m_orthogonalNeighbours) {
                if (needBeBlocked.contains(ncell))
                    continue;
                if (noExpandTiles.contains(ncell))
                    continue;
                tentativeBlocked.insert(ncell);
            }
        }
    }

    for (auto& tileZone : tileZones) {
        tileZone.m_innerAreaUsable = {};
        for (auto* cell : tileZone.m_area.m_innerArea) {
            if (needBeBlocked.contains(cell))
                tileZone.m_needBeBlocked.insert(cell);
            if (tentativeBlocked.contains(cell))
                tileZone.m_tentativeBlocked.insert(cell);
        }
        for (auto* cell : tileZone.m_area.m_innerArea) {
            if (tileZone.m_blocked.contains(cell)
                || tileZone.m_needBeBlocked.contains(cell)
                || tileZone.m_tentativeBlocked.contains(cell))
                continue;
            tileZone.m_innerAreaUsable.m_innerArea.insert(cell);
        }
        tileZone.m_innerAreaUsable.makeEdgeFromInnerArea();

        tileZone.m_innerAreaBottomLine = tileZone.m_innerAreaUsable.getBottomEdge();
        tileZone.m_innerAreaUsable.m_innerArea.erase(tileZone.m_innerAreaBottomLine);
        tileZone.m_innerAreaUsable.makeEdgeFromInnerArea();
    }
}

void SegmentHelper::makeSegments(TileZone& tileZone)
{
    // todo: why repulse for K-means? why segments must be far from each other on consequtive indices? I dunno
    auto segmentList             = tileZone.m_innerAreaUsable.m_innerArea.splitByMaxArea(m_logOutput, tileZone.m_rngZoneSettings.m_segmentAreaSize, true);
    tileZone.m_innerAreaSegments = MapTileRegionWithEdge::makeEdgeList(segmentList);
    auto borderNet               = MapTileRegionWithEdge::getInnerBorderNet(tileZone.m_innerAreaSegments);

    tileZone.m_roadNodes.insert(tileZone.m_roadNodesHighPriority);

    for (size_t i = 0; auto& area : tileZone.m_innerAreaSegments) {
        i++;
        area.removeEdgeFromInnerArea();

        for (auto* cell : area.m_innerEdge) {
            cell->m_segmentIndex = i;
            if (borderNet.contains(cell))
                tileZone.m_possibleRoadsArea.insert(cell);
        }

        for (auto* cell : area.m_innerArea) {
            cell->m_segmentIndex = i;
        }
    }

    for (MapTileRegionWithEdge& area : tileZone.m_innerAreaSegments) {
        for (MapTilePtr cell : area.m_innerEdge) {
            std::set<std::pair<TileZone*, size_t>> neighAreaBorders;
            if (!cell->m_neighborB || !cell->m_neighborT || !cell->m_neighborL || !cell->m_neighborR)
                neighAreaBorders.insert(std::pair<TileZone*, size_t>{ nullptr, 0 });

            for (MapTilePtr cellAdj : cell->m_orthogonalNeighbours) {
                if (area.contains(cellAdj))
                    continue;
                size_t    neighbourSegIndex = cellAdj->m_segmentIndex;
                TileZone* neightZone        = cellAdj->m_zone;
                if (neightZone != &tileZone)
                    neighbourSegIndex = 0;
                neighAreaBorders.insert({ neightZone, neighbourSegIndex });
            }
            if (neighAreaBorders.size() > 1) {
                int64_t minDistance = 1000;
                for (auto* roadCell : tileZone.m_roadNodes) {
                    minDistance = std::min(minDistance, posDistance(cell->m_pos, roadCell->m_pos));
                }
                if (minDistance > 2) {
                    tileZone.m_roadNodes.insert(cell);
                }
            }
        }
    }
    tileZone.m_possibleRoadsArea.insert(tileZone.m_roadNodes);
    tileZone.m_innerAreaSegmentsUnited.clear();
    for (auto& seg : tileZone.m_innerAreaSegments) {
        tileZone.m_innerAreaSegmentsUnited.insert(seg.m_innerArea);
    }
}

void SegmentHelper::refineSegments(TileZone& tileZone)
{
    Mernel::ProfilerScope scope("refineSegments");
    auto                  innerWithoutRoads = tileZone.m_innerAreaUsable.m_innerArea;
    innerWithoutRoads.erase(tileZone.m_placedRoads);
    {
        auto noSegmentArea = innerWithoutRoads;
        for (auto& seg : tileZone.m_innerAreaSegments) {
            noSegmentArea.erase(seg.m_innerArea);
        }
        for (auto cell : noSegmentArea)
            cell->m_segmentIndex = 0;
    }
    for (size_t index = 0; auto& seg : tileZone.m_innerAreaSegments) {
        index++;
        seg.refineEdge(MapTileRegionWithEdge::RefineTask::RemoveHollows, innerWithoutRoads, index);
        seg.refineEdge(MapTileRegionWithEdge::RefineTask::RemoveSpikes, innerWithoutRoads, index);

        seg.refineEdge(MapTileRegionWithEdge::RefineTask::Expand, innerWithoutRoads, index);

        seg.refineEdge(MapTileRegionWithEdge::RefineTask::RemoveHollows, innerWithoutRoads, index);
        seg.refineEdge(MapTileRegionWithEdge::RefineTask::RemoveSpikes, innerWithoutRoads, index);
    }
    tileZone.m_innerAreaSegmentsUnited.clear();
    for (auto& seg : tileZone.m_innerAreaSegments) {
        tileZone.m_innerAreaSegmentsUnited.insert(seg.m_innerArea);
    }
}

void SegmentHelper::makeHeatMap(TileZone& tileZone)
{
    using TileIntMapping = std::map<MapTilePtr, int>;
    using WeightTileMap  = std::map<int, MapTilePtrList>;

    TileIntMapping costs;
    for (auto tile : tileZone.m_innerAreaUsable.m_innerArea)
        costs[tile] = 100;
    for (auto& road : tileZone.m_roads) {
        int cost = 100;
        if (road.m_level == RoadLevel::Towns)
            cost = 20;
        if (road.m_level == RoadLevel::Exits)
            cost = 40;
        if (road.m_level == RoadLevel::InnerPoints || road.m_level == RoadLevel::BorderPoints)
            cost = 70;
        for (auto tile : road.m_path)
            costs[tile] = cost;
    }

    std::set<MapTilePtr> remaining, completed, edgeSet;

    TileIntMapping resultDistance;
    for (auto tile : tileZone.m_roadNodesTowns)
        completed.insert(tile);
    if (completed.empty()) {
        for (auto tile : tileZone.m_roadNodesHighPriority)
            completed.insert(tile);
    }
    edgeSet = completed;

    for (auto tile : tileZone.m_innerAreaUsable.m_innerArea) {
        if (!completed.contains(tile))
            remaining.insert(tile);
    }

    WeightTileMap edge;
    for (auto tile : completed)
        edge[0].push_back(tile);

    auto calcCost = [&costs](MapTilePtr one, MapTilePtr two, bool diag) -> int {
        const int costOne = costs.at(one);
        const int costTwo = costs.at(two);
        const int cost    = std::max(costOne, costTwo);
        return diag ? cost * 141 / 100 : cost;
    };

    auto consumePrevEdge = [&edge, &edgeSet, &resultDistance, &remaining, &completed, &calcCost]() -> bool {
        if (edge.empty())
            return false;

        const int            lowestCost      = edge.begin()->first;
        const MapTilePtrList lowestCostTiles = edge.begin()->second;
        edge.erase(edge.begin());

        for (auto tile : lowestCostTiles) {
            resultDistance[tile] = lowestCost;
            completed.insert(tile);
            edgeSet.erase(tile);
            remaining.erase(tile);
        }
        for (auto tile : lowestCostTiles) {
            for (bool diag : { false, true }) {
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

    while (consumePrevEdge()) {
        ;
    }

    std::map<int, int> heatFrequencies;
    for (const auto& [tile, w] : resultDistance)
        heatFrequencies[w]++;

    const int maxHeat           = 10;
    const int totalTiles        = costs.size();
    int       currentHeatTarget = 1;
    int       currentHeatCnt    = currentHeatTarget * totalTiles / maxHeat;
    int       totalCnt          = 0;

    std::map<int, int> heatThresholds;
    heatThresholds[0] = 0;
    int maxcost       = 0;
    for (const auto& [w, cnt] : heatFrequencies) {
        totalCnt += cnt;
        maxcost = std::max(maxcost, w);
        if (totalCnt > currentHeatCnt) {
            heatThresholds[w] = currentHeatTarget;
            currentHeatTarget++;
            currentHeatCnt = currentHeatTarget * totalTiles / maxHeat;
        }
    }
    heatThresholds[maxcost] = maxHeat;
    TileIntMapping resultHeat;
    for (const auto& [tile, w] : resultDistance) {
        auto it = heatThresholds.lower_bound(w); // first [w2, heat] where w2 >= w
        assert(it != heatThresholds.cend());
        const int heat             = std::max(1, std::min(maxHeat, it->second));
        tileZone.m_tile2heat[tile] = heat;
        tileZone.m_regionsByHeat[heat].insert(tile);
    }
}

}
