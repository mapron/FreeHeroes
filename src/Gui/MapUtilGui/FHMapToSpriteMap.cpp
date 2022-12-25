/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHMapToSpriteMap.hpp"

#include "FHMap.hpp"
#include "SpriteMap.hpp"

#include "LibraryDwelling.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"

#include "IGraphicsLibrary.hpp"

namespace FreeHeroes {

namespace {
const int g_terrainPriority = -3;
const int g_riverPriority   = -2;
const int g_roadPriority    = -1;
const int g_objPriority     = 10;

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
        auto& cell = result.m_planes[pos.m_z].m_grids[priority].m_rows[pos.m_y].m_cells[pos.m_x];

        item.m_animationOffset = pos.m_x * 7U + pos.m_y * 15U;
        cell.m_items.push_back(std::move(item));
    };

    auto placeObject = [&placeItem, graphicsLibrary](Core::LibraryObjectDefConstPtr def, const FHPos& pos) {
        auto            resourceIdDef = def->substituteFor ? def->substituteFor : def;
        auto            sprite        = graphicsLibrary->getObjectAnimation(resourceIdDef->id);
        SpriteMap::Item item;
        item.m_sprite      = sprite;
        item.m_spriteGroup = 0;
        placeItem(std::move(item), pos, g_objPriority - def->priority);
    };

    // @todo: remove this.
    auto placeObjectById = [&placeItem, graphicsLibrary](const std::string& id, const FHPos& pos) {
        auto            sprite = graphicsLibrary->getObjectAnimation(id);
        SpriteMap::Item item;
        item.m_sprite      = sprite;
        item.m_spriteGroup = 0;
        placeItem(std::move(item), pos, g_objPriority);
    };

    fhMap.m_tileMap.eachPosTile([&placeItem, graphicsLibrary](const FHPos& pos, const FHTileMap::Tile& tile) {
        {
            auto            sprite = graphicsLibrary->getObjectAnimation(tile.m_terrain->presentationParams.defFile);
            SpriteMap::Item item;
            item.m_sprite      = sprite;
            item.m_spriteGroup = tile.m_view;
            item.m_flipHor     = tile.m_flipHor;
            item.m_flipVert    = tile.m_flipVert;
            placeItem(std::move(item), pos, g_terrainPriority);
        }
        if (tile.m_riverType != FHRiverType::Invalid) {
            std::string id = "";
            if (tile.m_riverType == FHRiverType::Water)
                id = "clrrvr";
            if (tile.m_riverType == FHRiverType::Ice)
                id = "icyrvr";
            if (tile.m_riverType == FHRiverType::Mud)
                id = "mudrvr";

            if (tile.m_riverType == FHRiverType::Lava)
                id = "lavrvr";

            auto            sprite = graphicsLibrary->getObjectAnimation(id);
            SpriteMap::Item item;
            item.m_sprite      = sprite;
            item.m_spriteGroup = tile.m_riverView;
            item.m_flipHor     = tile.m_riverFlipHor;
            item.m_flipVert    = tile.m_riverFlipVert;
            placeItem(std::move(item), pos, g_riverPriority);
        }
        if (tile.m_roadType != FHRoadType::Invalid) {
            std::string id = "";
            if (tile.m_roadType == FHRoadType::Dirt)
                id = "dirtrd";
            if (tile.m_roadType == FHRoadType::Gravel)
                id = "gravrd";
            if (tile.m_roadType == FHRoadType::Cobblestone)
                id = "cobbrd";
            auto            sprite = graphicsLibrary->getObjectAnimation(id);
            SpriteMap::Item item;
            item.m_sprite        = sprite;
            item.m_spriteGroup   = tile.m_roadView;
            item.m_flipHor       = tile.m_roadFlipHor;
            item.m_flipVert      = tile.m_roadFlipVert;
            item.m_shiftHalfTile = true;
            placeItem(std::move(item), pos, g_roadPriority);
        }
    });

    for (const auto& obj : fhMap.m_towns) {
        auto* def = obj.m_factionId->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }

    for (auto& fhHero : fhMap.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto* libraryHero = fhHero.m_data.m_army.hero.library;
        assert(libraryHero);

        auto pos = fhHero.m_pos;
        pos.m_x += 1;
        const std::string id = playerIndex < 0 ? "avxprsn0" : libraryHero->getAdventureSprite() + "e";
        placeObjectById(id, pos);
    }
    for (auto& obj : fhMap.m_objects.m_resources) {
        if (obj.m_type == FHResource::Type::Resource) {
            auto* def = obj.m_id->objectDefs.get({});
            placeObject(def, obj.m_pos);
        } else {
            auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
            placeObject(def, obj.m_pos);
        }
    }

    for (auto& obj : fhMap.m_objects.m_resourcesRandom) {
        std::string id = "avtrndm0";
        placeObjectById(id, obj.m_pos);
    }

    for (auto& obj : fhMap.m_objects.m_artifacts) {
        auto* def = obj.m_id->objectDefs.get({});
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_artifactsRandom) {
        std::string id = "";
        switch (obj.m_type) {
            case FHRandomArtifact::Type::Any:
                id = "avarand";
                break;
            case FHRandomArtifact::Type::Treasure:
                id = "avarnd1";
                break;
            case FHRandomArtifact::Type::Minor:
                id = "avarnd2";
                break;
            case FHRandomArtifact::Type::Major:
                id = "avarnd3";
                break;
            case FHRandomArtifact::Type::Relic:
                id = "avarnd4";
                break;
            case FHRandomArtifact::Type::Invalid:
                break;
        }
        placeObjectById(id, obj.m_pos);
    }

    const int monsterXOffset = fhMap.m_version == Core::GameVersion::HOTA ? 1 : 0;

    for (auto& obj : fhMap.m_objects.m_monsters) {
        auto pos = obj.m_pos;
        pos.m_x += monsterXOffset;

        auto* def = obj.m_id->objectDefs.get({});
        placeObject(def, obj.m_pos);
    }

    for (auto& obj : fhMap.m_objects.m_dwellings) {
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_mines) {
        auto* def = obj.m_id->minesDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }

    for (auto& obj : fhMap.m_objects.m_banks) {
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_obstacles) {
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_visitables) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_shrines) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_skillHuts) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }
    for (auto& obj : fhMap.m_objects.m_scholars) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }

    for (auto& obj : fhMap.m_objects.m_questHuts) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        placeObject(def, obj.m_pos);
    }

    for (auto& obj : fhMap.m_objects.m_pandoras) {
        std::string id = "ava0128";
        placeObjectById(id, obj.m_pos);
    }

    result.m_width  = fhMap.m_tileMap.m_width;
    result.m_height = fhMap.m_tileMap.m_height;
    result.m_depth  = fhMap.m_tileMap.m_depth;

    return result;
}

}
