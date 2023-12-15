/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapTileRegionWithEdge.hpp"
#include "MapTileContainer.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>
#include <unordered_set>
#include <sstream>

namespace FreeHeroes {

void MapTileRegionWithEdge::makeEdgeFromInnerArea()
{
    std::tie(m_innerEdge, m_outsideEdge) = m_innerArea.makeInnerAndOuterEdge(m_diagonalGrowth);
}

bool MapTileRegionWithEdge::refineEdgeRemoveHollows(MapTileRegion& allowedArea)
{
    MapTilePtrSortedList additional;
    for (MapTilePtr cell : m_outsideEdge) {
        if (!allowedArea.contains(cell))
            continue;
        const int adjucent = m_innerArea.contains(cell->m_neighborB)
                             + m_innerArea.contains(cell->m_neighborT)
                             + m_innerArea.contains(cell->m_neighborR)
                             + m_innerArea.contains(cell->m_neighborL);

        if (adjucent >= 3) {
            additional.push_back(cell);
        }
    }
    allowedArea.erase(additional);
    m_innerArea.insert(additional);
    makeEdgeFromInnerArea();
    return true;
}

bool MapTileRegionWithEdge::refineEdgeRemoveSpikes(MapTileRegion& allowedArea)
{
    MapTilePtrSortedList removal;
    for (MapTilePtr cell : m_innerEdge) {
        const int adjucent = m_innerArea.contains(cell->m_neighborB)
                             + m_innerArea.contains(cell->m_neighborT)
                             + m_innerArea.contains(cell->m_neighborR)
                             + m_innerArea.contains(cell->m_neighborL);

        if (adjucent <= 1) {
            removal.push_back(cell);
        }
    }
    allowedArea.insert(removal);
    m_innerArea.erase(removal);
    makeEdgeFromInnerArea();
    return true;
}

bool MapTileRegionWithEdge::refineEdgeExpand(MapTileRegion& allowedArea)
{
    auto additional = allowedArea.intersectWith(m_outsideEdge);
    allowedArea.erase(additional);
    m_innerArea.insert(std::move(additional));
    makeEdgeFromInnerArea();
    return true;
}

bool MapTileRegionWithEdge::refineEdgeShrink(MapTileRegion& allowedArea)
{
    m_innerArea.erase(m_innerEdge);
    allowedArea.insert(m_innerEdge);
    makeEdgeFromInnerArea();
    return true;
}

MapTileRegion MapTileRegionWithEdge::getBottomEdge() const
{
    MapTileRegion result;
    result.reserve(m_innerEdge.size() / 3);
    for (MapTilePtr cell : m_innerEdge) {
        const bool hasB      = m_innerArea.contains(cell->m_neighborB);
        const bool hasBR     = m_innerArea.contains(cell->m_neighborBR);
        const bool hasBL     = m_innerArea.contains(cell->m_neighborBL);
        const int  hasBCount = hasB + hasBR + hasBL;
        if (hasBCount < 2)
            result.insert(cell);
    }
    return result;
}

MapTileRegion MapTileRegionWithEdge::floodFillDiagonalByInnerEdge(MapTilePtr cellStart) const
{
    auto segments = m_innerEdge.splitByFloodFill(true, cellStart);
    if (segments.empty())
        return {};
    return segments[0];
}

MapTileRegionWithEdgeList MapTileRegionWithEdge::makeEdgeList(const MapTileRegionList& regions)
{
    MapTileRegionWithEdgeList result(regions.size());
    for (size_t i = 0; i < regions.size(); ++i) {
        result[i].m_innerArea = regions[i];
        result[i].makeEdgeFromInnerArea();
    }
    return result;
}

MapTileRegion MapTileRegionWithEdge::getInnerBorderNet(const std::vector<MapTileRegionWithEdge>& areas)
{
    MapTileRegion result;
    for (size_t i = 0; i < areas.size(); ++i) {
        const MapTileRegionWithEdge& areaX = areas[i];
        for (size_t k = i + 1; k < areas.size(); ++k) {
            const MapTileRegionWithEdge& areaY = areas[k];
            result.insert(areaX.m_innerEdge.intersectWith(areaY.m_outsideEdge));
        }
    }
    return result;
}

MapTileRegion MapTileRegionWithEdge::getOuterBorderNet(const MapTileRegionWithEdgeList& areas)
{
    MapTileRegion result;
    for (size_t i = 0; i < areas.size(); ++i) {
        const MapTileRegionWithEdge& areaX = areas[i];
        for (size_t k = 0; k < areas.size(); ++k) {
            if (i == k)
                continue;
            const MapTileRegionWithEdge& areaY = areas[k];
            result.insert(areaX.m_outsideEdge.intersectWith(areaY.m_outsideEdge));
        }
    }
    return result;
}

std::pair<MapTileRegionWithEdge::CollisionResult, FHPos> MapTileRegionWithEdge::getCollisionShiftForObject(const MapTileRegion& object, const MapTileRegion& obstacle, bool invertObstacle)
{
    if (object.empty() || obstacle.empty())
        return std::pair{ CollisionResult::InvalidInputs, FHPos{} };

    const MapTileRegion intersection = invertObstacle ? object.diffWith(obstacle) : object.intersectWith(obstacle);
    if (intersection.empty())
        return std::pair{ CollisionResult::NoCollision, FHPos{} };

    if (intersection == object)
        return std::pair{ CollisionResult::ImpossibleShift, FHPos{} };

    const MapTilePtr collisionCentroid = intersection.makeCentroid(false);

    MapTileRegion objectWithoutObstacle = object;
    objectWithoutObstacle.erase(collisionCentroid);

    FHPos topLeftBoundary = object[0]->m_pos, rightBottomBoundary = object[0]->m_pos;
    for (auto* tile : object) {
        topLeftBoundary.m_x     = std::min(topLeftBoundary.m_x, tile->m_pos.m_x);
        topLeftBoundary.m_y     = std::min(topLeftBoundary.m_y, tile->m_pos.m_y);
        rightBottomBoundary.m_x = std::max(rightBottomBoundary.m_x, tile->m_pos.m_x);
        rightBottomBoundary.m_y = std::max(rightBottomBoundary.m_y, tile->m_pos.m_y);
    }
    FHPos     diff       = rightBottomBoundary - topLeftBoundary;
    const int width      = diff.m_x + 1;
    const int height     = diff.m_y + 1;
    const int horRadius  = width / 2; // 1x1 => 0, 2x2 -> 1, 3x3 -> 1 , 4x4 -> 2
    const int vertRadius = height / 2;

    const MapTilePtr objectCentroid = objectWithoutObstacle.makeCentroid(false);

    FHPos collisionOffset = objectCentroid->m_pos - collisionCentroid->m_pos;
    int   cx              = collisionOffset.m_x;
    int   cy              = collisionOffset.m_y;
    if (cx == 0 && cy == 0)
        return std::pair{ CollisionResult::ImpossibleShift, FHPos{} };

    if (cx > 0) {
        if (horRadius > 1)
            cx = horRadius - cx + 1;
    }
    if (cx < 0) {
        if (horRadius > 1)
            cx = -horRadius - cx - 1;
    }
    if (cy > 0) {
        if (vertRadius > 1)
            cy = vertRadius - cy + 1;
    }
    if (cy < 0) {
        if (horRadius > 1)
            cy = -vertRadius - cy - 1;
    }
    return std::pair{ CollisionResult::HasShift, FHPos{ cx, cy } };
}

}
