/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHMapToSpriteMap.hpp"

#include "FHMap.hpp"
#include "SpriteMap.hpp"

#include "IGameDatabase.hpp"

#include "LibraryBuilding.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryPlayer.hpp"

#include "ScoreUtil.hpp"

#include "IGraphicsLibrary.hpp"

namespace FreeHeroes {

namespace {

const Core::CombinedMask g_oneTileMask = Core::createOneTileCombinedMask();

QColor makeColor(const std::vector<int>& rgb)
{
    if (rgb.size() == 3)
        return QColor(rgb[0], rgb[1], rgb[2]);
    return QColor();
}

std::string playerToString(Core::LibraryPlayerConstPtr player)
{
    return player->untranslatedName;
}

QColor makePlayerColor(Core::LibraryPlayerConstPtr player)
{
    if (!player)
        return {};
    return QColor("#" + QString::fromStdString(player->presentationParams.colorRGB));
}

}

SpriteMap MapRenderer::render(const FHMap& fhMap, const Gui::IGraphicsLibrary* graphicsLibrary, const Core::IGameDatabase* database) const
{
    SpriteMap result;
    result.m_planes.resize(fhMap.m_tileMap.m_depth);
    std::set<const FHCommonObject*> rendered;

    auto makeItem = [](const FHPos& pos, int priority = 0) -> SpriteMap::Item {
        SpriteMap::Item item;
        item.m_x        = pos.m_x;
        item.m_y        = pos.m_y;
        item.m_z        = pos.m_z;
        item.m_priority = SpriteMap::s_objectMaxPriority - priority;
        return item;
    };

    bool hasMissingSprites = false;

    auto makeItemById = [&makeItem, &hasMissingSprites, graphicsLibrary](SpriteMap::Layer layer, const std::string& id, const FHPos& pos, int priority = 0) -> SpriteMap::Item {
        auto item    = makeItem(pos, priority);
        item.m_layer = layer;

        auto sprite = graphicsLibrary->getObjectAnimation(id);
        if (!sprite || !sprite->preload())
            hasMissingSprites = true;
        item.m_sprite      = sprite;
        item.m_spriteGroup = 0;
        item.addInfo("def", id);
        return item;
    };

    auto makeItemByDef = [&makeItemById](SpriteMap::Layer layer, Core::LibraryObjectDefConstPtr def, const FHPos& pos, bool calcX = false) -> SpriteMap::Item {
        auto resourceIdDef = def->substituteFor ? def->substituteFor : def;
        auto item          = makeItemById(layer, resourceIdDef->id, pos, def->priority);
        item.m_blockMask   = def->combinedMask;
        if (calcX)
            item.m_x = item.m_x - item.m_blockMask.m_visitable.begin()->m_x;
        if (def != resourceIdDef)
            item.addInfo("substDef", def->id);
        return item;
    };

    auto makeItemByDefId = [&makeItemByDef, database](SpriteMap::Layer layer, const std::string& id, const FHPos& pos, bool calcX = false) {
        auto* def = database->objectDefs()->find(id);
        if (!def)
            throw std::runtime_error("failed to find def id:" + id);
        return makeItemByDef(layer, def, pos, calcX);
    };

    auto makeItemByVisitable = [&makeItemByDef](SpriteMap::Layer layer, const FHCommonVisitable& obj) -> SpriteMap::Item {
        auto* def = obj.m_visitableId ? obj.m_visitableId->objectDefs.get(obj.m_defIndex) : obj.m_fixedDef;
        assert(def);
        auto item = makeItemByDef(layer, def, obj.m_pos);
        if (obj.m_visitableId)
            item.addInfo("id", obj.m_visitableId->id);
        return item;
    };

    auto addValueInfo = [](SpriteMap::Item* item, const FHCommonObject& obj) {
        if (obj.m_guard)
            item->addInfo("Guard", std::to_string(obj.m_guard));
        if (!obj.m_generationId.empty()) {
            item->addInfo("ScoreId", obj.m_generationId);
            item->m_generationId = obj.m_generationId;
        }
        item->m_score = obj.m_score;
        for (const auto& [key, value] : obj.m_score) {
            item->addInfo(FHScoreSettings::attrToString(key) + " Value", std::to_string(value));
        }
    };

    for (auto& tile : fhMap.m_debugTiles) {
        auto& cell    = result.m_planes[tile.m_pos.m_z].m_merged.m_rows[tile.m_pos.m_y].m_cells[tile.m_pos.m_x];
        auto  makeClr = [](int hue, int paletteSize, int alpha) {
            // generated 6, 8, 12, 16 hues that perceptually most different on max value and saturation.
            const std::vector<std::vector<int>> neatHues{
                 { 13, 45, 132, 183, 210, 314 },
                 { 8, 38, 61, 163, 187, 208, 266, 325 },
                 { 7, 31, 45, 64, 138, 171, 187, 200, 214, 273, 307, 335 },
                 { 10, 28, 39, 50, 68, 108, 161, 177, 188, 198, 209, 220, 276, 297, 324, 341 },
            };

            if (hue == 0)
                return QColor(Qt::transparent);
            if (hue == -1) {
                QColor clr(Qt::black);
                clr.setAlpha(alpha);
                return clr;
            }
            if (hue == -2) {
                QColor clr(Qt::white);
                clr.setAlpha(alpha);
                return clr;
            }

            hue = std::clamp(hue - 1, 0, 359);
            if (paletteSize > 0) {
                for (const auto& pal : neatHues) {
                    const int palSize = pal.size();
                    if (paletteSize <= palSize) {
                        hue = std::clamp(hue, 0, paletteSize - 1);
                        hue = pal[hue];
                        break;
                    }
                }
            }

            QColor clr;
            clr.setHsv(hue, 255, 255, alpha);
            return clr;
        };
        cell.m_debug.push_back({
            .m_shapeColor  = makeClr(tile.m_brushColor, tile.m_brushPalette, tile.m_brushAlpha),
            .m_penColor    = makeClr(tile.m_penColor, tile.m_penPalette, tile.m_penAlpha),
            .m_textColor   = makeClr(tile.m_textColor, tile.m_textPalette, tile.m_textAlpha),
            .m_shape       = tile.m_shape,
            .m_shapeRadius = tile.m_shapeRadius,
            .m_text        = QString::fromStdString(tile.m_text),
        });
    }

    fhMap.m_tileMap.eachPosTile([&makeItemById, &result](const FHPos& pos, const FHTileMap::Tile& tile, size_t) {
        if (tile.m_terrainId) {
            SpriteMap::Item item = makeItemById(SpriteMap::Layer::Terrain, tile.m_terrainId->presentationParams.defFile, pos);
            item.m_spriteGroup   = tile.m_terrainView.m_view;
            item.m_flipHor       = tile.m_terrainView.m_flipHor;
            item.m_flipVert      = tile.m_terrainView.m_flipVert;
            item.m_priority      = SpriteMap::s_terrainPriority;
            item.addInfo("id", tile.m_terrainId->id);
            item.addInfo("view", std::to_string(tile.m_terrainView.m_view));
            item.addInfo("flipHor", item.m_flipHor ? "true" : "false");
            item.addInfo("flipVert", item.m_flipVert ? "true" : "false");

            result.getCellMerged(item).m_colorBlocked   = makeColor(tile.m_terrainId->presentationParams.minimapBlocked);
            result.getCellMerged(item).m_colorUnblocked = makeColor(tile.m_terrainId->presentationParams.minimapUnblocked);
            result.addItem(std::move(item));
        }
        if (tile.m_riverType != FHRiverType::None) {
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

            item.m_spriteGroup = tile.m_riverView.m_view;
            item.m_flipHor     = tile.m_riverView.m_flipHor;
            item.m_flipVert    = tile.m_riverView.m_flipVert;
            item.m_priority    = SpriteMap::s_riverPriority;

            item.addInfo("flipHor", item.m_flipHor ? "true" : "false");
            item.addInfo("flipVert", item.m_flipVert ? "true" : "false");
            result.addItem(std::move(item));
        }
        if (tile.m_roadType != FHRoadType::None) {
            std::string id = "";
            if (tile.m_roadType == FHRoadType::Dirt)
                id = "dirtrd";
            if (tile.m_roadType == FHRoadType::Gravel)
                id = "gravrd";
            if (tile.m_roadType == FHRoadType::Cobblestone)
                id = "cobbrd";

            SpriteMap::Item item = makeItemById(SpriteMap::Layer::Terrain, id, pos);

            item.m_spriteGroup   = tile.m_roadView.m_view;
            item.m_flipHor       = tile.m_roadView.m_flipHor;
            item.m_flipVert      = tile.m_roadView.m_flipVert;
            item.m_shiftHalfTile = true;
            item.m_priority      = SpriteMap::s_roadPriority;

            item.addInfo("flipHor", item.m_flipHor ? "true" : "false");
            item.addInfo("flipVert", item.m_flipVert ? "true" : "false");
            result.addItem(std::move(item));
        }
    });

    for (const auto& obj : fhMap.m_towns) {
        Core::ObjectDefIndex defIndex;
        //
        //"sod.faction.castle"          : [ {"m": {"":"avccast0", "FORT": "avccasf0", "CIT": "avccasc0", "CAS": "avccasx0", "CAP": "avccasz0"}} ],
        if (!obj.m_hasCustomBuildings) {
            if (!obj.m_hasFort)
                defIndex.variant = "";
            else
                defIndex.variant = "FORT";
        } else {
            std::set<std::string> buildingIds;
            for (auto* building : obj.m_buildings)
                buildingIds.insert(building->id);
            if (buildingIds.contains("fort1"))
                defIndex.variant = "FORT";
            if (buildingIds.contains("fort2"))
                defIndex.variant = "CIT";
            if (buildingIds.contains("fort3"))
                defIndex.variant = "CAS";
            if (buildingIds.contains("townHall3"))
                defIndex.variant = "CAP";
        }

        auto* def = obj.m_factionId ? obj.m_factionId->objectDefs.get(defIndex) : obj.m_randomId;
        assert(def);

        result.addItem(makeItemByDef(SpriteMap::Layer::Town, def, obj.m_pos)
                           .addInfo("player", playerToString(obj.m_player))
                           .addInfo("fort", (obj.m_hasFort ? "true" : "false")))
            ->m_keyColor
            = makePlayerColor(obj.m_player);
    }

    for (auto& fhHero : fhMap.m_wanderingHeroes) {
        auto* libraryHero = fhHero.m_data.m_army.hero.library;

        bool  water = fhMap.m_tileMap.get(fhHero.m_pos).m_terrainId->id == Core::LibraryTerrain::s_terrainWater;
        auto* item  = result.addItem(makeItemByDefId(SpriteMap::Layer::Hero, fhHero.getDefId(water), fhHero.m_pos, true)
                                        .setPriority(fhHero.m_isPrison ? SpriteMap::s_objectMaxPriority : SpriteMap::s_objectMaxPriority + 1));

        if (libraryHero) {
            item->addInfo("id", libraryHero->id);
        }
        if (!fhHero.m_isPrison) {
            SpriteMap::Item flagItem = makeItem(fhHero.m_pos + FHPos{ 0, -1 }, -2);

            flagItem.m_flagColor = makePlayerColor(fhHero.m_player);
            result.addItem(flagItem);
        }
    }
    for (auto& obj : fhMap.m_objects.m_resources) {
        rendered.insert(&obj);
        Core::LibraryObjectDefConstPtr def;
        def = obj.m_id->objectDefs.get({});

        auto* item = result.addItem(makeItemByDef(SpriteMap::Layer::Resource, def, obj.m_pos));
        addValueInfo(item, obj);
        {
            auto strCount = std::to_string(obj.m_amount);
            item->addInfo("amount", strCount);
            if (obj.m_amount >= 5000)
                strCount = std::to_string(obj.m_amount / 1000) + " K";
            item->m_overlayInfo        = strCount == "0" ? "?" : strCount;
            item->m_overlayInfoOffsetX = 0;
        }
    }

    for (auto& obj : fhMap.m_objects.m_resourcesRandom) {
        rendered.insert(&obj);
        std::string id = "avtrndm0";
        result.addItem(makeItemByDefId(SpriteMap::Layer::Resource, id, obj.m_pos));
    }

    for (auto& obj : fhMap.m_objects.m_artifacts) {
        rendered.insert(&obj);
        auto* def = obj.m_id->objectDefs.get({});

        auto* item = result.addItem(makeItemByDef(SpriteMap::Layer::Artifact, def, obj.m_pos).addInfo("id", obj.m_id->id));
        addValueInfo(item, obj);
        if (obj.m_id->scrollSpell) {
            item->m_overlayInfo        = obj.m_id->scrollSpell->presentationParams.shortName.ts.at("en_US");
            item->m_overlayInfoOffsetX = 0;
        }
    }
    for (auto& obj : fhMap.m_objects.m_artifactsRandom) {
        rendered.insert(&obj);
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
        result.addItem(makeItemByDefId(SpriteMap::Layer::Artifact, id, obj.m_pos));
    }

    for (auto& obj : fhMap.m_objects.m_monsters) {
        rendered.insert(&obj);
        auto pos = obj.m_pos;

        auto strCount = std::to_string(obj.m_count);

        SpriteMap::Item* item = nullptr;
        if (obj.m_randomLevel >= 0) {
            static const std::vector<std::string> s_randomIds{
                "avwmrnd0",
                "avwmon1",
                "avwmon2",
                "avwmon3",
                "avwmon4",
                "avwmon5",
                "avwmon6",
                "avwmon7",
            };
            auto id = s_randomIds[obj.m_randomLevel];
            item    = result.addItem(makeItemByDefId(SpriteMap::Layer::Monster, id, pos, true)
                                      .addInfo("id", id));

        } else {
            auto* def = obj.m_id->objectDefs.get({});
            item      = result.addItem(makeItemByDef(SpriteMap::Layer::Monster, def, pos, true)
                                      .addInfo("id", obj.m_id->id));
        }
        item->addInfo("count", strCount);
        item->addInfo("value", std::to_string(obj.m_guardValue));

        addValueInfo(item, obj);
        strCount            = strCount == "0" ? "?" : strCount;
        item->m_overlayInfo = strCount + (obj.m_upgradedStack == FHMonster::UpgradedStack::Yes ? " u" : "");
    }

    for (auto& obj : fhMap.m_objects.m_dwellings) {
        rendered.insert(&obj);
        auto* def        = obj.m_id->objectDefs.get(obj.m_defIndex);
        auto* item       = result.addItem(makeItemByDef(SpriteMap::Layer::Dwelling, def, obj.m_pos).addInfo("id", obj.m_id->id));
        item->m_keyColor = makePlayerColor(obj.m_player);
        addValueInfo(item, obj);
    }
    for (auto& obj : fhMap.m_objects.m_randomDwellings) {
        rendered.insert(&obj);
        auto* def        = obj.m_id;
        auto* item       = result.addItem(makeItemByDef(SpriteMap::Layer::Dwelling, def, obj.m_pos).addInfo("id", obj.m_id->id));
        item->m_keyColor = makePlayerColor(obj.m_player);
        addValueInfo(item, obj);
    }

    for (auto& obj : fhMap.m_objects.m_mines) {
        rendered.insert(&obj);
        auto* def        = obj.m_id->minesDefs.get(obj.m_defIndex);
        auto* item       = result.addItem(makeItemByDef(SpriteMap::Layer::Mine, def, obj.m_pos).addInfo("id", obj.m_id->id));
        item->m_keyColor = makePlayerColor(obj.m_player);
        addValueInfo(item, obj);
    }
    for (auto& obj : fhMap.m_objects.m_abandonedMines) {
        rendered.insert(&obj);
        auto* item = result.addItem(makeItemByVisitable(SpriteMap::Layer::Mine, obj));

        addValueInfo(item, obj);
    }

    for (auto& obj : fhMap.m_objects.m_banks) {
        rendered.insert(&obj);
        auto* def               = obj.m_id->objectDefs.get(obj.m_defIndex);
        auto  strCount          = obj.m_guardsVariant < 0 ? "R" : std::to_string(obj.m_guardsVariant + 1);
        auto* item              = result.addItem(makeItemByDef(SpriteMap::Layer::Bank, def, obj.m_pos).addInfo("id", obj.m_id->id).addInfo("size", strCount));
        item->m_overlayInfo     = strCount + (obj.m_upgradedStack == FHBank::UpgradedStack::Yes ? " u" : "");
        item->m_overlayInfoFont = 16;
        addValueInfo(item, obj);

        for (size_t index = 0; auto* art : obj.m_artifacts) {
            auto artPos = obj.m_pos;
            artPos.m_x -= 2;
            artPos.m_x -= index % 2;
            artPos.m_y -= index / 2;
            index++;

            auto* artdef  = art->objectDefs.get({});
            auto* artItem = result.addItem(makeItemByDef(SpriteMap::Layer::Artifact, artdef, artPos).setPriority(SpriteMap::s_objectMaxPriority + 2));
            estimateArtScore(art, artItem->m_score);
            artItem->m_opacity       = obj.m_artifacts.size() > 1 ? 0.9 : 0.7;
            artItem->m_blockMask     = {};
            artItem->m_isOverlayItem = true;
            addValueInfo(artItem, obj);
        }
    }
    for (auto& obj : fhMap.m_objects.m_obstacles) {
        rendered.insert(&obj);
        auto* def = obj.m_id->objectDefs.get(obj.m_defIndex);
        addValueInfo(result.addItem(makeItemByDef(SpriteMap::Layer::Decoration, def, obj.m_pos).addInfo("id", obj.m_id->id).setRowPriority(-1)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_visitables) {
        rendered.insert(&obj);
        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::GeneralVisitable, obj)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_controlledVisitables) {
        rendered.insert(&obj);
        auto* item       = result.addItem(makeItemByVisitable(SpriteMap::Layer::GeneralVisitable, obj));
        item->m_keyColor = makePlayerColor(obj.m_player);
        addValueInfo(item, obj);
    }
    for (auto& obj : fhMap.m_objects.m_shrines) {
        rendered.insert(&obj);
        auto* item = result.addItem(makeItemByVisitable(SpriteMap::Layer::Shrine, obj));
        addValueInfo(item, obj);
        if (obj.m_spellId) {
            item->m_overlayInfo        = obj.m_spellId->presentationParams.shortName.ts.at("en_US");
            item->m_overlayInfoOffsetX = 0;
        }
    }
    for (auto& obj : fhMap.m_objects.m_skillHuts) {
        rendered.insert(&obj);
        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::SkillHut, obj)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_scholars) {
        rendered.insert(&obj);
        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::Scholar, obj)), obj);
    }

    for (auto& obj : fhMap.m_objects.m_questHuts) {
        rendered.insert(&obj);
        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::QuestHut, obj)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_questGuards) {
        rendered.insert(&obj);
        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::QuestGuard, obj)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_localEvents) {
        rendered.insert(&obj);
        std::string id    = "avzevnt0";
        auto*       item  = result.addItem(makeItemByDefId(SpriteMap::Layer::Event, id, obj.m_pos));
        item->m_blockMask = g_oneTileMask;
        addValueInfo(item, obj);
    }
    for (auto& obj : fhMap.m_objects.m_signs) {
        rendered.insert(&obj);

        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::Decoration, obj)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_garisons) {
        rendered.insert(&obj);

        addValueInfo(result.addItem(makeItemByVisitable(SpriteMap::Layer::Bank, obj)), obj);
    }
    for (auto& obj : fhMap.m_objects.m_heroPlaceholders) {
        rendered.insert(&obj);
        auto pos = obj.m_pos;
        pos.m_x += 1;

        std::string id   = "ahplace";
        auto*       item = result.addItem(makeItemByDefId(SpriteMap::Layer::Hero, id, pos));
        addValueInfo(item, obj);
    }
    for (auto& obj : fhMap.m_objects.m_grails) {
        rendered.insert(&obj);
        std::string id   = "avzgrail";
        auto*       item = result.addItem(makeItemByDefId(SpriteMap::Layer::Artifact, id, obj.m_pos));
        addValueInfo(item, obj);
    }
    for (auto& obj : fhMap.m_objects.m_unknownObjects) {
        rendered.insert(&obj);

        std::string id   = obj.m_defId;
        auto*       item = result.addItem(makeItemByDefId(SpriteMap::Layer::Decoration, id, obj.m_pos).setRowPriority(-1));
        addValueInfo(item, obj);
    }

    for (auto& obj : fhMap.m_objects.m_pandoras) {
        rendered.insert(&obj);
        bool        water = fhMap.m_tileMap.get(obj.m_pos).m_terrainId->id == Core::LibraryTerrain::s_terrainWater;
        std::string id    = water ? "ava0128w" : "ava0128";
        auto*       item  = result.addItem(makeItemByDefId(SpriteMap::Layer::Pandora, id, obj.m_pos));
        if (!obj.m_reward.units.empty()) {
            auto pos = obj.m_pos;
            pos.m_x += 1;
            auto& unit  = obj.m_reward.units[0];
            auto* def   = unit.unit->objectDefs.get({});
            auto* itemG = result.addItem(makeItemByDef(SpriteMap::Layer::Monster, def, pos)
                                             .addInfo("id", unit.unit->id)
                                             .addInfo("count", std::to_string(unit.count)));
            addValueInfo(itemG, obj);
            itemG->m_opacity       = 0.7;
            itemG->m_isOverlayItem = true;
            itemG->m_blockMask     = {};
        }
        item->m_blockMask          = g_oneTileMask;
        item->m_overlayInfo        = obj.m_key;
        item->m_overlayInfoOffsetX = 0;
        addValueInfo(item, obj);
    }
    if (m_settings.m_strict) {
        if (hasMissingSprites)
            throw std::runtime_error("Some sprites are missing; make sure to convert all data from LOD files!");

        auto                            allObj = fhMap.m_objects.getAllObjects();
        std::set<const FHCommonObject*> allSet(allObj.cbegin(), allObj.cend());

        std::vector<const FHCommonObject*> diff;

        std::set_difference(allSet.begin(), allSet.end(), rendered.begin(), rendered.end(), std::inserter(diff, diff.begin()));

        if (diff.size() != 0)
            throw std::runtime_error("Not all objects are rendered: " + std::to_string(diff.size()));
    }

    result.m_width  = fhMap.m_tileMap.m_width;
    result.m_height = fhMap.m_tileMap.m_height;
    result.m_depth  = fhMap.m_tileMap.m_depth;

    auto checkCell = [&result](const SpriteMap::Cell& cell, int x, int y, int z) {
        for (const auto& item : cell.m_items) {
            for (auto& point : item.m_blockMask.m_blocked) {
                auto& merged     = result.m_planes[z].m_merged.m_rows[y + point.m_y].m_cells[x + point.m_x];
                merged.m_blocked = true;
                if (item.m_keyColor.isValid()) {
                    merged.m_colorPlayer = item.m_keyColor;
                    merged.m_player      = true;
                }
            }
            for (auto& point : item.m_blockMask.m_visitable)
                result.m_planes[z].m_merged.m_rows[y + point.m_y].m_cells[x + point.m_x].m_visitable = true;
        }
    };

    for (int z = 0; const auto& plane : result.m_planes) {
        for (const auto& [priority, grid] : plane.m_grids) {
            for (const auto& [rowIndex, rowSlice] : grid.m_rowsSlices) {
                for (const auto& [rowPriority, row] : rowSlice.m_rows) {
                    for (const auto& [colIndex, cell] : row.m_cells) {
                        checkCell(cell, colIndex, rowIndex, z);
                    }
                }
            }
        }
        z++;
    }

    return result;
}

}
