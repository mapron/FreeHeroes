/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "RoadHelper.hpp"
#include "KMeans.hpp"
#include "AstarGenerator.hpp"
#include "../FHMap.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>

namespace FreeHeroes {

RoadHelper::RoadHelper(FHMap&                        map,
                       MapTileContainer&             tileContainer,
                       Core::IRandomGenerator* const rng,
                       std::ostream&                 logOutput)
    : m_map(map)
    , m_tileContainer(tileContainer)
    , m_rng(rng)
    , m_logOutput(logOutput)
{
}

void RoadHelper::makeBorders(std::vector<TileZone>& tileZones)
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
            twoSideBorder.doSort();
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
        FHPos borderCentroid = TileZone::makeCentroid(border); // switch to k-means when we need more than one connection.

        auto       it   = std::min_element(border.cbegin(), border.cend(), [&borderCentroid](MapTilePtr l, MapTilePtr r) {
            return posDistance(borderCentroid, l->m_pos) < posDistance(borderCentroid, r->m_pos);
        });
        MapTilePtr cell = (*it);
        cell->m_zone->m_roadNodesHighPriority.insert(cell);

        const bool guarded = connections.m_guard || !connections.m_mirrorGuard.empty();

        MapTilePtr ncellFound = nullptr;

        for (MapTilePtr ncell : cell->m_allNeighbours) {
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

        MapTileRegion forErase({ cell, ncellFound, cell->m_neighborT, ncellFound->m_neighborT, cell->m_neighborL, ncellFound->m_neighborL });
        forErase.doSort();
        border.erase(forErase);
        border.doSort();

        connectionUnblockableCells.insert(cell);
    }
    connectionUnblockableCells.doSort();
    MapTileRegion noExpandTiles;
    for (MapTile& tile : m_tileContainer.m_tiles) {
        for (auto* cell : connectionUnblockableCells) {
            if (posDistance(tile.m_pos, cell->m_pos) < 4)
                noExpandTiles.insert(&tile);
        }
    }
    noExpandTiles.doSort();
    MapTileRegion needBeBlocked;
    MapTileRegion tentativeBlocked;
    for (const auto& [key, border] : borderTiles) {
        needBeBlocked.insert(border);
    }
    needBeBlocked.doSort();
    for (const auto& [key, border] : borderTiles) {
        for (auto* cell : border) {
            for (MapTilePtr ncell : cell->m_allNeighbours) {
                if (needBeBlocked.contains(ncell))
                    continue;
                if (noExpandTiles.contains(ncell))
                    continue;
                tentativeBlocked.insert(ncell);
            }
        }
    }

    tentativeBlocked.doSort();

