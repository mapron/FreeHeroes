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

QColor makeColor(const std::vector<int>& rgb)
{
    if (rgb.size() == 3)
        return QColor(rgb[0], rgb[1], rgb[2]);
    return QColor();
}

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

    auto makeItem = [](const FHPos& pos, int priority = 0) -> SpriteMap::Item {
        SpriteMap::Item item;
        item.m_x        = pos.m_x;
        item.m_y        = pos.m_y;
        item.m_z        = pos.m_z;
        item.m_priority = SpriteMap::s_objectMaxPriority - priority;
        return item;
    };

    auto makeItemById = [&makeItem, graphicsLibrary](SpriteMap::Layer layer, const std::string& id, const FHPos& pos, int priority = 0) -> SpriteMap::Item {
        auto item    = makeItem(pos, priority);
        item.m_layer = layer;

        auto sprite        = graphicsLibrary->getObjectAnimation(id);
        item.m_sprite      = sprite;
        item.m_spriteGroup = 0;
        item.addInfo("def", id);
        return item;
    };

    auto makeItemByDef = [&makeItemById](SpriteMap::Layer layer, Core::LibraryObjectDefConstPtr def, const FHPos& pos) -> SpriteMap::Item {
        auto resourceIdDef = def->substituteFor ? def->substituteFor : def;
        auto item          = makeItemById(layer, resourceIdDef->id, pos, def->priority);
        item.m_width       = def->blockMapPlanar.width;
        item.m_height      = def->blockMapPlanar.height;
        if (def != resourceIdDef)
            item.addInfo("substDef", def->id);
        return item;
    };

    fhMap.m_tileMap.eachPosTile([&makeItemById, &result](const FHPos& pos, const FHTileMap::Tile& tile) {
        {
            SpriteMap::Item item = makeItemById(SpriteMap::Layer::Terrain, tile.m_terrain->presentationParams.defFile, pos);
            item.m_spriteGroup   = tile.m_view;
            item.m_flipHor       = tile.m_flipHor;
            item.m_flipVert      = tile.m_flipVert;
            item.m_priority      = SpriteMap::s_terrainPriority;
            item.addInfo("id", tile.m_terrain->id);
            item.addInfo("view", std::to_string(tile.m_view));
            item.addInfo("flipHor", item.m_flipHor ? "true" : "false");
            item.addInfo("flipVert", item.m_flipVert ? "true" : "false");

            result.getCellMerged(item).m_colorBlocked   = makeColor(tile.m_terrain->presentationParams.minimapBlocked);
            result.getCellMerged(item).m_colorUnblocked = makeColor(tile.m_terrain->presentationParams.minimapUnblocked);
            result.addItem(std::move(item));
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

            SpriteMap::Item item = makeItemById(SpriteMap::Layer::Terrain, id, pos);

            item.m_spriteGroup = tile.m_riverView;
            item.m_flipHor     = tile.m_riverFlipHor;
            item.m_flipVert    = tile.m_riverFlipVert;
            item.m_priority    = SpriteMap::s_riverPriority;

            item.addInfo("flipHor", item.m_flipHor ? "true" : "false");
            item.addInfo("flipVert", item.m_flipVert ? "true" : "false");
            result.addItem(std::move(item));
        }
        if (tile.m_roadType != FHRoadType::Invalid) {
            std::string id = "";
            if (tile.m_roadType == FHRoadType::Dirt)
                id = "dirtrd";
            if (tile.m_roadType == FHRoadType::Gravel)
                id = "gravrd";
            if (tile.m_roadType == FHRoadType::Cobblestone)
                id = "cobbrd";

            SpriteMap::Item item = makeItemById(SpriteMap::Layer::Terrain, id, pos);

            item.m_spriteGroup   = tile.m_roadView;
            item.m_flipHor       = tile.m_roadFlipHor;
            item.m_flipVert      = tile.m_roadFlipVert;
            item.m_shiftHalfTile = true;
            item.m_priority      = SpriteMap::s_roadPriority;

            item.addInfo("flipHor", item.m_flipHor ? "true" : "false");
            item.addInfo("flipVert", item.m_flipVert ? "true" : "false");
            result.addItem(std::move(item));
        }
    });

    for (const auto& obj : fhMap.m_towns) {
        auto* def = obj.m_factionId->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Town, def, obj.m_pos));
    }

    for (auto& fhHero : fhMap.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto* libraryHero = fhHero.m_data.m_army.hero.library;
        assert(libraryHero);

        auto pos = fhHero.m_pos;
        pos.m_x += 1;
        const std::string id = playerIndex < 0 ? "avxprsn0" : libraryHero->getAdventureSprite() + "e";
        result.addItem(makeItemById(SpriteMap::Layer::Town, id, fhHero.m_pos));
    }
    for (auto& obj : fhMap.m_objects.m_resources) {
        if (obj.m_type == FHResource::Type::Resource) {
            auto* def = obj.m_id->objectDefs.get({});
            result.addItem(makeItemByDef(SpriteMap::Layer::Resource, def, obj.m_pos));
        } else {
            auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
            result.addItem(makeItemByDef(SpriteMap::Layer::Resource, def, obj.m_pos));
        }
    }

    for (auto& obj : fhMap.m_objects.m_resourcesRandom) {
        std::string id = "avtrndm0";
        result.addItem(makeItemById(SpriteMap::Layer::Resource, id, obj.m_pos));
    }

    for (auto& obj : fhMap.m_objects.m_artifacts) {
        auto* def = obj.m_id->objectDefs.get({});
        result.addItem(makeItemByDef(SpriteMap::Layer::Artifact, def, obj.m_pos).addInfo("id", obj.m_id->id));
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
        result.addItem(makeItemById(SpriteMap::Layer::Artifact, id, obj.m_pos));
    }

    const int monsterXOffset = fhMap.m_version == Core::GameVersion::HOTA ? 1 : 0;

    for (auto& obj : fhMap.m_objects.m_monsters) {
        auto pos = obj.m_pos;
        pos.m_x += monsterXOffset;

        auto* def = obj.m_id->objectDefs.get({});
        result.addItem(makeItemByDef(SpriteMap::Layer::Monster, def, pos).addInfo("id", obj.m_id->id));
    }

    for (auto& obj : fhMap.m_objects.m_dwellings) {
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Dwelling, def, obj.m_pos).addInfo("id", obj.m_id->id));
    }
    for (auto& obj : fhMap.m_objects.m_mines) {
        auto* def = obj.m_id->minesDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Mine, def, obj.m_pos).addInfo("id", obj.m_id->id));
    }

    for (auto& obj : fhMap.m_objects.m_banks) {
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Bank, def, obj.m_pos).addInfo("id", obj.m_id->id));
    }
    for (auto& obj : fhMap.m_objects.m_obstacles) {
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Decoration, def, obj.m_pos).addInfo("id", obj.m_id->id));
    }
    for (auto& obj : fhMap.m_objects.m_visitables) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::GeneralVisitable, def, obj.m_pos).addInfo("id", obj.m_visitableId->id));
    }
    for (auto& obj : fhMap.m_objects.m_shrines) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Shrine, def, obj.m_pos).addInfo("id", obj.m_visitableId->id));
    }
    for (auto& obj : fhMap.m_objects.m_skillHuts) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::SkillHut, def, obj.m_pos).addInfo("id", obj.m_visitableId->id));
    }
    for (auto& obj : fhMap.m_objects.m_scholars) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::Scholar, def, obj.m_pos).addInfo("id", obj.m_visitableId->id));
    }

    for (auto& obj : fhMap.m_objects.m_questHuts) {
        auto* def = obj.m_visitableId->objectDefs.get(obj.m_defIndex);
        result.addItem(makeItemByDef(SpriteMap::Layer::QuestHut, def, obj.m_pos).addInfo("id", obj.m_visitableId->id));
    }

    for (auto& obj : fhMap.m_objects.m_pandoras) {
        std::string id = "ava0128";
        result.addItem(makeItemById(SpriteMap::Layer::Pandora, id, obj.m_pos));
    }

    result.m_width  = fhMap.m_tileMap.m_width;
    result.m_height = fhMap.m_tileMap.m_height;
    result.m_depth  = fhMap.m_tileMap.m_depth;

    return result;
}

}
