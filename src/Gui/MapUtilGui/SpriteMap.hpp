/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGuiResource.hpp"

#include <vector>
#include <map>

namespace FreeHeroes {

struct SpriteMap {
    struct Row {
        struct Cell {
            struct Item {
                Gui::IAsyncSpritePtr m_sprite;
                bool                 m_isAnimated      = false;
                int                  m_spriteGroup     = 0;
                int                  m_animationOffset = 0;
                bool                 m_flipHor         = false;
                bool                 m_flipVert        = false;
            };

            std::vector<Item> m_items;
        };
        std::map<int, Cell> m_cells;
    };
    std::map<int, Row> m_rows;

    int m_width  = 0;
    int m_height = 0;
};

}
