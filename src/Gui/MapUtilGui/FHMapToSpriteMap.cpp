/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHMapToSpriteMap.hpp"

#include "FHMap.hpp"
#include "SpriteMap.hpp"

#include "IGraphicsLibrary.hpp"

namespace FreeHeroes {

namespace {
const int g_terrainPriority = -1;

/*
DrawHint directionToHint(HeroDirection direction)
{
    switch (direction) {
        case HeroDirection::T:
            return { 0, false };
        case HeroDirection::TR:
            return { 1, false };
        case HeroDirection::R:
            return { 2, false };
        case HeroDirection::BR:
            return { 3, false };
        case HeroDirection::B:
            return { 4, false };
        case HeroDirection::BL:
            return { 3, true };
        case HeroDirection::L:
            return { 2, true };
        case HeroDirection::TL:
            return { 1, true };
    }
    return {};
}*/
}

SpriteMap MapRenderer::render(const FHMap& fhMap, const Gui::IGraphicsLibrary* graphicsLibrary) const
{
    SpriteMap result;
    result.m_planes.resize(fhMap.m_tileMap.m_depth);

    auto placeItem = [&result](SpriteMap::Item item, const FHPos& pos, int priority = 0) {
        auto& cell = result.m_planes[pos.m_z].m_rows[pos.m_y].m_cells[pos.m_x];
        cell.m_items[priority].push_back(std::move(item));
    };

    fhMap.m_tileMap.eachPosTile([&placeItem, graphicsLibrary](const FHPos& pos, const FHTileMap::Tile& tile) {
        auto            sprite = graphicsLibrary->getObjectAnimation(tile.m_terrain->presentationParams.defFile);
        SpriteMap::Item item;
        item.m_sprite      = sprite;
        item.m_spriteGroup = tile.m_view;
        item.m_flipHor     = tile.m_flipHor;
        item.m_flipVert    = tile.m_flipVert;
        placeItem(std::move(item), pos, g_terrainPriority);
    });

    result.m_width  = fhMap.m_tileMap.m_width;
    result.m_height = fhMap.m_tileMap.m_height;
    result.m_depth  = fhMap.m_tileMap.m_depth;

    return result;
}

}
