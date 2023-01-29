/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"

#include <algorithm>

namespace FreeHeroes {

struct ObstacleBucket {
    std::vector<Core::LibraryMapObstacleConstPtr> m_objects;
    Core::LibraryObjectDef::PlanarMask            m_mask;
    size_t                                        m_area = 0;
};
using ObstacleBucketList = std::vector<ObstacleBucket>;

struct ObstacleIndex {
    ObstacleBucketList m_bucketLists;

    void add(Core::LibraryMapObstacleConstPtr obj)
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
    void doSort()
    {
        //assert(m_bucketLists.contains(Type::Small));
        // for (auto& bucketList : m_bucketLists) {
        std::sort(m_bucketLists.begin(), m_bucketLists.end(), [](const ObstacleBucket& l, const ObstacleBucket& r) {
            return l.m_area > r.m_area;
        });
        //}
    }

    bool isEmpty(const Core::LibraryObjectDef::PlanarMask& mask, size_t xOffset, size_t yOffset, size_t w, size_t h)
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

    // mask is 8x6, for example. we will search for an object that fits int top-left corner.
    std::vector<const ObstacleBucket*> find(const Core::LibraryObjectDef::PlanarMask& mask, size_t xOffset, size_t yOffset)
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
};

}
