/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGuiResource.hpp"

#include "LibraryObjectDef.hpp"
#include "MapScore.hpp"

#include <vector>
#include <map>
#include <string>

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
        QuestGuard,
        GeneralVisitable,
        Decoration,
        Event,
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

        QColor m_keyColor;

        Layer m_layer         = Layer::Invalid;
        int   m_spriteGroup   = 0;
        bool  m_flipHor       = false;
        bool  m_flipVert      = false;
        bool  m_shiftHalfTile = false;

        int m_x        = 0;
        int m_y        = 0;
        int m_z        = 0;
        int m_priority = 0;

        double m_opacity = 1.0;

        Core::CombinedMask m_blockMask;
        Core::MapScore     m_score;

        std::vector<std::pair<std::string, std::string>> m_info;

        std::string m_generationId;
        std::string m_overlayInfo;
        int         m_overlayInfoOffsetX = -1;
        int         m_overlayInfoFont    = 12;

        Item& addInfo(std::string a, std::string b)
        {
            m_info.push_back({ std::move(a), std::move(b) });
            return *this;
        }
        Item& setPriority(int priority)
        {
            m_priority = priority;
            return *this;
        }
    };

    struct Cell {
        std::vector<Item> m_items;
    };
    struct Row {
        std::map<int, Cell> m_cells;
    };
    struct LayerGrid {
        std::map<int, Row> m_rows;
    };

    struct CellIntegral {
        QColor m_colorUnblocked;
        QColor m_colorBlocked;

        bool m_blocked   = false;
        bool m_visitable = false;

        struct DebugPiece {
            QColor m_shapeColor;
            QColor m_penColor;
            QColor m_textColor;

            int m_shape       = 1; // 0 - none, 1 - circle, 2 - square
            int m_shapeRadius = 2; // 1..4

            QString m_text;
        };
        std::vector<DebugPiece> m_debug;
    };
    struct RowIntegral {
        std::map<int, CellIntegral> m_cells;
    };
    struct LayerGridIntegral {
        std::map<int, RowIntegral> m_rows;
    };

    struct Plane {
        LayerGridIntegral        m_merged;
        std::map<int, LayerGrid> m_grids; // item by draw priority
    };

    std::vector<Plane> m_planes;

    int m_width  = 0;
    int m_height = 0;
    int m_depth  = 0;

    [[maybe_unused]] static constexpr const int s_terrainPriority = -1000;
    [[maybe_unused]] static constexpr const int s_riverPriority   = s_terrainPriority + 1;
    [[maybe_unused]] static constexpr const int s_roadPriority    = s_riverPriority + 1;

    [[maybe_unused]] static constexpr const int s_objectMaxPriority = 1000;

    Cell& getCell(const Item& item)
    {
        auto& cell = m_planes[item.m_z].m_grids[item.m_priority].m_rows[item.m_y].m_cells[item.m_x];
        return cell;
    }
    CellIntegral& getCellMerged(const Item& item)
    {
        auto& cell = m_planes[item.m_z].m_merged.m_rows[item.m_y].m_cells[item.m_x];
        return cell;
    }

    std::vector<const Cell*> findCells(int x, int y, int z) const
    {
        std::vector<const Cell*> res;
        if (z >= (int) m_planes.size())
            return {};

        for (const auto& [priority, grid] : m_planes[z].m_grids) {
            if (!grid.m_rows.contains(y))
                continue;
            const auto& row = grid.m_rows.at(y);
            if (!row.m_cells.contains(x))
                continue;
            res.push_back(&row.m_cells.at(x));
        }
        return res;
    }

    Item* addItem(Item item)
    {
        auto& cell = getCell(item);
        cell.m_items.push_back(std::move(item));
        return &cell.m_items.back();
    }

    static QString layerTypeToString(Layer layer);
};

struct SpritePaintSettings {
    int  m_viewScalePercent = 100;
    bool m_doubleScale      = false;
    bool m_doubleScaleTmp   = false;

    bool m_animateTerrain = false;
    bool m_animateObjects = true;
    bool m_grid           = true;
    bool m_gridOnTop      = false;
    bool m_blockMask      = false;
    bool m_overlay        = true;
    int  m_gridOpacity    = 110;
    int  m_tileSize       = 32;

    bool m_strict = false;

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

    LayerRules                 m_globalRules;
    std::set<SpriteMap::Layer> m_filterLayer;
    std::set<Core::ScoreAttr>  m_filterAttr;
    std::string                m_filterGenerationId;

    std::map<SpriteMap::Layer, LayerRules> m_specificRules;

    int getEffectiveScale() const noexcept
    {
        int percent = m_viewScalePercent;
        if (m_doubleScale)
            percent *= 2;
        if (m_doubleScaleTmp)
            percent *= 2;
        return percent;
    }
};

struct SpriteRenderSettings {
    bool m_reserved = true;
    bool m_strict   = false;
};

}
