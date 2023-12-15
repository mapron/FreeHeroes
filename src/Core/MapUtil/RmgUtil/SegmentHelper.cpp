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

        cell->m_zone->m_nodes.add(cell, RoadLevel::Exits);

        const bool guarded = connections.m_guard || !connections.m_mirrorGuard.empty();

        MapTilePtr ncellFound = nullptr;

        for (MapTilePtr ncell : cell->m_orthogonalNeighbours) {
            if (!ncellFound && ncell && ncell->m_zone != cell->m_zone) {
                ncell->m_zone->m_nodes.add(cell, RoadLevel::Exits);
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
    for (MapTilePtr tile : m_tileContainer.m_all) {
        for (auto* cell : connectionUnblockableCells) {
            if (posDistance(tile->m_pos, cell->m_pos) < 4)
                noExpandTiles.insert(tile);
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
    Mernel::ProfilerScope scope("makeSegments");
    // make k-means segmentation
    auto segmentList = tileZone.m_innerAreaUsable.m_innerArea.splitByMaxArea(tileZone.m_rngZoneSettings.m_segmentAreaSize, 30);

    if (segmentList.empty())
        throw std::runtime_error("No segments in tile zone!");

    tileZone.setSegments(MapTileRegionWithEdge::makeEdgeList(segmentList));
    // smooth edges
    {
        MapTileRegion allowed;
        for (auto& seg : tileZone.m_innerAreaSegments) {
            seg.refineEdgeRemoveSpikes(allowed);
        }
        for (auto& seg : tileZone.m_innerAreaSegments) {
            seg.refineEdgeRemoveHollows(allowed);
        }
    }

    // make first iteration of inner network (can have holes)
    auto borderNet = MapTileRegionWithEdge::getInnerBorderNet(tileZone.getSegments());

    //m_logOutput << "borderNet=" << borderNet.size() << "\n";

    // remove inner network from segments
    for (auto& seg : tileZone.m_innerAreaSegments) {
        seg.m_innerArea.erase(borderNet);
        seg.makeEdgeFromInnerArea();
    }

    // smooth edges
    MapTileRegion segmentSpikesOnBorder;
    {
        MapTileRegion segmentSpikes;
        for (auto& seg : tileZone.m_innerAreaSegments) {
            seg.refineEdgeRemoveSpikes(segmentSpikes);
        }
        segmentSpikesOnBorder = segmentSpikes.intersectWith(tileZone.m_innerAreaUsable.m_innerEdge);
    }

    tileZone.updateSegmentIndex();

    // make bordernet as everything non-segment
    borderNet = tileZone.m_innerAreaUsable.m_innerArea.diffWith(tileZone.m_innerAreaSegmentsUnited);

    //

    MapTileRegion innerNodes;
    //MapTileRegion outerNodes;
    // walk over borderNet, calculate local maximum of outsideEdgeCounter for each tile
    for (MapTilePtr cell : borderNet) {
        MapTileRegion cellLocal;
        for (auto* n : cell->m_allNeighboursWithDiag)
            cellLocal.insert(n->m_orthogonalNeighbours);
        cellLocal.erase(cell->m_allNeighboursWithDiag);
        cellLocal.erase(cell);
        cellLocal.erase(borderNet);
        //bool isBorder = false;

        std::set<std::pair<bool, MapTileSegment*>> neighAreaBorders; // unique set of neighbor zones
        for (auto* neighbour : cellLocal) {
            if (neighbour->m_allNeighboursWithDiag.size() != 8) {
                neighAreaBorders.insert(std::pair<bool, MapTileSegment*>{ false, nullptr }); // map border
            }

            MapTileSegment* neighbourSegIndex = neighbour->m_segmentMedium;
            TileZone*       neightZone        = neighbour->m_zone;
            bool            selfZone          = neightZone == &tileZone;
            if (!selfZone)
                neighbourSegIndex = nullptr;

            neighAreaBorders.insert({ selfZone, neighbourSegIndex });
        }
        if (neighAreaBorders.size() >= 3) {
            innerNodes.insert(cell);
        }
    }

    //    if (1)
    //        return;
    //    {
    //        for (MapTilePtr cell : innerNodes) {
    //            tileZone.m_nodes.addIfNotExist(cell, RoadLevel::InnerPoints);
    //        }
    //    }
    //    if (1)
    //       return;

    // reduce each road node group to single node.
    // try town nodes;
    // try zone inner edge;
    // try centroid.
    MapTileRegion innerNodesReduced;
    MapTileRegion outerNodesReduced;
    auto          innerNodesSegmentList = innerNodes.splitByFloodFill(true);
    for (auto& group : innerNodesSegmentList) {
        if (!tileZone.m_nodes.m_byLevel[RoadLevel::Exits].intersectWith(group).empty())
            continue;
        if (!tileZone.m_innerAreaTownsBorders.intersectWith(group).empty())
            continue;

        auto borderIntersection = tileZone.m_innerAreaUsable.m_innerEdge.intersectWith(group).diffWith(segmentSpikesOnBorder);
        if (!borderIntersection.empty()) {
            if (borderIntersection.size() == 1) {
                if (segmentSpikesOnBorder.contains(borderIntersection[0]))
                    continue;
            }
            outerNodesReduced.insert(borderIntersection[0]);
            continue;
        }

        innerNodesReduced.insert(group.makeCentroid(true));
    }
    tileZone.m_roadPotentialArea.insert(tileZone.m_nodes.m_all);

    for (MapTilePtr cell : outerNodesReduced)
        tileZone.m_nodes.add(cell, RoadLevel::BorderPoints);
    for (MapTilePtr cell : innerNodesReduced)
        tileZone.m_nodes.add(cell, RoadLevel::InnerPoints);

    tileZone.m_roadPotentialArea.insert(borderNet);

    // make sure everything is connected in m_roadPotentialArea
    {
        auto parts = tileZone.m_roadPotentialArea.splitByFloodFill(true);
        if (parts.size() >= 2) {
            auto                 largestIt = std::max_element(parts.cbegin(), parts.cend(), [](const MapTileRegion& l, const MapTileRegion& r) {
                return l.size() < r.size();
            });
            const MapTileRegion* largest   = &(*largestIt);
            for (const MapTileRegion& orphan : parts) {
                if (&orphan == largest)
                    continue;
                if (orphan.size() < 3 && tileZone.m_nodes.m_all.intersectWith(orphan).empty())
                    continue;
                auto* orphanCentroid = orphan.makeCentroid(false);

                auto             itLargest      = std::min_element(largest->cbegin(), largest->cend(), [orphanCentroid](MapTilePtr l, MapTilePtr r) {
                    return posDistance(orphanCentroid, l, 100) < posDistance(orphanCentroid, r, 100);
                });
                const MapTilePtr largestNearest = *itLargest;

                auto itOrphan = std::min_element(orphan.cbegin(), orphan.cend(), [largestNearest](MapTilePtr l, MapTilePtr r) {
                    return posDistance(largestNearest, l, 100) < posDistance(largestNearest, r, 100);
                });

                const MapTilePtr closestTileInOrphan = *itOrphan;
                auto             connection          = closestTileInOrphan->makePathTo(true, largestNearest);

                tileZone.m_roadPotentialArea.insert(connection);
            }
        }
    }

    tileZone.updateSegmentIndex();
}

void SegmentHelper::refineSegments(TileZone& tileZone)
{
    Mernel::ProfilerScope scope("refineSegments");
    auto                  innerWithoutRoads = tileZone.m_innerAreaUsable.m_innerArea;
    innerWithoutRoads.erase(tileZone.m_roads.m_all);

    for (auto& seg : tileZone.m_innerAreaSegments) {
        seg.m_innerArea.erase(tileZone.m_roads.m_all);
        seg.makeEdgeFromInnerArea();
        seg.refineEdgeRemoveSpikes(innerWithoutRoads);
    }

    for (auto& seg : tileZone.m_innerAreaSegments)
        seg.refineEdgeRemoveHollows(innerWithoutRoads);

    for (auto& seg : tileZone.m_innerAreaSegments)
        seg.refineEdgeExpand(innerWithoutRoads);

    for (auto& seg : tileZone.m_innerAreaSegments)
        seg.refineEdgeRemoveSpikes(innerWithoutRoads);

    for (auto& seg : tileZone.m_innerAreaSegments)
        seg.refineEdgeRemoveHollows(innerWithoutRoads);

    tileZone.updateSegmentIndex();
}

void SegmentHelper::makeHeatMap(TileZone& tileZone)
{
    using TileIntMapping = std::map<MapTilePtr, int>;
    using WeightTileMap  = std::map<int, MapTilePtrList>;

    TileIntMapping costs;
    for (auto tile : tileZone.m_innerAreaUsable.m_innerArea)
        costs[tile] = 100;
    for (const auto& [level, area] : tileZone.m_roads.m_byLevel) {
        int cost = 100;
        if (level == RoadLevel::Towns)
            cost = 20;
        if (level == RoadLevel::Exits)
            cost = 40;
        if (level == RoadLevel::InnerPoints || level == RoadLevel::BorderPoints)
            cost = 70;
        for (auto* tile : area)
            costs[tile] = cost;
    }

    std::set<MapTilePtr> remaining, completed, edgeSet;

    TileIntMapping resultDistance;
    for (auto tile : tileZone.m_nodes.m_byLevel[RoadLevel::Towns])
        completed.insert(tile);
    if (completed.empty()) {
        for (auto tile : tileZone.m_nodes.m_byLevel[RoadLevel::Exits])
            completed.insert(tile);
    }
    if (completed.empty())
        completed.insert(tileZone.m_centroid);

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

    WeightTileMap      resultByDistance;
    std::map<int, int> heatFrequencies;
    for (const auto& [tile, w] : resultDistance)
        resultByDistance[w].push_back(tile);

    MapTilePtrList roadTiles;
    MapTilePtrList segmentTiles;
    for (const auto& [_, area] : resultByDistance) {
        for (auto* tile : area) {
            if (tileZone.m_roads.m_all.contains(tile))
                roadTiles.push_back(tile);
            else
                segmentTiles.push_back(tile);
        }
    }

    // 0 * 15 / 10 = 0
    // 1 * 15 / 10 = 1
    // 2 * 15 / 10 = 3
    // 9 * 15 / 10 = 13
    auto chop = [](const MapTilePtrList& src, TileZone::HeatData& dest, int heat, int maxHeat) {
        const size_t totalTiles = src.size();
        const size_t startIndex = heat * totalTiles / maxHeat;
        const size_t endIndex   = (heat + 1) * totalTiles / maxHeat;
        for (size_t i = startIndex; i < endIndex; ++i)
            dest.add(src[i], heat);
    };

    const int maxHeat = 10; // @todo:
    for (int heat = 0; heat < maxHeat; heat++) {
        chop(roadTiles, tileZone.m_roadHeat, heat, maxHeat);
        chop(segmentTiles, tileZone.m_segmentHeat, heat, maxHeat);
    }
}

}
