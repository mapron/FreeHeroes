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
