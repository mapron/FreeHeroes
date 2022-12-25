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
    enum class Layer
    {
        Invalid,
        Terrain,
        Town,
        Hero,
        Resource,
        Artifact,
        Monster,
        Dwelling,
        Bank,
        Mine,
        Pandora,
        Shrine,
        SkillHut,
        Scholar,
        QuestHut,
        GeneralVisitable,
        Decoration,
    };

    struct BlockMask {
        struct Cell {
            bool m_visitable = false;
            bool m_blocked   = false;
        };

        struct Row {
            std::vector<Cell> m_cells;
        };
        std::vector<Row> m_rows;
    };

    struct Item {
        Gui::IAsyncSpritePtr m_sprite;

        Layer m_layer         = Layer::Invalid;
        int   m_spriteGroup   = 0;
        bool  m_flipHor       = false;
        bool  m_flipVert      = false;
        bool  m_shiftHalfTile = false;
        int   m_width         = 1;
        int   m_height        = 1;

        int m_x        = 0;
        int m_y        = 0;
        int m_z        = 0;
        int m_priority = 0;

        BlockMask m_blockMask;
    };

    struct Cell {
        std::vector<Item> m_items;
        QColor            m_colorUnblocked;
        QColor            m_colorBlocked;
    };
    struct Row {
        std::map<int, Cell> m_cells;
    };
    struct LayerGrid {
        std::map<int, Row> m_rows;
    };

    struct Plane {
        std::map<int, LayerGrid> m_grids; // item by draw priority
    };

    std::vector<Plane> m_planes;

    int m_width  = 0;
    int m_height = 0;
    int m_depth  = 0;

    Cell& getCell(const Item& item)
    {
        auto& cell = m_planes[item.m_z].m_grids[item.m_priority].m_rows[item.m_y].m_cells[item.m_x];
        return cell;
    }

    void addItem(Item item)
    {
        auto& cell = getCell(item);
        cell.m_items.push_back(std::move(item));
    }
};

struct SpritePaintSettings {
    bool m_animateTerrain = false;
    bool m_animateObjects = false;
    bool m_grid           = false;
    bool m_gridOnTop      = false;
    int  m_gridOpacity    = 110;
    int  m_tileSize       = 32;

    enum class ContourStyle
    {
        None,
        Rect,
        Diamond,
        Cross,
    };

    struct LayerRules {
        bool         m_visible      = true;
        bool         m_showContour  = true;
        ContourStyle m_contourStyle = ContourStyle::None;
    };

    LayerRules m_globalRules;

    std::map<SpriteMap::Layer, LayerRules> m_specificRules;
};

struct SpriteRenderSettings {
};

}
