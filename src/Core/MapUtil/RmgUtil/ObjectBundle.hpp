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
        //FHPos                       m_relativePos;
    };
    std::vector<Item> m_items;
    //FHPos             m_guardRelativePos;

    FHPos   m_absPos;
    FHPos   m_guardAbsPos;
    int64_t m_guard = 0;

    std::set<FHPos> m_estimatedOccupied;

    void makeGuard(int percent);

    void estimateOccupied();

    void placeOnMap(std::set<FHPos>& availableCells, Core::IRandomGenerator* const rng);
};

class ObjectBundleSet {
public:
    void consume(const ObjectGenerator&        generated,
                 TileZone&                     tileZone,
                 Core::IRandomGenerator* const rng);

    std::vector<ObjectBundle> m_bundles;
};

}
