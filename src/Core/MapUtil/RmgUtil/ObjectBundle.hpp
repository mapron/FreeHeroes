/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ObjectGenerator.hpp"

namespace FreeHeroes {

struct TileZone;
struct ObjectBundle {
    struct Item {
        ObjectGenerator::IObjectPtr m_obj;
        int64_t                     m_guard = 0;
        FHPos                       m_absPos;
    };
    std::vector<Item> m_items;

    FHPos   m_absPos;
    FHPos   m_guardAbsPos;
    int64_t m_guard = 0;

    ObjectGenerator::IObject::Type m_type = ObjectGenerator::IObject::Type::Visitable;

    std::set<FHPos> m_estimatedOccupied;
    std::set<FHPos> m_protectionBorder;
    std::set<FHPos> m_blurForPassable;
    std::set<FHPos> m_guardRegion;
    std::set<FHPos> m_allArea;

    size_t getEstimatedArea() const { return m_allArea.size(); }

    bool m_canPushMore = false;

    void sumGuard();
    void checkIfCanPushMore();

    void estimateOccupied();

    bool placeOnMap(std::set<FHPos>& availableCells, Core::IRandomGenerator* const rng);

    std::string toPrintableString() const;
};

class ObjectBundleSet {
public:
    void consume(const ObjectGenerator&        generated,
                 TileZone&                     tileZone,
                 Core::IRandomGenerator* const rng);

    std::vector<ObjectBundle> m_bundlesGuarded;
    std::vector<ObjectBundle> m_bundlesNonGuarded;

    struct BucketItem {
        ObjectGenerator::IObjectPtr m_obj;
        int64_t                     m_guard = 0;
    };

    struct Bucket {
        std::vector<BucketItem> m_guarded;
        std::vector<BucketItem> m_nonGuarded;
    };

    std::map<ObjectGenerator::IObject::Type, Bucket> m_buckets;

    std::set<FHPos> m_cells;
};

}
