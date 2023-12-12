/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapTileRegion.hpp"
#include "MapTile.hpp"
#include "MapTileContainer.hpp"

#include "KMeans.hpp"

namespace FreeHeroes {

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

MapTileRegionList MapTileRegion::splitByMaxArea(std::ostream& os, size_t maxArea, bool repulse) const
{
    MapTileRegionList result;
    size_t            zoneArea = this->size();
    if (!zoneArea)
        return result;

    const size_t k = (zoneArea + maxArea + 1) / maxArea;

    return splitByK(os, k, repulse);
}

MapTileRegionList MapTileRegion::splitByK(std::ostream& os, size_t k, bool repulse) const
{
    MapTileRegionList result;
    size_t            zoneArea = this->size();
    if (!zoneArea)
        return result;

    if (k == 1) {
        result.push_back(*this);
    } else {
        KMeansSegmentation seg;
        seg.m_points.reserve(zoneArea);
        for (auto* cell : *this) {
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
            result.push_back(std::move(zoneSeg));
        }
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
    if (ensureInbounds && !region.contains(centroid)) {
        auto it  = std::min_element(region.cbegin(), region.cend(), [centroid](MapTilePtr l, MapTilePtr r) {
            return posDistance(centroid, l, 100) < posDistance(centroid, r, 100);
        });
        centroid = (*it);
    }

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

}