    for (auto& tileZone : tileZones) {
        tileZone.m_innerAreaUsable = {};
        for (auto* cell : tileZone.m_area.m_innerArea) {
            if (needBeBlocked.contains(cell))
                tileZone.m_needBeBlocked.insert(cell);
            if (tentativeBlocked.contains(cell))
                tileZone.m_tentativeBlocked.insert(cell);
        }
        tileZone.m_needBeBlocked.doSort();
        tileZone.m_tentativeBlocked.doSort();
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

void RoadHelper::placeRoads(TileZone& tileZone)
{
    //Mernel::ProfilerScope topScope("placeRoads");

    tileZone.m_innerAreaSegments = tileZone.m_innerAreaUsable.splitByMaxArea(m_logOutput, tileZone.m_rngZoneSettings.m_segmentAreaSize, true);
    auto borderNet               = MapTileArea::getInnerBorderNet(tileZone.m_innerAreaSegments);

    tileZone.m_roadNodes.insert(tileZone.m_roadNodesHighPriority);
    tileZone.m_roadNodes.doSort();

    for (size_t i = 0; auto& area : tileZone.m_innerAreaSegments) {
        i++;
        area.makeEdgeFromInnerArea();
        area.removeEdgeFromInnerArea();

        for (auto* cell : area.m_innerEdge) {
            cell->m_segmentIndex = i;
            if (borderNet.contains(cell))
                tileZone.m_innerAreaSegmentsRoads.insert(cell);
        }

        for (auto* cell : area.m_innerArea) {
            cell->m_segmentIndex = i;
        }
    }

    for (MapTileArea& area : tileZone.m_innerAreaSegments) {
        for (MapTilePtr cell : area.m_innerEdge) {
            std::set<std::pair<TileZone*, size_t>> neighAreaBorders;
            if (!cell->m_neighborB || !cell->m_neighborT || !cell->m_neighborL || !cell->m_neighborR)
                neighAreaBorders.insert(std::pair<TileZone*, size_t>{ nullptr, 0 });

            for (MapTilePtr cellAdj : cell->m_allNeighbours) {
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
                    tileZone.m_roadNodes.doSort();
                }
            }
        }
    }

    tileZone.m_innerAreaSegmentsRoads.insert(tileZone.m_roadNodes);
    tileZone.m_innerAreaSegmentsRoads.doSort();

    {
        // sometimes we have 'deadends' caused by  if (!hasNeighbourInRoads) condition above.
        // Try to bring some connections back by this intrinsic.
        MapTileRegion roadNeighbours;
        for (MapTilePtr cell : tileZone.m_roadNodes) {
            for (auto* ncell : cell->m_allNeighboursWithDiag) {
                if (tileZone.m_innerAreaUsable.contains(ncell))
                    roadNeighbours.insert(ncell);
            }
        }
        roadNeighbours.doSort();
        for (MapTilePtr cell : roadNeighbours) {
            const bool roadB = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborB);
            const bool roadT = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborT);
            const bool roadR = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborR);
            const bool roadL = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborL);

            const bool roadTL = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborTL);
            const bool roadTR = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborTR);
            const bool roadBL = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborBL);
            const bool roadBR = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborBR);

            const bool roadX = tileZone.m_innerAreaSegmentsRoads.contains(cell);

            const int topRowRoads = roadT + roadTL + roadTR;
            const int botRowRoads = roadB + roadBL + roadBR;
            const int rigRowRoads = roadR + roadTR + roadBR;
            const int lefRowRoads = roadL + roadBL + roadTL;

            const int verCenterRoads = roadT + roadB + roadX;
            const int horCenterRoads = roadR + roadL + roadX;

            const bool addForVert1 = verCenterRoads == 0 && rigRowRoads > 0 && lefRowRoads > 0 && cell->m_neighborB && cell->m_neighborT;
            const bool addForHorz1 = horCenterRoads == 0 && topRowRoads > 0 && botRowRoads > 0 && cell->m_neighborR && cell->m_neighborL;

            if (addForVert1 || addForHorz1) {
                tileZone.m_innerAreaSegmentsRoads.insert(cell);
                tileZone.m_innerAreaSegmentsRoads.doSort();
            }
        }
    }

    {
        // correct zigzag road tiles.
        for (MapTilePtr cell : tileZone.m_innerAreaUsable.m_innerArea) {
            const bool roadB = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborB);
            const bool roadT = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborT);
            const bool roadR = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborR);
            const bool roadL = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborL);

            const bool roadTL = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborTL);
            const bool roadTR = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborTR);
            const bool roadBL = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborBL);
            const bool roadBR = tileZone.m_innerAreaSegmentsRoads.contains(cell->m_neighborBR);

            const bool roadX = tileZone.m_innerAreaSegmentsRoads.contains(cell);

            const int crossRoads = roadB + roadT + roadR + roadL;
            const int diagRoads  = roadTL + roadTR + roadBL + roadBR;

            if (!roadX && crossRoads >= 3 && diagRoads == 0) {
                tileZone.m_innerAreaSegmentsRoads.insert(cell);
                tileZone.m_innerAreaSegmentsRoads.doSort();
            }
        }
    }

    std::vector<MapTilePtr> unconnectedRoadNodesAll(tileZone.m_roadNodes.cbegin(), tileZone.m_roadNodes.cend());
    if (unconnectedRoadNodesAll.size() <= 1)
        return;

    tileZone.m_roadNodesHighPriority.doSort();
    {
        //Mernel::ProfilerScope scope("making extra innerAreaSegments");
        for (auto* townCell : tileZone.m_roadNodesHighPriority) {
            MapTileRegion allPossibleRoads = tileZone.m_innerAreaSegmentsRoads;

            allPossibleRoads.erase(townCell);
            allPossibleRoads.doSort();
            if (allPossibleRoads.empty())
                break;

            {
                MapTileArea roadsArea;
                roadsArea.m_innerEdge     = tileZone.m_innerAreaSegmentsRoads;
                auto roadCellsNearTheTown = roadsArea.floodFillDiagonalByInnerEdge(townCell);

                auto otherRoadNodes = tileZone.m_roadNodes;
                otherRoadNodes.erase(townCell);
                otherRoadNodes.doSort();
                bool okNear = false;
                for (auto* nearCell : roadCellsNearTheTown.m_innerArea) {
                    if (otherRoadNodes.contains(nearCell)) {
                        okNear = true;
                        break;
                    }
                }
                if (!okNear) {
                    allPossibleRoads.erase(roadCellsNearTheTown.m_innerArea);
                }
            }
            allPossibleRoads.doSort();

            auto it = std::min_element(allPossibleRoads.cbegin(), allPossibleRoads.cend(), [townCell](MapTilePtr l, MapTilePtr r) {
                return posDistance(townCell->m_pos, l->m_pos) < posDistance(townCell->m_pos, r->m_pos);
            });

            MapTilePtr closest = *it;
            auto       path    = aStarPath(tileZone, townCell, closest, true);
            tileZone.m_innerAreaSegmentsRoads.insert(path);
            tileZone.m_innerAreaSegmentsRoads.doSort();
        }
    }

    {
        // unite separate networks
        MapTileArea roadNet;
        roadNet.m_innerArea    = tileZone.m_innerAreaSegmentsRoads;
        auto disconnectedParts = roadNet.splitByFloodFill(true);
        if (disconnectedParts.size() > 1) {
            std::sort(disconnectedParts.begin(), disconnectedParts.end(), [](const MapTileArea& r, const MapTileArea& l) {
                return r.m_innerArea.size() < l.m_innerArea.size();
            });
            MapTileArea mainPart = disconnectedParts.back();
            disconnectedParts.pop_back();
            auto  mainCentroid     = TileZone::makeCentroid(mainPart.m_innerArea);
            auto* mainCentroidTile = m_tileContainer.m_tileIndex.at(mainCentroid);
            for (const MapTileArea& part : disconnectedParts) {
                if (part.m_innerArea.size() <= 2)
                    continue;
                auto       it      = std::min_element(part.m_innerArea.cbegin(), part.m_innerArea.cend(), [mainCentroidTile](MapTilePtr l, MapTilePtr r) {
                    return posDistance(mainCentroidTile, l) < posDistance(mainCentroidTile, r);
                });
                MapTilePtr closest = (*it);

                auto it2 = std::min_element(mainPart.m_innerArea.cbegin(), mainPart.m_innerArea.cend(), [closest](MapTilePtr l, MapTilePtr r) {
                    return posDistance(closest, l) < posDistance(closest, r);
                });

                MapTilePtr closestMain = (*it2);

                auto path = aStarPath(tileZone, closest, closestMain, true);
                tileZone.m_innerAreaSegmentsRoads.insert(path);
            }

            tileZone.m_innerAreaSegmentsRoads.doSort();
        }
    }

    std::vector<MapTilePtrList> unconnectedRoadNodesByLevel;
    unconnectedRoadNodesByLevel.resize(3);

    for (MapTilePtr node : unconnectedRoadNodesAll) {
        const int roadLevel = tileZone.getRoadLevel(node);
        assert(roadLevel >= 0);
        unconnectedRoadNodesByLevel[roadLevel].push_back(node);
    }

    MapTileRegion           pathAsRegion;
    std::vector<MapTilePtr> connected;
    for (int level = 0; level < 3; ++level) {
        MapTilePtrList& unconnectedRoadNodes = unconnectedRoadNodesByLevel[level];
        std::sort(unconnectedRoadNodes.begin(), unconnectedRoadNodes.end(), [](MapTilePtr l, MapTilePtr r) {
            return l->m_pos < r->m_pos;
        });

        if (unconnectedRoadNodes.empty())
            continue;

        if (connected.empty()) {
            MapTilePtr cell = unconnectedRoadNodes.back();
            unconnectedRoadNodes.pop_back();
            connected.push_back(cell);
        }

        //Mernel::ProfilerScope scope("final road lookup");

        while (!unconnectedRoadNodes.empty()) {
            MapTilePtr cell = unconnectedRoadNodes.back();
            unconnectedRoadNodes.pop_back();

            std::vector<MapTilePtr> connectedTmp = connected;

            std::sort(connectedTmp.begin(), connectedTmp.end(), [cell, &tileZone](MapTilePtr l, MapTilePtr r) {
                const int     roadLevelL  = tileZone.getRoadLevel(l);
                const int     roadLevelR  = tileZone.getRoadLevel(r);
                const int64_t lBorderMult = roadLevelL + 1;
                const int64_t rBorderMult = roadLevelR + 1;

                return posDistance(cell->m_pos, l->m_pos) * lBorderMult < posDistance(cell->m_pos, r->m_pos) * rBorderMult;
            });
            if (connectedTmp.size() > 4)
                connectedTmp.resize(4);

            std::vector<MapTilePtrList> pathAlternatives;
            for (MapTilePtr closest : connectedTmp) {
                auto path = aStarPath(tileZone, cell, closest, false);
                if (!path.empty())
                    pathAlternatives.push_back(std::move(path));
            }
            if (pathAlternatives.empty())
                continue;

            std::sort(pathAlternatives.begin(), pathAlternatives.end(), [](const MapTilePtrList& l, const MapTilePtrList& r) {
                return l.size() < r.size();
            });
            auto path = pathAlternatives[0];

            connected.push_back(cell);
            pathAsRegion.insert(path);

            placeRoad(tileZone, std::move(path), level);
        }
    }

    pathAsRegion.doSort();
    for (MapTileArea& area : tileZone.m_innerAreaSegments) {
        area.m_innerArea.erase(pathAsRegion);
        area.m_innerArea.doSort();
    }
}

