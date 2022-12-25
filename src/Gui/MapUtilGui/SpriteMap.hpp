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
    struct Item {
        Gui::IAsyncSpritePtr m_sprite;
        int                  m_spriteGroup     = 0;
        int                  m_animationOffset = 0;
        bool                 m_flipHor         = false;
        bool                 m_flipVert        = false;
    };

    struct Plane {
        struct Row {
            struct Cell {
                std::map<int, std::vector<Item>> m_items; // item by draw priority
            };
            std::map<int, Cell> m_cells;
        };
        std::map<int, Row> m_rows;
    };

    std::vector<Plane> m_planes;

    int m_width  = 0;
    int m_height = 0;
    int m_depth  = 0;
};

struct SpritePaintSettings {
    bool m_animateTerrain = false;
    bool m_animateObjects = false;
    bool m_grid           = false;
    int  m_tileSize       = 32;
};

struct SpriteRenderSettings {
};

}
