/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapTileRegion.hpp"
#include "MapTile.hpp"
#include "MapTileContainer.hpp"

#include <map>

namespace FreeHeroes {

namespace {
struct KMeansData {
    struct Cluster {
        FHPos                m_centroid;
        FHPos                m_centerMass;
        MapTilePtrSortedList m_points;
        size_t               m_pointsCount = 0;
        int                  m_radius      = 100;
        int                  m_speed       = 100;

        int64_t distanceTo(MapTilePtr point) const
        {
            return posDistance(point->m_pos, m_centroid) * 1000 / m_radius;
        }

        void updateCentroid()
        {
            if (m_speed == 0)
                return;

            m_centroid = posMidPoint(m_centroid, m_centerMass, m_speed, 100);
        }
        void clearMass()
        {
            m_centerMass  = FHPos{};
            m_pointsCount = 0;
        }
        void addToMass(FHPos pos)
        {
            m_centerMass = m_centerMass + pos;
            m_pointsCount++;
        }
        void finalizeMass()
        {
            if (m_pointsCount) {
                m_centerMass.m_x /= m_pointsCount;
                m_centerMass.m_y /= m_pointsCount;
            }
        }
    };

    void clearMass()
    {
        for (auto& cluster : m_clusters)
            cluster.clearMass();
    }
    void assignPoints(bool isDone)
    {
        for (size_t i = 0; MapTilePtr tile : *m_region) {
            const size_t clusterId = m_nearestIndex[i];
            m_clusters[clusterId].addToMass(tile->m_pos);
            if (isDone) {
                m_clusters[clusterId].m_points.push_back(tile);
            }
            i++;
        }
    }
    void finalizeMass()
    {
        for (auto& cluster : m_clusters)
            cluster.finalizeMass();
    }
    void updateCentroids()
    {
        for (auto& cluster : m_clusters)
            cluster.updateCentroid();
    }

    size_t getNearestClusterId(MapTilePtr point) const
    {
        int64_t minDist          = m_clusters[0].distanceTo(point);
        size_t  nearestClusterId = 0;

        for (size_t i = 1; i < m_clusters.size(); i++) {
            const int64_t dist = m_clusters[i].distanceTo(point);
            if (dist < minDist) {
                minDist          = dist;
                nearestClusterId = i;
            }
        }

        return nearestClusterId;
    }

    bool runIter(bool last)
    {
        bool done = true;

        // Add all points to their nearest cluster
        for (size_t i = 0; MapTilePtr tile : *m_region) {
            size_t&      currentClusterId = m_nearestIndex[i];
            const size_t nearestClusterId = getNearestClusterId(tile);

            if (currentClusterId != nearestClusterId) {
                currentClusterId = nearestClusterId;
                done             = false;
            }
            i++;
        }

        // clear all existing clusters
        clearMass();

        // reassign points to their new clusters
        assignPoints(done || last);

        // calculate new mass points
        finalizeMass();

        updateCentroids();

        return done;
    }

    const MapTileRegion* m_region = nullptr;
    std::vector<size_t>  m_nearestIndex;
    std::vector<Cluster> m_clusters;
};
}

MapTileRegionList MapTileRegion::splitByFloodFill(bool useDiag, MapTilePtr hint) const
{
    if (empty())
        return {};

    MapTileRegionList result;
    MapTilePtrList    currentBuffer;

    MapTileRegion  visited;
    MapTilePtrList currentEdge;
    auto           addToCurrent = [&currentBuffer, &visited, &currentEdge, *this](MapTilePtr cell) {
        if (visited.contains(cell))
            return;
        if (!this->contains(cell))
            return;
        visited.insert(cell);
        currentBuffer.push_back(cell);
        currentEdge.push_back(cell);
    };
    MapTileRegion remain = *this;
    if (hint) {
        if (!remain.contains(hint))
            throw std::runtime_error("Invalid tile hint provided");
    }

    while (!remain.empty()) {
        MapTilePtr startCell = hint ? hint : *remain.begin();
        hint                 = nullptr;
        addToCurrent(startCell);

        while (!currentEdge.empty()) {
            MapTilePtrList nextEdge = std::move(currentEdge);

            for (MapTilePtr edgeCell : nextEdge) {
                const auto& neighbours = edgeCell->neighboursList(useDiag);
                for (auto growedCell : neighbours) {
                    addToCurrent(growedCell);
                }
            }
        }
        MapTileRegion current = MapTileRegion(std::move(currentBuffer));
        remain.erase(current);
        result.push_back(std::move(current));
    }

    return result;
}

MapTileRegionList MapTileRegion::splitByMaxArea(size_t maxArea, size_t iterLimit) const
{
    MapTileRegionList result;
    size_t            zoneArea = this->size();
    if (!zoneArea)
        return result;

    const size_t k = (zoneArea + maxArea + 1) / maxArea;

    return splitByK(k, iterLimit);
}

MapTileRegionList MapTileRegion::splitByK(size_t k, size_t iterLimit) const
{
    size_t zoneArea = this->size();
    if (!zoneArea)
        return {};

    if (k == 1)
        return { *this };

    SplitRegionSettingsList settings(k);
    size_t                  s = size();
    for (size_t i = 0; i < k; ++i) {
        settings[i].m_start = (*this)[i * s / k];
    }
    return splitByKExt(settings);
}

MapTileRegionList MapTileRegion::splitByKExt(const SplitRegionSettingsList& settingsList, size_t iterLimit) const
{
    if (empty())
        return {};

    if (settingsList.size() == 1)
        return { *this };

    KMeansData   kmeans;
    const size_t K = settingsList.size();
    kmeans.m_clusters.resize(K);
    kmeans.m_region = this;
    kmeans.m_nearestIndex.resize(size());
    for (size_t i = 0; i < K; i++) {
        auto& c      = kmeans.m_clusters[i];
        c.m_centroid = settingsList[i].m_start->m_pos;
        c.m_points.reserve(this->size() / K);
        c.m_radius = settingsList[i].m_radius;
        c.m_speed  = settingsList[i].m_speed;
    }

    for (size_t i = 0; i < kmeans.m_nearestIndex.size(); i++) {
        kmeans.m_nearestIndex[i] = size_t(-1);
    }

    for (size_t iter = 0; iter < iterLimit; ++iter) {
        const bool last = iter == iterLimit - 1;
        if (kmeans.runIter(last))
            break;
    }

    MapTileRegionList result;
    result.resize(K);
    for (size_t i = 0; i < K; i++) {
        result[i] = MapTileRegion(kmeans.m_clusters[i].m_points);
    }
    return result;
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
    auto it = std::min_element(cbegin(), cend(), [pos](MapTilePtr l, MapTilePtr r) {
        return posDistance(pos, l->m_pos, 100) < posDistance(pos, r->m_pos, 100);
    });
    return (*it);
}

}
