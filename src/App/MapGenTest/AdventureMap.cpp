/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureMap.hpp"

namespace FreeHeroes {

AdventureMap::AdventureMap(int width, int height, int depth)
    : m_width(width)
    , m_height(height)
    , m_depth(depth)
{
    m_data.resize(depth);
    for (int z = 0; z < depth; ++z) {
        m_data[z].resize(height);
        for (int h = 0; h < height; ++h)
            m_data[z][h].resize(width);
    }
}

}
