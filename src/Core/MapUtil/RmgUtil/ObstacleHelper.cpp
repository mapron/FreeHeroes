/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ObstacleHelper.hpp"

#include "IGameDatabase.hpp"

#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"

namespace FreeHeroes {

void ObstacleIndex::add(Core::LibraryMapObstacleConstPtr obj)
{
    auto* def = obj->objectDefs.get({});
    assert(def);
    auto mask = def->blockMapPlanar;
    auto w    = mask.width;
    auto h    = mask.height;
    if (!w || !h)
        return;

    ObstacleBucketList& bucketList = m_bucketLists;
    auto                it         = std::find_if(bucketList.begin(), bucketList.end(), [&mask](const ObstacleBucket& buck) {
        return buck.m_mask == mask;
    });
    if (it != bucketList.end()) {
        (*it).m_objects.push_back(obj);
        return;
    }
    ObstacleBucket buck;
    buck.m_mask = mask;
    buck.m_area = mask.height * mask.width;
    buck.m_objects.push_back(obj);
    bucketList.push_back(std::move(buck));
}

void ObstacleIndex::doSort()
{
    //assert(m_bucketLists.contains(Type::Small));
    // for (auto& bucketList : m_bucketLists) {
    std::sort(m_bucketLists.begin(), m_bucketLists.end(), [](const ObstacleBucket& l, const ObstacleBucket& r) {
        return l.m_area > r.m_area;
    });
    //}
}

bool ObstacleIndex::isEmpty(const Core::LibraryObjectDef::PlanarMask& mask, size_t xOffset, size_t yOffset, size_t w, size_t h)
{
    for (size_t y = 0; y < h && (y + yOffset) < mask.height; ++y) {
        for (size_t x = 0; x < w && (x + xOffset) < mask.width; ++x) {
            const uint8_t requiredBlockBit = mask.data[y + yOffset][x + xOffset];
            if (requiredBlockBit)
                return false;
        }
    }
    return true;
}

std::vector<const ObstacleBucket*> ObstacleIndex::find(const Core::LibraryObjectDef::PlanarMask& mask, size_t xOffset, size_t yOffset)
{
    auto w = mask.width;
    auto h = mask.height;
    if (!w || !h)
        return {};

    auto isMaskFit = [&mask, xOffset, yOffset](const Core::LibraryObjectDef::PlanarMask& maskObj) {
        const auto w       = mask.width - xOffset;
        const auto h       = mask.height - yOffset;
        const auto wmin    = std::min(maskObj.width, w);
        const auto hmin    = std::min(maskObj.height, h);
        size_t     overlap = 0;
        for (size_t y = 0; y < hmin; ++y) {
            const auto& row = mask.data[y + yOffset];
            for (size_t x = 0; x < wmin; ++x) {
                const bool emptyBlockBit    = row[x + xOffset] == 0;
                const bool requiredBlockBit = row[x + xOffset] == 1;
                //const bool tentativeBlockBit = row[x + xOffset] == 2;
                const bool objBlockBit = maskObj.data[y][x] == 1;
                if (objBlockBit) {
                    overlap++;
                    if (emptyBlockBit)
                        return false;
                }
                if (!objBlockBit) {
                    if (requiredBlockBit)
                        return false;
                }
            }
        }
        return overlap > 0;
    };

    std::vector<const ObstacleBucket*> result;
    for (const ObstacleBucket& bucket : m_bucketLists) {
        if (isMaskFit(bucket.m_mask))
            result.push_back(&bucket);
    }
    return result;
}

ObstacleHelper::ObstacleHelper(FHMap&                        map,
                               MapCanvas&                    mapCanvas,
                               Core::IRandomGenerator* const rng,
                               const Core::IGameDatabase*    database,
                               std::ostream&                 logOutput)
    : m_map(map)
    , m_mapCanvas(mapCanvas)
    , m_rng(rng)
    , m_database(database)
    , m_logOutput(logOutput)
{
}

void ObstacleHelper::placeObstacles()
{
    ObstacleIndex obstacleIndex;
    using Type = Core::LibraryMapObstacle::Type;
    const std::set<Type> suitableObjTypes{
        Type::BRUSH,
        Type::BUSH,
        Type::CACTUS,
        Type::CANYON,
        Type::CRATER,
        Type::HILL,

        Type::LAKE,
        Type::LAVA_FLOW,
        Type::LAVA_LAKE,
        Type::MANDRAKE,
        Type::MOUNTAIN,
        Type::OAK_TREES,
        Type::PINE_TREES,

        Type::ROCK,
        Type::SAND_DUNE,
        Type::SAND_PIT,
        Type::SHRUB,
        Type::STALAGMITE,
        Type::STUMP,
        Type::TAR_PIT,
        Type::TREES,
        Type::VOLCANIC_VENT,
        Type::VOLCANO,
        Type::WILLOW_TREES,
        Type::YUCCA_TREES,

        Type::DESERT_HILLS,
        Type::DIRT_HILLS,
        Type::GRASS_HILLS,
        Type::ROUGH_HILLS,

        Type::SUBTERRANEAN_ROCKS,
        Type::SWAMP_FOLIAGE,
    };

    for (auto* record : m_database->mapObstacles()->records()) {
        if (!suitableObjTypes.contains(record->type))
            continue;
        obstacleIndex.add(record);
    }
    obstacleIndex.doSort();

    Core::LibraryObjectDef::PlanarMask mapMask;
    mapMask.width  = m_map.m_tileMap.m_width;
    mapMask.height = m_map.m_tileMap.m_height;
    mapMask.data.resize(mapMask.height);
    for (auto& row : mapMask.data)
        row.resize(mapMask.width);

    for (MapCanvas::Tile* cell : m_mapCanvas.m_needBeBlocked)
        mapMask.data[cell->m_pos.m_y][cell->m_pos.m_x] = 1;
    for (MapCanvas::Tile* cell : m_mapCanvas.m_tentativeBlocked) {
        mapMask.data[cell->m_pos.m_y][cell->m_pos.m_x] = 2;

        //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = 0, .m_valueB = 2 });
    }

    const size_t maxMaskLookupWidth  = 8;
    const size_t maxMaskLookupHeight = 6;

    /*
    for (size_t y = 0; y < mapMask.height; ++y) {
        for (size_t x = 0; x < mapMask.width; ++x) {
            if (mapMask.data[y][x] == 0)
                continue;
            FHPos pos{ (int) x, (int) y, 0 };
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 0 });
        }
    }*/

    for (size_t y = 0; y < mapMask.height; ++y) {
        Core::LibraryMapObstacleConstPtr prev = nullptr;
        for (size_t x = 0; x < mapMask.width; ++x) {
            //if (mapMask.data[y][x] == 0)
            //    continue;
            if (obstacleIndex.isEmpty(mapMask, x, y, maxMaskLookupWidth, maxMaskLookupHeight))
                continue;
            std::vector<const ObstacleBucket*> buckets = obstacleIndex.find(mapMask, x, y);
            if (buckets.empty())
                continue;
            assert(!buckets.empty());
            int         z = 0;
            const FHPos mapPos{ (int) x, (int) y, z };
            //buckets = obstacleIndex.find(mapMask, x, y);

            //const ObstacleBucket* firstBucket = buckets.front();

            std::vector<Core::LibraryMapObstacleConstPtr> suitable;
            for (const ObstacleBucket* bucket : buckets) {
                for (auto* obst : bucket->m_objects) {
                    if (obst == prev)
                        continue;
                    auto* def = obst->objectDefs.get({}); // @todo: substitutions?

                    FHPos objPos = mapPos;
                    objPos.m_x += def->blockMapPlanar.width - 1;
                    objPos.m_y += def->blockMapPlanar.height - 1;
                    if (!m_mapCanvas.m_tileIndex.contains(objPos))
                        continue;

                    Core::LibraryTerrainConstPtr requiredTerrain = m_mapCanvas.m_tileIndex.at(objPos)->m_zone->m_terrain;

                    if (def->terrainsSoftCache.contains(requiredTerrain))
                        suitable.push_back(obst);
                }
            }
            //{
            //    FHPos pos{ (int) x, (int) y, 0 };
            //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 2 });
            //}
            if (suitable.empty())
                continue;

            //const auto                       bucketSize = firstBucket->m_objects.size();
            Core::LibraryMapObstacleConstPtr obst = suitable[m_rng->gen(suitable.size() - 1)];
            assert(obst);

            prev = obst;

            auto* def = obst->objectDefs.get({});
            assert(def);

            for (size_t my = 0; my < def->blockMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->blockMapPlanar.width; ++mx) {
                    if (def->blockMapPlanar.data[my][mx] == 0)
                        continue;
                    size_t px = x + mx;
                    size_t py = y + my;
                    FHPos  maskBitPos{ (int) px, (int) py, 0 };
                    if (py < mapMask.height && px < mapMask.width) {
                        if (mapMask.data[py][px] == 1)
                            mapMask.data[py][px] = 2;
                        auto* cell = m_mapCanvas.m_tileIndex.at(maskBitPos);
                        m_mapCanvas.m_needBeBlocked.erase(cell);
                        m_mapCanvas.m_tentativeBlocked.erase(cell);
                        m_mapCanvas.m_blocked.insert(cell);
                    }
                    //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 3 });
                }
            }

            FHPos objPos = mapPos;
            objPos.m_x += def->blockMapPlanar.width - 1;
            objPos.m_y += def->blockMapPlanar.height - 1;

            FHObstacle fhOb;
            fhOb.m_id  = obst;
            fhOb.m_pos = objPos;
            m_map.m_objects.m_obstacles.push_back(std::move(fhOb));

            //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 1 }); // red

            //if (cnt-- <= 0)
            //    return;
        }
    }
}

}
