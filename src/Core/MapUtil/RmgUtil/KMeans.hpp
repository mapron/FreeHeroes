/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <vector>
#include <set>
#include <string>
#include <cassert>
#include <tuple>

#include "TemplateUtils.hpp"
#include "../FHTileMap.hpp"

#include "IRandomGenerator.hpp"

namespace FreeHeroes {

class KMeansSegmentation {
public:
    struct Cluster;
    struct Point {
        int      m_x         = 0;
        int      m_y         = 0;
        int      m_zUnused   = 0;
        Cluster* m_clusterId = nullptr;

        constexpr Point() noexcept = default;
        constexpr Point(const FHPos& pos) noexcept
        {
            m_x       = pos.m_x;
            m_y       = pos.m_y;
            m_zUnused = pos.m_z;
        }
        constexpr Point(int x, int y, int z) noexcept
        {
            m_x       = x;
            m_y       = y;
            m_zUnused = z;
        }

        constexpr FHPos toPos() const noexcept
        {
            return { m_x, m_y, m_zUnused };
        }

        Cluster* getCluster() const noexcept { return m_clusterId; }

        void setCluster(Cluster* val) { m_clusterId = val; }

        constexpr int  getVal(size_t pos) const noexcept { return pos == 0 ? m_x : m_y; }
        constexpr void setVal(size_t pos, int val) noexcept
        {
            if (pos == 0)
                m_x = val;
            else
                m_y = val;
        }

        constexpr int64_t getDistance(const Point& another) const noexcept
        {
            const int64_t diffX = m_x - another.m_x;
            const int64_t diffY = m_y - another.m_y;
            return intSqrt(diffX * diffX + diffY * diffY);
        }

        constexpr auto asTuple() const noexcept
        {
            return std::tie(m_x, m_y);
        }

        constexpr bool operator<(const Point& another) const noexcept
        {
            return asTuple() < another.asTuple();
        }
    };

    struct Cluster {
        Point               m_centroid;
        std::vector<Point*> m_points;
        size_t              m_index  = 0;
        int64_t             m_radius = 100;

        Cluster() = default;
        Cluster(Point* centroid_)
        {
            m_centroid = *centroid_;
            addPoint(centroid_);
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

        constexpr int getCentroidByPos(size_t pos) const noexcept { return m_centroid.getVal(pos); }

        constexpr const Point& getCentroid() const noexcept { return m_centroid; }

        constexpr void setCentroidByPos(size_t pos, int val) noexcept { m_centroid.setVal(pos, val); }

        std::string getCentroidStr() const
        {
            return "(" + std::to_string(m_centroid.m_x) + ", " + std::to_string(m_centroid.m_y) + ")";
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
            const size_t index = centroidPointIndexList[i];
            m_clusters[i]      = Cluster(&m_points[index]);

            m_clusters[i].m_index = i;
            m_points[index].setCluster(&m_clusters[i]);
        }
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

    void runIter();

    void run(std::ostream& os);
};

}
