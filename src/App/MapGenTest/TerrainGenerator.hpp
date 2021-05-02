/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <compare>
#include <vector>

namespace FreeHeroes {

struct TerrainPlanePoint {
    int            x;
    int            y;
    constexpr auto operator<=>(const TerrainPlanePoint&) const = default;
};

class TerrainPlane {
public:
    TerrainPlane(int width, int height)
        : m_width(width)
        , m_height(height)
    {
        m_data.resize(height);
        for (int i = 0; i < height; ++i)
            m_data[i].resize(width);
    }

    void set(int x, int y, int value)
    {
        m_data[y][x] = value;
    }
    int get(int x, int y) const
    {
        return m_data[y][x];
    }

    void set(TerrainPlanePoint p, int value)
    {
        set(p.x, p.y, value);
    }
    int get(TerrainPlanePoint p) const
    {
        return get(p.x, p.y);
    }

    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    std::vector<std::vector<int>> m_data;
    const int                     m_width;
    const int                     m_height;
};

void generateTerrainPlane(TerrainPlane& plane);

}
