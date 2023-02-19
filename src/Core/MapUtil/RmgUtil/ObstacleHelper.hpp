/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TileZone.hpp"

#include <algorithm>

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
}

struct ObstacleBucket {
    std::vector<Core::LibraryMapObstacleConstPtr> m_objects;
    Core::PlanarMask                              m_mask;
    size_t                                        m_area = 0;
};
using ObstacleBucketList = std::vector<ObstacleBucket>;

struct ObstacleIndex {
    ObstacleBucketList m_bucketLists;

    void add(Core::LibraryMapObstacleConstPtr obj);
    void doSort();

    bool isEmpty(const Core::PlanarMask& mask, size_t xOffset, size_t yOffset, size_t w, size_t h);

    // mask is 8x6, for example. we will search for an object that fits int top-left corner.
    std::vector<const ObstacleBucket*> find(const Core::PlanarMask& mask, size_t xOffset, size_t yOffset);
};

struct FHMap;
class ObstacleHelper {
public:
    ObstacleHelper(FHMap&                        map,
                   std::vector<TileZone>&        tileZones,
                   MapTileContainer&             tileContainer,
                   Core::IRandomGenerator* const rng,
                   const Core::IGameDatabase*    database,
                   std::ostream&                 logOutput);

    void placeObstacles();

private:
    FHMap&                           m_map;
    std::vector<TileZone>&           m_tileZones;
    MapTileContainer&                m_tileContainer;
    Core::IRandomGenerator* const    m_rng;
    const Core::IGameDatabase* const m_database;
    std::ostream&                    m_logOutput;
};

}