void RoadHelper::placeRoad(TileZone& tileZone, const MapTilePtrList& tileList, int level)
{
    if (tileList.empty())
        return;

    MapTileArea filtered;
    for (auto* cell : tileList) {
        if (!tileZone.m_placedRoads.contains(cell))
            filtered.m_innerArea.insert(cell);
    }
    filtered.doSort();
    auto         segments  = filtered.splitByFloodFill(false);
    const size_t minLength = (segments.size() == 1 && level < 2) ? 1 : (level == 0 ? 3 : 7);
    for (auto& seg : segments) {
        auto roadLength = seg.m_innerArea.size();
        if (roadLength < minLength)
            continue;

        MapTilePtrList filteredPath(seg.m_innerArea.cbegin(), seg.m_innerArea.cend());
        placeRoad(std::move(filteredPath), level);
    }

    tileZone.m_placedRoads.insert(tileList);
    tileZone.m_placedRoads.doSort();
}

void RoadHelper::placeRoad(const MapTilePtrList& tileList, int level)
{
    std::vector<FHPos> path;
    path.reserve(tileList.size());
    for (auto* cell : tileList)
        path.push_back(cell->m_pos);

    placeRoadPath(std::move(path), level);
}

void RoadHelper::placeRoadPath(std::vector<FHPos> path, int level)
{
    if (path.empty() || level < 0)
        return;
    FHRoad road;
    auto&  settings = m_map.m_template.m_userSettings;
    road.m_type     = settings.m_defaultRoad;
    if (level == 1)
        road.m_type = settings.m_innerRoad;
    if (level == 2)
        road.m_type = settings.m_borderRoad;

    if (path.size() <= 2 && level == 1) {
        if (settings.m_defaultRoad != settings.m_innerRoad && settings.m_innerRoad != FHRoadType::Invalid) {
            road.m_type = settings.m_defaultRoad;
        }
    }

    if (road.m_type == FHRoadType::Invalid)
        return;

    road.m_tiles = std::move(path);
    m_map.m_roads.push_back(std::move(road));
}

