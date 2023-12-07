/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapTileArea.hpp"
#include "KMeans.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>
#include <unordered_set>
#include <sstream>

namespace FreeHeroes {

void MapTileArea::makeEdgeFromInnerArea()
{
    //Mernel::ProfilerScope scope("makeEdgeFromInnerArea");

    m_innerEdge = m_innerArea;
    removeNonInnerFromInnerEdge();
}

void MapTileArea::removeNonInnerFromInnerEdge()
{
    //Mernel::ProfilerScope scope("removeNonInnerFromInnerEdge");
    auto edge = m_innerEdge;
    m_innerEdge.clear();

    for (MapTilePtr cell : edge) {
        if (m_diagonalGrowth) {
            if (m_innerArea.contains(cell->m_neighborB)
                && m_innerArea.contains(cell->m_neighborT)
                && m_innerArea.contains(cell->m_neighborR)
                && m_innerArea.contains(cell->m_neighborL)
                && m_innerArea.contains(cell->m_neighborTL)
                && m_innerArea.contains(cell->m_neighborTR)
                && m_innerArea.contains(cell->m_neighborBL)
                && m_innerArea.contains(cell->m_neighborBR))
                continue;
        } else {
            if (m_innerArea.contains(cell->m_neighborB)
                && m_innerArea.contains(cell->m_neighborT)
                && m_innerArea.contains(cell->m_neighborR)
                && m_innerArea.contains(cell->m_neighborL))
                continue;
        }

        m_innerEdge.insert(cell);
    }

    makeOutsideEdge();
}

void MapTileArea::makeOutsideEdge()
{
    m_outsideEdge.clear();
    m_outsideEdge.reserve(m_innerEdge.size());
    auto predicate = [this](MapTilePtr cell) {
        return cell && !m_innerArea.contains(cell);
    };
    for (auto* cell : m_innerEdge) {
        for (auto* ncell : cell->neighboursList(m_diagonalGrowth))
            addIf(m_outsideEdge, ncell, predicate);
    }
}

void MapTileArea::removeEdgeFromInnerArea()
{
    //Mernel::ProfilerScope scope("removeEdgeFromInnerArea");
    m_innerArea.erase(m_innerEdge);
}

bool MapTileArea::refineEdge(RefineTask task, const MapTileRegion& allowedArea, size_t index)
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

MapTileRegion MapTileArea::getBottomEdge() const
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

MapTileArea MapTileArea::floodFillDiagonalByInnerEdge(MapTilePtr cellStart) const
{
    MapTileArea innerEdgeArea;
    innerEdgeArea.m_innerArea = this->m_innerEdge;

    auto segments = innerEdgeArea.splitByFloodFill(true, cellStart);
    if (segments.empty())
        return {};
    return segments[0];
}

std::vector<MapTileArea> MapTileArea::splitByFloodFill(bool useDiag, MapTilePtr hint) const
{
    if (m_innerArea.empty())
        return {};
    //Mernel::ProfilerScope scope("splitByFloodFill");

    std::vector<MapTileArea> result;
    MapTileArea              current;

    std::unordered_set<MapTilePtr> visited;
    MapTilePtrList                 currentEdge;
    auto                           addToCurrent = [&current, &visited, &currentEdge, *this](MapTilePtr cell) {
        if (visited.contains(cell))
            return;
        if (!m_innerArea.contains(cell))
            return;
        visited.insert(cell);
        current.m_innerArea.insert(cell);
        currentEdge.push_back(cell);
    };
    std::set<MapTilePtr> remain(m_innerArea.begin(), m_innerArea.end());
    if (hint) {
        if (!remain.contains(hint))
            throw std::runtime_error("Invalid tile hint provided");
    }

    while (!remain.empty()) {
        MapTilePtr startCell = hint ? hint : *remain.begin();
        hint                 = nullptr;
        addToCurrent(startCell);

        while (!currentEdge.empty()) {
            MapTilePtrList nextEdge = currentEdge;
            currentEdge.clear();

            for (MapTilePtr edgeCell : nextEdge) {
                MapTilePtrList neighbours = edgeCell->neighboursList(useDiag);
                for (auto growedCell : neighbours) {
                    addToCurrent(growedCell);
                }
            }
        }

        current.makeEdgeFromInnerArea();
        for (auto* tile : current.m_innerArea)
            remain.erase(tile);
        result.push_back(std::move(current));
    }

    return result;
}

std::vector<MapTileArea> MapTileArea::splitByMaxArea(std::ostream& os, size_t maxArea, bool repulse) const
{
    std::vector<MapTileArea> result;
    size_t                   zoneArea = m_innerArea.size();
    if (!zoneArea)
        return result;

    const size_t k = (zoneArea + maxArea + 1) / maxArea;

    return splitByK(os, k, repulse);
}

std::vector<MapTileArea> MapTileArea::splitByK(std::ostream& os, size_t k, bool repulse) const
{
    std::vector<MapTileArea> result;
    size_t                   zoneArea = m_innerArea.size();
    if (!zoneArea)
        return result;

    if (k == 1) {
        result.push_back(*this);
    } else {
        Mernel::ProfilerScope scope("segmentation");

        KMeansSegmentation seg;
        seg.m_points.reserve(zoneArea);
        for (auto* cell : m_innerArea) {
            seg.m_points.push_back({ cell });
        }
        seg.m_iters = 30;
        seg.initEqualCentoids(k);
        seg.run(os);
        std::vector<KMeansSegmentation::Cluster*> clusters;
        for (KMeansSegmentation::Cluster& cluster : seg.m_clusters)
            clusters.push_back(&cluster);
        if (repulse) {
            std::vector<KMeansSegmentation::Cluster*> clustersSorted;
            auto*                                     curr = clusters.back();
            clusters.pop_back();
            clustersSorted.push_back(curr);
            while (!clusters.empty()) {
                auto it = std::max_element(clusters.begin(), clusters.end(), [curr](KMeansSegmentation::Cluster* l, KMeansSegmentation::Cluster* r) {
                    return posDistance(curr->m_centroid, l->m_centroid) < posDistance(curr->m_centroid, r->m_centroid);
                });
                curr    = *it;
                clustersSorted.push_back(curr);
                clusters.erase(it);
            }
            clusters = clustersSorted;
        }

        for (KMeansSegmentation::Cluster* cluster : clusters) {
            MapTileRegion zoneSeg;
            zoneSeg.reserve(cluster->m_points.size());
            for (auto& point : cluster->m_points)
                zoneSeg.insert(point->m_pos);

            assert(zoneSeg.size() > 0);
            result.push_back(MapTileArea{ .m_innerArea = std::move(zoneSeg) });
        }
    }
    for (auto& area : result)
        area.makeEdgeFromInnerArea();
    return result;
}

MapTileArea MapTileArea::getInnerBorderNet(const std::vector<MapTileArea>& areas)
{
    MapTileArea result;
    for (size_t i = 0; i < areas.size(); ++i) {
        const MapTileArea& areaX = areas[i];
        for (size_t k = i + 1; k < areas.size(); ++k) {
            const MapTileArea& areaY = areas[k];
            for (auto* innerCellX : areaX.m_innerEdge) {
                if (areaY.m_outsideEdge.contains(innerCellX))
                    result.m_innerArea.insert(innerCellX);
            }
        }
    }
    return result;
}

}
