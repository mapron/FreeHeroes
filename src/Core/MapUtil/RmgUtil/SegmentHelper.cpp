/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SegmentHelper.hpp"
#include "MapTileRegionSegmentation.hpp"
#include "AstarGenerator.hpp"
#include "../FHMap.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>

namespace FreeHeroes {

SegmentHelper::SegmentHelper(FHMap&                        map,
                             MapTileContainer&             tileContainer,
                             Core::IRandomGenerator* const rng,
                             std::ostream&                 logOutput,
                             bool                          extraLogging)
    : m_map(map)
    , m_tileContainer(tileContainer)
    , m_rng(rng)
    , m_logOutput(logOutput)
    , m_extraLogging(extraLogging)
{
}

void SegmentHelper::makeInitialZones(std::vector<TileZone>& tileZones)
{
    const int w = m_map.m_tileMap.m_width;
    const int h = m_map.m_tileMap.m_height;

    const int64_t mapArea = w * h;

    size_t totalRelativeArea = 0;
    for (auto& tileZone : tileZones) {
        totalRelativeArea += tileZone.m_relativeArea;
    }
    if (!totalRelativeArea)
        throw std::runtime_error("Total relative area can't be zero");

    for (auto& tileZone : tileZones) {
        tileZone.m_absoluteArea   = tileZone.m_relativeArea * mapArea / totalRelativeArea;
        tileZone.m_absoluteRadius = intSqrt(tileZone.m_absoluteArea * 1'000'000);

        m_logOutput << m_indent << "zone [" << tileZone.m_id << "] area=" << tileZone.m_absoluteArea
                    << ", radius=" << (double(tileZone.m_absoluteRadius) / 1000)
                    << ", startTile=" << tileZone.m_startTile->toPrintableString()
                    << ", townFaction=" << tileZone.m_mainTownFaction->id
                    << ", rewardFaction=" << tileZone.m_rewardsFaction->id
                    << ", terrain=" << tileZone.m_terrain->id
                    << ", zoneGuardPercent=" << tileZone.m_rngZoneSettings.m_zoneGuardPercent
                    << "\n";
    }

    KMeansSegmentationSettings settings;
    settings.m_items.resize(tileZones.size());
    for (auto& tileZone : tileZones) {
        settings.m_items[tileZone.m_index] = KMeansSegmentationSettings::Item{
            .m_initialCentroid = tileZone.m_startTile,
            .m_areaHint        = tileZone.m_absoluteArea,
            .m_extraMassPoint  = tileZone.m_startTile,
            .m_extraMassWeight = size_t(tileZone.m_absoluteArea) * 2,
        };
    }

    auto splitRegions = m_tileContainer.m_all.splitByKExt(settings, 1);
    for (auto& tileZone : tileZones) {
        const auto& zoneArea           = splitRegions[tileZone.m_index];
        auto&       zoneSettings       = settings.m_items[tileZone.m_index];
        const auto  centroid           = zoneArea.makeCentroid(true);
        zoneSettings.m_initialCentroid = centroid;

        //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = centroid->m_pos, .m_brushColor = 330, .m_shapeRadius = 4 });
    }

    const int attempts = 10;
    for (int i = 0; i < attempts; i++) {
        bool done = true;
        for (auto& tileZone : tileZones) {
            const auto& zoneArea     = splitRegions[tileZone.m_index];
            auto&       zoneSettings = settings.m_items[tileZone.m_index];
            //const auto    centroid     = zoneArea.makeCentroid(true);
            const int64_t intendedArea = tileZone.m_absoluteArea;
            const int64_t prevArea     = zoneSettings.m_areaHint;
            const int64_t placedArea   = zoneArea.size();

            const int64_t intendedRadius = MapTileRegionSegmentation::getRadiusPromille(intendedArea);
            const int64_t prevRadius     = MapTileRegionSegmentation::getRadiusPromille(prevArea);
            const int64_t placedRadius   = MapTileRegionSegmentation::getRadiusPromille(placedArea);

            const int64_t correctionRadius = std::max(int64_t(100), prevRadius + (intendedRadius - placedRadius));

            const int64_t correctionArea = MapTileRegionSegmentation::getArea(correctionRadius);
            const int64_t diff           = std::abs(placedArea - intendedArea);
            const int64_t diffPercent    = diff * 100 / tileZone.m_absoluteArea;
            //zoneSettings.m_initialCentroid = centroid;
            zoneSettings.m_areaHint = std::max(int64_t(1), correctionArea);
            if (intendedRadius < placedRadius) { // need shrink zone = need bigger distance weight
                //zoneSettings.m_insideWeight  = zoneSettings.m_insideWeight * (100 + diffPercent) / 100;
                //zoneSettings.m_outsideWeight = zoneSettings.m_outsideWeight * (100 + diffPercent) / 100;
            } else if (zoneSettings.m_insideWeight > 10) {
                //zoneSettings.m_insideWeight  = zoneSettings.m_insideWeight * (100 - diffPercent / 3) / 100;
                //zoneSettings.m_outsideWeight = zoneSettings.m_outsideWeight * (100 - diffPercent / 3) / 100;
            }

            if (diff > 10 && diffPercent > 5)
                done = false;

            if (m_extraLogging)
                m_logOutput << m_indent << "refine step # " << i << " [" << tileZone.m_id << "]: "
                            << "int. S=" << intendedArea
                            << ", prev S=" << prevArea
                            << ", placed S=" << placedArea
                            << ", next S=" << correctionArea
                            << ", next r=" << correctionRadius
                            << ", next W=" << zoneSettings.m_outsideWeight
                            << " diff=" << diff << " (" << diffPercent << " %)\n";
        }
        if (done) {
            m_logOutput << m_indent << "Finished area refinement successfully, no deficit in areas! \n";
            break;
        }
        if (m_extraLogging)
            m_logOutput << "\n";
        splitRegions = m_tileContainer.m_all.splitByKExt(settings);
    }

    for (auto& tileZone : tileZones) {
        tileZone.m_area.m_innerArea = std::move(splitRegions[tileZone.m_index]);

        tileZone.m_area.makeEdgeFromInnerArea();
        /*
        for (auto* tile : tileZone.m_area.m_innerEdge) {
            const bool eT        = tileZone.m_area.m_innerArea.contains(tile->m_neighborT);
            const bool eL        = tileZone.m_area.m_innerArea.contains(tile->m_neighborL);
            const bool eR        = tileZone.m_area.m_innerArea.contains(tile->m_neighborR);
            const bool eB        = tileZone.m_area.m_innerArea.contains(tile->m_neighborB);
            const int  sameCount = eT + eL + eR + eB;
            if (sameCount == 2) {
                if ((eT && eB) || (eR && eL)) {
                    tileZone.m_area.m_innerArea.erase(tile);
                }
            }
        }
        tileZone.m_area.m_innerArea.eraseExclaves(false);*/
        tileZone.m_area.makeEdgeFromInnerArea();

        for (auto* tile : tileZone.m_area.m_innerArea)
            tile->m_zone = &tileZone;
        tileZone.m_centroid = tileZone.m_area.m_innerArea.makeCentroid(true);

        m_logOutput << m_indent << "zone [" << tileZone.m_id << "] areaDeficit=" << tileZone.getAreaDeficit() << "\n";
    }
}

MapGuardList SegmentHelper::makeBorders(std::vector<TileZone>& tileZones)
{
    MapGuardList guards;
    using ZKey = std::pair<TileZone*, TileZone*>;
    std::map<ZKey, MapTileRegion> borderTiles;

    auto makeKey = [](TileZone& f, TileZone& s) -> ZKey {
        std::pair key{ &f, &s };
        if (f.m_index > s.m_index)
            key = std::pair{ &s, &f };
        return key;
    };

    std::map<std::string, TileZone*> zoneIndex;
    for (auto& tileZone : tileZones) {
        zoneIndex[tileZone.m_id] = &tileZone;
    }

    auto findZoneById = [&zoneIndex](const std::string& id) -> TileZone& {
        auto it = zoneIndex.find(id);
        if (it == zoneIndex.end())
            throw std::runtime_error("Invalid zone id:" + id);
        return *(it->second);
    };

    MapTileRegion allBorderNet;

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
            allBorderNet.insert(twoSideBorder);
        }
    }

    // generate blocked tiles

    for (auto& tileZone : tileZones) {
        tileZone.m_protectionBorder   = tileZone.m_area.m_innerArea.makeInnerEdge(true).intersectWith(allBorderNet);
        tileZone.m_needPlaceObstacles = tileZone.m_protectionBorder;

        const TileZone::TileIntMapping costs = tileZone.makeMoveCosts(false);

        std::set<MapTilePtr> remaining, completed;

        for (auto tile : tileZone.m_protectionBorder)
            completed.insert(tile);

        for (auto tile : tileZone.m_area.m_innerArea) {
            remaining.insert(tile);
        }
        const int borderRadius = 2;

        auto resultByDistance = TileZone::computeDistances(costs, completed, remaining, borderRadius * 100);

        MapTilePtrList roadTiles;
        MapTilePtrList segmentTiles;

        for (const auto& [distance, area] : resultByDistance) {
            if (distance <= (borderRadius - 1) * 100)
                tileZone.m_needPlaceObstacles.insert(area);
            else
                tileZone.m_needPlaceObstaclesTentative.insert(area);
        }

        tileZone.m_innerAreaUsable.m_innerArea = tileZone.m_area.m_innerArea;
        tileZone.m_innerAreaUsable.m_innerArea.erase(tileZone.m_needPlaceObstacles);
        tileZone.m_innerAreaUsable.m_innerArea.erase(tileZone.m_needPlaceObstaclesTentative);
        tileZone.m_innerAreaUsable.makeEdgeFromInnerArea();

        auto bottomLine = tileZone.m_innerAreaUsable.getBottomEdge();
        tileZone.m_innerAreaUsable.m_innerArea.erase(bottomLine);
        tileZone.m_innerAreaUsable.makeEdgeFromInnerArea();
    }

    MapTileRegion connectionUnblockableCells;

    for (const auto& [_, connection] : m_map.m_template.m_connections) {
        auto&          tileZoneFrom = findZoneById(connection.m_from);
        auto&          tileZoneTo   = findZoneById(connection.m_to);
        auto           key          = makeKey(tileZoneFrom, tileZoneTo);
        MapTileRegion& border       = borderTiles[key];
        if (border.empty()) {
            throw std::runtime_error("No border between '" + connection.m_from + "' and '" + connection.m_to + "'"); // @todo: portal connections.
        }
        MapTilePtr centroid = border.makeCentroid(true);

        for (const auto& [connectionId, conPath] : connection.m_paths) {
            MapTilePtr cell = border.findClosestPoint(centroid->m_pos);

            if (m_extraLogging)
                m_logOutput << m_indent << "placing connection '" << connectionId << "' " << connection.m_from << " -> " << connection.m_to << " at " << cell->toPrintableString() << "\n";

            const bool guarded = conPath.m_guard || !conPath.m_mirrorGuard.empty();

            MapTilePtr ncell = nullptr;

            for (MapTilePtr n : cell->m_orthogonalNeighbours) {
                if (!ncell && n->m_zone != cell->m_zone) {
                    ncell = n;
                }
            }
            if (!ncell)
                throw std::runtime_error("Failed to get connection neighbour tile");

            MapTilePtr cellFrom = cell->m_zone == &tileZoneFrom ? cell : ncell;
            MapTilePtr cellTo   = cell->m_zone == &tileZoneFrom ? ncell : cell;

            auto processCell = [&conPath, &connectionId](MapTilePtr cell) {
                auto&      tileZone  = *(cell->m_zone);
                MapTilePtr cellInner = tileZone.m_innerAreaUsable.m_outsideEdge.findClosestPoint(cell->m_pos);

                tileZone.m_nodes.add(cellInner, conPath.m_road);

                tileZone.m_namedTiles[connectionId] = cellInner;

                auto roadFrom = cell->makePathTo(false, cellInner, true);
                for (auto* tile : roadFrom) {
                    tileZone.m_roads.add(tile, conPath.m_road);
                    tileZone.m_needPlaceObstacles.erase(tile);
                    tileZone.m_needPlaceObstaclesTentative.erase(tile);
                    for (auto* n : tile->m_orthogonalNeighbours) {
                        if (!tileZone.m_protectionBorder.contains(n)) {
                            tileZone.m_needPlaceObstacles.erase(n);
                            tileZone.m_needPlaceObstaclesTentative.erase(n);
                        }
                    }
                }
            };
            processCell(cellFrom);
            processCell(cellTo);

            if (guarded) {
                MapGuard guard;
                guard.m_id           = connectionId;
                guard.m_value        = conPath.m_guard;
                guard.m_mirrorFromId = conPath.m_mirrorGuard;
                guard.m_pos          = cellFrom;
                guard.m_zone         = nullptr;
                guards.push_back(guard);
            }

            MapTileRegion forErase;
            for (auto* borderCell : border) {
                if (posDistance(borderCell, cellFrom, 100) < conPath.m_radius * 100) {
                    forErase.insert(borderCell);
                }
            }
            border.erase(forErase);

            connectionUnblockableCells.insert(cell);
        }
    }
    return guards;
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

                AstarGenerator generator;
                generator.setPoints(closestTileInOrphan, largestNearest);
                auto usable = tileZone.m_innerAreaUsable.m_innerArea;
                usable.insert(closestTileInOrphan);
                usable.insert(largestNearest);
                generator.setNonCollision(std::move(usable));

                auto path = generator.findPath();

                tileZone.m_roadPotentialArea.insert(path);
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
    const TileZone::TileIntMapping costs = tileZone.makeMoveCosts();

    std::set<MapTilePtr> remaining, completed;

    for (auto tile : tileZone.m_nodes.m_byLevel[RoadLevel::Towns])
        completed.insert(tile);
    if (completed.empty()) {
        for (auto tile : tileZone.m_nodes.m_byLevel[RoadLevel::Exits])
            completed.insert(tile);
    }
    if (completed.empty())
        completed.insert(tileZone.m_centroid);

    for (auto tile : tileZone.m_innerAreaUsable.m_innerArea) {
        if (!completed.contains(tile))
            remaining.insert(tile);
    }

    auto resultByDistance = TileZone::computeDistances(costs, completed, remaining);

    MapTilePtrList roadTiles;
    MapTilePtrList segmentTiles;
    for (const auto& [distance, area] : resultByDistance) {
        for (auto* tile : area) {
            tileZone.m_distances.add(tile, distance);
            //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tile->m_pos, .m_text = std::to_string(_) });
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

    const int maxHeat = tileZone.m_rngZoneSettings.m_maxHeat;
    for (int heat = 0; heat < maxHeat; heat++) {
        chop(roadTiles, tileZone.m_heatForRoads, heat, maxHeat);
        chop(segmentTiles, tileZone.m_heatForSegments, heat, maxHeat);
        chop(roadTiles, tileZone.m_heatForAll, heat, maxHeat);
        chop(segmentTiles, tileZone.m_heatForAll, heat, maxHeat);
    }
}

}
