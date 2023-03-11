/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "TemplateUtils.hpp"
#include "MapTile.hpp"

#include "IRandomGenerator.hpp"

namespace FreeHeroes {

class KMeansSegmentation {
public:
    struct Cluster;
    struct Point {
        MapTilePtr m_pos       = nullptr;
        Cluster*   m_clusterId = nullptr;

        constexpr Point() noexcept = default;
        constexpr Point(MapTilePtr pos) noexcept
        {
            m_pos = pos;
        }

        Cluster* getCluster() const noexcept { return m_clusterId; }

        void setCluster(Cluster* val) { m_clusterId = val; }

        constexpr int64_t getDistance(const Point& another) const noexcept
        {
            return posDistance(m_pos->m_pos, another.m_pos->m_pos);
        }
        constexpr int64_t getDistance(const FHPos& another) const noexcept
        {
            return posDistance(m_pos->m_pos, another);
        }

        constexpr bool operator<(const Point& another) const noexcept
        {
            return m_pos->m_pos < another.m_pos->m_pos;
        }
    };

    struct Cluster {
        FHPos               m_centroid;
        std::vector<Point*> m_points;
        size_t              m_index  = 0;
        int64_t             m_radius = 100;
        int                 m_speed  = 100;

        Cluster() = default;
        Cluster(Point* centroid)
        {
            m_centroid = centroid->m_pos->m_pos;
            addPoint(centroid);
        }
        void addPoint(Point* p)
        {
            p->setCluster(this);
            m_points.push_back(p);
        }

        bool removePoint(Point* p)
        {
            size_t size = m_points.size();

            for (size_t i = 0; i < size; i++) {
                if (m_points[i] == p) {
                    m_points.erase(m_points.begin() + i);
                    return true;
                }
            }
            return false;
        }

        void removeAllPoints() { m_points.clear(); }

        Point* getPoint(size_t pos) const { return m_points[pos]; }

        size_t getSize() const { return m_points.size(); }

        int64_t distanceTo(Point& point) const
        {
            return point.getDistance(m_centroid) * 1000 / m_radius;
        }

        std::string getCentroidStr() const
        {
            return m_centroid.toPrintableString();
        }
    };

public:
    int  m_iters = 10;
    bool m_done  = false;

    std::vector<Cluster> m_clusters;
    std::vector<Point>   m_points;

    void clearClusters()
    {
        for (Cluster& cluster : m_clusters)
            cluster.removeAllPoints();
    }

    Cluster* getNearestClusterId(Point& point);

    void initClustersByCentroids(const std::vector<size_t>& centroidPointIndexList)
    {
        size_t K = centroidPointIndexList.size();
        assert(K < m_points.size());

        m_clusters.resize(K);

        for (size_t i = 0; i < K; i++) {
            const size_t pointIndex = centroidPointIndexList[i];
            initCluster(i, pointIndex, false);
        }
    }

    void initCluster(size_t i, size_t pointIndex, bool fix)
    {
        m_clusters[i] = Cluster(&m_points[pointIndex]);

        m_clusters[i].m_index = i;
        if (fix)
            m_clusters[i].m_speed = 0;
        m_points[pointIndex].setCluster(&m_clusters[i]);
    }

    void initRandomClusterCentoids(size_t K, Core::IRandomGenerator* rng)
    {
        std::sort(m_points.begin(), m_points.end());

        std::set<size_t> usedPointIds;

        for (size_t i = 0; i < K; i++) {
            while (true) {
                size_t index = rng->gen(m_points.size() - 1);

                if (!usedPointIds.contains(index)) {
                    usedPointIds.insert(index);
                    break;
                }
            }
        }

        initClustersByCentroids(std::vector<size_t>(usedPointIds.cbegin(), usedPointIds.cend()));
    }

    void initEqualCentoids(size_t K)
    {
        std::sort(m_points.begin(), m_points.end());

        std::set<size_t> usedPointIds;
        size_t           step = m_points.size() / K;

        for (size_t i = 0; i < K; i++)
            usedPointIds.insert(i * step);

        initClustersByCentroids(std::vector<size_t>(usedPointIds.cbegin(), usedPointIds.cend()));
    }

    void runIter();

    void run(std::ostream& os);
};

}
