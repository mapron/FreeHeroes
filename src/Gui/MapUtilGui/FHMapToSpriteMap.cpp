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

Gui::IGraphicsLibrary::PixmapKey getTerrainKey(Core::LibraryTerrainConstPtr terrain, int variant)
{
    if (terrain->presentationParams.defFileSplit) {
        Gui::IGraphicsLibrary::PixmapKey result;
        auto                             suffix = std::to_string(variant);
        while (suffix.size() < 3)
            suffix = "0" + suffix;
        result.resourceName = terrain->presentationParams.defFile + suffix;
        return result;
    }
    const bool isAnimated = terrain->presentationParams.isAnimated;
    return Gui::IGraphicsLibrary::PixmapKey(terrain->presentationParams.defFile, isAnimated ? variant : 0, isAnimated ? 0 : variant);
}

}

SpriteMap MapRenderer::render(const FHMap& fhMap, const Gui::IGraphicsLibrary* graphicsLibrary, int z) const
{
    SpriteMap result;
    fhMap.m_tileMap.eachPosTile([&result, graphicsLibrary, z](const FHPos& pos, const FHTileMap::Tile& tile) {
        if (pos.m_z != z)
            return;

        auto& destTile = result.m_rows[pos.m_y].m_cells[pos.m_y];

        {
            auto                       key    = getTerrainKey(tile.m_terrain, tile.m_view);
            auto                       sprite = graphicsLibrary->getObjectAnimation(key.resourceName);
            SpriteMap::Row::Cell::Item item;
            item.m_sprite      = sprite;
            item.m_isAnimated  = tile.m_terrain->presentationParams.isAnimated;
            item.m_spriteGroup = key.group;
            item.m_flipHor     = tile.m_flipHor;
            item.m_flipVert    = tile.m_flipVert;
            destTile.m_items.push_back(item);
        }
    });

    return result;
}

}
