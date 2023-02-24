/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TileZone.hpp"

#include "TemplateUtils.hpp"

#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes {

FHPos TileZone::makeCentroid(const MapTileRegion& region)
{
    int64_t sumX = 0, sumY = 0;
    int64_t size = region.size();
    for (const auto* cell : region) {
        sumX += cell->m_pos.m_x;
        sumY += cell->m_pos.m_y;
    }
    sumX /= size;
    sumY /= size;
    int z = 0;
    return FHPos{ static_cast<int>(sumX), static_cast<int>(sumY), z };
}

void TileZone::estimateCentroid()
{
    m_centroid = m_tileContainer->m_tileIndex.at(makeCentroid(m_area.m_innerArea));
}

void TileZone::readFromMap()
{
    //Mernel::ProfilerScope scope("readFromMap");

    m_area.m_innerArea.clear();
    for (auto& cell : m_tileContainer->m_tiles) {
        if (cell.m_zone == this)
            m_area.m_innerArea.insert(&cell);
    }
    m_area.m_innerArea.doSort();

    auto floodRegions  = m_area.splitByFloodFill(false, m_centroid);
    m_area.m_innerArea = floodRegions[0].m_innerArea;
    for (size_t i = 1; i < floodRegions.size(); i++) {
        for (auto* tile : floodRegions[i].m_innerArea)
            tile->m_zone = nullptr;
    }

    m_area.makeEdgeFromInnerArea();
}

void TileZone::readFromMapIfDirty()
{
    if (!m_tileContainer->m_dirtyZones.contains(this))
        return;
    m_tileContainer->m_dirtyZones.erase(this);
    readFromMap();
}

bool TileZone::tryGrowOnceToNeighbour(size_t limit, TileZone* prioritized)
{
    MapTileRegion lastGrowed;
    const bool    result = m_area.tryGrowOnce(lastGrowed, [this](MapTilePtr cell) {
        return cell->m_zone != this;
    });
    if (!result)
        return false;

    std::vector<MapTilePtr> nextEdge(lastGrowed.cbegin(), lastGrowed.cend());
    if (nextEdge.size() > limit && limit > 0) {
        std::nth_element(nextEdge.begin(), nextEdge.begin() + limit, nextEdge.end(), [this, prioritized](MapTilePtr l, MapTilePtr r) {
            if (prioritized) {
                if (l->m_zone == prioritized && r->m_zone != prioritized)
                    return true;
                if (l->m_zone != prioritized && r->m_zone == prioritized)
                    return false;
            }
            if (!l->m_zone && r->m_zone)
                return true;
            if (l->m_zone && !r->m_zone)
                return false;
            return posDistance(l->m_pos, this->m_centroid->m_pos) < posDistance(r->m_pos, this->m_centroid->m_pos);
        });
        nextEdge.resize(limit);
    }

    for (MapTilePtr cell : nextEdge) {
        if (cell->m_zone && cell->m_zone != this) {
            m_tileContainer->m_dirtyZones.insert(cell->m_zone);
        }
        cell->m_zone = this;

        m_area.m_innerEdge.insert(cell);
        m_area.m_innerArea.insert(cell);
    }
    m_area.removeNonInnerFromInnerEdge();
    return true;
}

bool TileZone::tryGrowOnceToUnzoned()
{
    MapTileRegion lastGrowed;
    const bool    result = m_area.tryGrowOnce(lastGrowed, [](MapTilePtr cell) {
        return cell->m_zone == nullptr;
    });
    if (!result)
        return false;

    for (MapTilePtr cell : lastGrowed) {
        cell->m_zone = this;

        m_area.m_innerEdge.insert(cell);
        m_area.m_innerArea.insert(cell);
    }
    m_area.removeNonInnerFromInnerEdge();
    return true;
}

void TileZone::fillDeficit(int thresholdPercent, TileZone* prioritized)
{
    while (true) {
        if (getAreaDeficitPercent() < thresholdPercent)
            break;
        if (!tryGrowOnceToNeighbour(10, prioritized))
            break;
    }
}

void TileZone::fillUnzoned()
{
    while (tryGrowOnceToUnzoned()) {
        ;
    }
}

int TileZone::getRoadLevel(MapTilePtr node) const
{
    const bool isHigh = m_roadNodesHighPriority.contains(node);
    if (isHigh || m_innerAreaTowns.contains(node))
        return 0;

    const bool isBorder = m_innerAreaUsable.m_innerEdge.contains(node);
    if (isBorder)
        return 2;

    if (m_roadNodes.contains(node))
        return 1;

    return -1;
}

}
