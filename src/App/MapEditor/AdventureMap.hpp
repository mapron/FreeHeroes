/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include "AdventureArmy.hpp"

#include <compare>
#include <vector>

namespace FreeHeroes {

struct AdventureMapTile {
    Core::LibraryTerrainConstPtr m_terrain        = nullptr;
    uint8_t                      m_terrainVariant = 0;
};

struct AdventureMapPoint {
    int            x;
    int            y;
    int            z;
    constexpr auto operator<=>(const AdventureMapPoint&) const = default;
};

// clang-format off
enum class HeroDirection { T, TR, R, BR, B, BL, L, TL, Default = R };
// clang-format on

struct AdventureMapHero {
    Core::AdventureArmyConstPtr m_army;
    AdventureMapPoint           m_pos;
    HeroDirection               m_direction = HeroDirection::Default;
};

class AdventureMap {
public:
    AdventureMap(int width, int height, int depth = 1);

    const AdventureMapTile& get(int x, int y, int z) const
    {
        return m_data[z][y][x];
    }
    AdventureMapTile& get(int x, int y, int z)
    {
        return m_data[z][y][x];
    }

    const AdventureMapTile& get(AdventureMapPoint p) const
    {
        return get(p.z, p.x, p.y);
    }
    AdventureMapTile& get(AdventureMapPoint p)
    {
        return get(p.z, p.x, p.y);
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int depth() const { return m_depth; }

    std::vector<AdventureMapHero> m_heroes;

private:
    std::vector<std::vector<std::vector<AdventureMapTile>>> m_data;

    const int m_width;
    const int m_height;
    const int m_depth;
};

}
