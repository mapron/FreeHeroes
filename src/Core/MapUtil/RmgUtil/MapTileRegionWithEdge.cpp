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
    m_innerEdge = m_innerArea;
    removeNonInnerFromInnerEdge();
}

void MapTileRegionWithEdge::removeNonInnerFromInnerEdge()
{
    MapTilePtrSortedList forErase;

    Mernel::ProfilerScope scope("make InnerEdge");
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
    m_innerEdge.erase(forErase);

    makeOutsideEdge();
}

void MapTileRegionWithEdge::makeOutsideEdge()
{
    m_outsideEdge.clear();
    m_outsideEdge.reserve(m_innerEdge.size());

    for (auto* cell : m_innerEdge) {
        for (auto* ncell : cell->neighboursList(m_diagonalGrowth)) {
            if (!m_innerArea.contains(ncell))
                m_outsideEdge.insert(ncell);
        }
    }
}

void MapTileRegionWithEdge::removeEdgeFromInnerArea()
{
    m_innerArea.erase(m_innerEdge);
}

bool MapTileRegionWithEdge::refineEdge(RefineTask task, const MapTileRegion& allowedArea, size_t index)
{
    if (task == RefineTask::RemoveHollows) {
        MapTilePtrList additional;
        for (MapTilePtr cell : m_outsideEdge) {
            if (!allowedArea.contains(cell) || (cell->m_segmentIndex > 0 && cell->m_segmentIndex != index))
                continue;
            const int adjucent = m_innerArea.contains(cell->m_neighborB)
                                 + m_innerArea.contains(cell->m_neighborT)
                                 + m_innerArea.contains(cell->m_neighborR)
                                 + m_innerArea.contains(cell->m_neighborL);

            if (adjucent >= 3) {
                cell->m_segmentIndex = index;
                additional.push_back(cell);
            }
        }
        m_innerArea.insert(additional);
    }
    if (task == RefineTask::RemoveSpikes) {
        MapTilePtrList removal;
        for (MapTilePtr cell : m_innerEdge) {
            const int adjucent = m_innerArea.contains(cell->m_neighborB)
                                 + m_innerArea.contains(cell->m_neighborT)
                                 + m_innerArea.contains(cell->m_neighborR)
                                 + m_innerArea.contains(cell->m_neighborL);

            if (adjucent <= 1) {
                cell->m_segmentIndex = 0;
                removal.push_back(cell);
            }
        }
        m_innerArea.erase(removal);
    }
    if (task == RefineTask::Expand) {
        MapTilePtrList additional;
        for (MapTilePtr cell : m_outsideEdge) {
            if (!allowedArea.contains(cell) || (cell->m_segmentIndex > 0 && cell->m_segmentIndex != index))
                continue;

            cell->m_segmentIndex = index;
            additional.push_back(cell);
        }
        m_innerArea.insert(additional);
    }

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
    auto segments = this->m_innerEdge.splitByFloodFill(true, cellStart);
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

MapTileRegionWithEdge MapTileRegionWithEdge::getInnerBorderNet(const std::vector<MapTileRegionWithEdge>& areas)
{
    MapTileRegionWithEdge result;
    for (size_t i = 0; i < areas.size(); ++i) {
        const MapTileRegionWithEdge& areaX = areas[i];
        for (size_t k = i + 1; k < areas.size(); ++k) {
            const MapTileRegionWithEdge& areaY = areas[k];
            for (auto* innerCellX : areaX.m_innerEdge) {
                if (areaY.m_outsideEdge.contains(innerCellX))
                    result.m_innerArea.insert(innerCellX);
            }
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

void MapTileRegionWithEdge::decompose(MapTileContainer* tileContainer, MapTileRegion& object, MapTileRegion& obstacle, const std::string& serialized, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto*        tile             = tileContainer->m_tileIndex.at(FHPos{ x, y, 0 });
            const size_t testOffset       = x + width * y;
            const char   c                = serialized[testOffset];
            const bool   objectOccupied   = c == 'O' || c == 'X';
            const bool   obstacleOccupied = c == '-' || c == 'X';
            if (objectOccupied)
                object.insert(tile);
            if (obstacleOccupied)
                obstacle.insert(tile);
        }
    }
}

void MapTileRegionWithEdge::compose(const MapTileRegion& object, const MapTileRegion& obstacle, std::string& serialized, bool obstacleInverted, bool printable)
{
    MapTileContainer* tileContainer = nullptr;
    if (!object.empty())
        tileContainer = object[0]->m_container;
    if (!obstacle.empty())
        tileContainer = obstacle[0]->m_container;
    if (!tileContainer)
        return;

    const int z      = object.empty() ? obstacle[0]->m_pos.m_z : object[0]->m_pos.m_z;
    const int width  = tileContainer->m_width;
    const int height = tileContainer->m_height;
    serialized.clear();
    for (int y = 0; y < height; ++y) {
        if (printable)
            serialized += '"';
        for (int x = 0; x < width; ++x) {
            auto* tile = tileContainer->m_tileIndex.at(FHPos{ x, y, z });
            //const size_t testOffset = x + width * y;
            char c;

            const bool objectOccupied   = object.contains(tile);
            const bool obstacleOccupied = obstacleInverted ? !obstacle.contains(tile) : obstacle.contains(tile);
            if (objectOccupied && obstacleOccupied)
                c = 'X';
            else if (objectOccupied)
                c = 'O';
            else if (obstacleOccupied)
                c = '-';
            else
                c = '.';
            serialized += c;
        }
        if (printable)
            serialized += '"', serialized += '\n';
    }
}

}
