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

    enum class GuardPosition
    {
        TL,
        T,
        TR,
        L,
        R,
        BL,
        B,
        BR
    };

    FHPos   m_absPos;
    FHPos   m_guardAbsPos;
    int64_t m_guard         = 0;
    bool    m_considerBlock = false;

    ObjectGenerator::IObject::Type m_type          = ObjectGenerator::IObject::Type::Visitable;
    GuardPosition                  m_guardPosition = GuardPosition::B;

    std::set<FHPos> m_estimatedOccupied;
    std::set<FHPos> m_protectionBorder;
    std::set<FHPos> m_protectionBorder2;
    std::set<FHPos> m_blurForPassable;
    std::set<FHPos> m_guardRegion;
    std::set<FHPos> m_allArea;
    std::set<FHPos> m_fitArea;

    size_t getEstimatedArea() const { return m_fitArea.size(); }

    size_t  m_itemLimit   = 0;
    bool    m_canPushMore = false;
    int64_t m_targetGuard = 0;

    void sumGuard();

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