MapTilePtrList RoadHelper::aStarPath(TileZone& zone, MapTilePtr start, MapTilePtr end, bool allTiles)
{
    //Mernel::ProfilerScope scope("aStarPath");

    AstarGenerator generator;
    generator.setPoints(start, end);

    MapTileRegion nonCollideSet;
    if (allTiles) {
        nonCollideSet = zone.m_innerAreaUsable.m_innerArea;
        nonCollideSet.insert(zone.m_innerAreaBottomLine);
    } else {
        nonCollideSet = zone.m_innerAreaSegmentsRoads;
    }
    nonCollideSet.doSort();
    nonCollideSet.erase(zone.m_needBeBlocked);
    nonCollideSet.doSort();

    generator.setNonCollision(std::move(nonCollideSet));

    auto path = generator.findPath();
    if (!generator.isSuccess()) {
        return {};
    }

    const auto pathCopy = path;
    for (size_t i = 1; i < pathCopy.size(); i++) {
        MapTilePtr prev  = pathCopy[i - 1];
        MapTilePtr cur   = pathCopy[i];
        const int  prevX = prev->m_pos.m_x;
        const int  prevY = prev->m_pos.m_y;
        const int  curX  = cur->m_pos.m_x;
        const int  curY  = cur->m_pos.m_y;
        if (prevX != curX && prevY != curY) // diagonal
        {
            MapTilePtr extra1 = prev;
            MapTilePtr extra2 = prev;

            // from TL to BR
            if (prevX < curX && prevY < curY) {
                extra1 = prev->m_neighborB;
                extra2 = prev->m_neighborR;
            }

            // from BL to TR
            if (prevX < curX && prevY > curY) {
                extra1 = prev->m_neighborT;
                extra2 = prev->m_neighborR;
            }

            // from TR to BL
            if (prevX > curX && prevY < curY) {
                extra1 = prev->m_neighborB;
                extra2 = prev->m_neighborL;
            }

            // from BR to TL
            if (prevX > curX && prevY > curY) {
                extra1 = prev->m_neighborT;
                extra2 = prev->m_neighborL;
            }

            if (zone.m_placedRoads.contains(extra1))
                path.push_back(extra1);
            else if (zone.m_placedRoads.contains(extra2))
                path.push_back(extra2);
            else if (!zone.m_needBeBlocked.contains(extra1))
                path.push_back(extra1);
            else if (!zone.m_needBeBlocked.contains(extra2))
                path.push_back(extra2);
        }
    }

    return path;
}

}
