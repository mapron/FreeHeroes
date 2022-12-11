/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FH2H3M.hpp"

#include "FHMap.hpp"
#include "H3MMap.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryObjectDef.hpp"

#define assume(cond) \
    if (!(cond)) \
        throw std::runtime_error(#cond " - violated, invalid FH map provided.");

namespace FreeHeroes {

namespace {

constexpr const std::string_view g_terrainDirt  = "sod.terrain.dirt";
constexpr const std::string_view g_terrainSand  = "sod.terrain.sand";
constexpr const std::string_view g_terrainWater = "sod.terrain.water";

H3Pos int3fromPos(FHPos pos, int xoffset = 0)
{
    return { (uint8_t) (pos.m_x + xoffset), (uint8_t) pos.m_y, (uint8_t) pos.m_z };
}

std::vector<uint8_t> reverseArray(std::vector<uint8_t> arr)
{
    std::reverse(arr.begin(), arr.end());
    return arr;
}
}

void convertFH2H3M(const FHMap& src, H3Map& dest, const Core::IGameDatabase* database, Core::IRandomGenerator* rng)
{
    dest              = {};
    dest.m_mapName    = src.m_name;
    dest.m_mapDescr   = src.m_descr;
    dest.m_difficulty = src.m_difficulty;
    auto tileMap      = src.m_tileMap;
    tileMap.updateSize();
    assume(tileMap.m_width == tileMap.m_height && tileMap.m_width > 0);

    dest.m_format                 = src.m_version == Core::GameVersion::SOD ? MapFormat::SOD : MapFormat::HOTA3;
    dest.m_anyPlayers             = true;
    dest.m_tiles.m_size           = tileMap.m_width;
    dest.m_tiles.m_hasUnderground = tileMap.m_depth > 1;
    dest.m_tiles.updateSize();
    dest.prepareArrays();

    auto fillZoneTerrain = [&tileMap](const FHZone& zone) {
        if (!zone.m_terrain)
            return;

        zone.placeOnMap(tileMap);
    };

    fillZoneTerrain(FHZone{ .m_terrain = src.m_defaultTerrain, .m_rect{ FHZone::Rect{ .m_pos{ 0, 0, 0 }, .m_width = tileMap.m_width, .m_height = tileMap.m_height } } });
    for (auto& zone : src.m_zones)
        fillZoneTerrain(zone);

    const auto*  dirtTerrain   = database->terrains()->find(std::string(g_terrainDirt));
    const auto*  sandTerrain   = database->terrains()->find(std::string(g_terrainSand));
    const auto*  waterTerrain  = database->terrains()->find(std::string(g_terrainWater));
    const size_t terrainsCount = database->terrains()->legacyOrderedIds().size();

    tileMap.correctTerrainTypes(dirtTerrain, sandTerrain, waterTerrain);
    tileMap.rngTiles(rng);

    for (uint8_t z = 0; z < tileMap.m_depth; ++z) {
        for (uint32_t y = 0; y < tileMap.m_height; ++y) {
            for (uint32_t x = 0; x < tileMap.m_width; ++x) {
                auto& tile              = tileMap.get(x, y, z);
                auto& destTile          = dest.m_tiles.get(x, y, z);
                destTile.m_terType      = static_cast<uint8_t>(tile.m_terrain->legacyId);
                destTile.m_terView      = tile.m_view;
                destTile.m_extTileFlags = 0;
                if (tile.m_flipHor)
                    destTile.m_extTileFlags |= MapTile::TerrainFlipHor;
                if (tile.m_flipVert)
                    destTile.m_extTileFlags |= MapTile::TerrainFlipVert;
                if (tile.m_coastal)
                    destTile.m_extTileFlags |= MapTile::Coastal;
            }
        }
    }

    const int townGateOffset = 2;

    auto* factionsContainer = database->factions();
    auto* defsContainer     = database->objectDefs();

    for (auto& [playerId, fhPlayer] : src.m_players) {
        auto  index    = static_cast<int>(playerId);
        auto& h3player = dest.m_players[index];

        h3player.m_canHumanPlay    = fhPlayer.m_humanPossible;
        h3player.m_canComputerPlay = fhPlayer.m_aiPossible;

        uint16_t factionsBitmask = 0;
        for (Core::LibraryFactionConstPtr faction : fhPlayer.m_startingFactions) {
            assume(faction != nullptr);
            factionsBitmask |= 1U << uint32_t(faction->legacyId);
        }
        h3player.m_allowedFactionsBitmask = factionsBitmask;
    }
    for (auto& bit : dest.m_allowedHeroes)
        bit = 1;

    std::map<std::string, uint32_t> defs;
    auto                            getDefFileIndex = [&dest, &defs](ObjectTemplate def) -> uint32_t {
        auto key = def.m_animationFile;
        if (defs.contains(key))
            return defs.at(key);
        dest.m_objectDefs.push_back(std::move(def));
        auto index = static_cast<uint32_t>(dest.m_objectDefs.size() - 1);
        defs[key]  = index;
        return index;
    };

    auto makeDefFromDb = [terrainsCount](Core::LibraryObjectDefConstPtr record, bool forceSoft = false) {
        assert(record);
        ObjectTemplate res{
            .m_animationFile = record->defFile + ".def",
            .m_blockMask     = reverseArray(record->blockMap),
            .m_visitMask     = reverseArray(record->visitMap),
            .m_terrainsHard  = reverseArray(forceSoft ? record->terrainsSoft : record->terrainsHard),
            .m_terrainsSoft  = reverseArray(record->terrainsSoft),

            .m_id           = static_cast<uint32_t>(record->objId),
            .m_subid        = static_cast<uint32_t>(record->subId),
            .m_type         = static_cast<ObjectTemplate::Type>(record->type),
            .m_drawPriority = static_cast<uint8_t>(record->priority),
        };
        res.m_terrainsHard.resize(terrainsCount);
        res.m_terrainsSoft.resize(terrainsCount);
        return res;
    };

    auto makeDefFromDbId = [defsContainer, &makeDefFromDb](std::string defId, bool forceSoft = false) {
        if (defId.ends_with(".def"))
            defId = defId.substr(0, defId.size() - 4);
        std::transform(defId.begin(), defId.end(), defId.begin(), [](unsigned char c) { return std::tolower(c); });
        return makeDefFromDb(defsContainer->find(defId), forceSoft);
    };

    auto makeHeroDef = [&makeDefFromDbId](Core::LibraryHeroConstPtr hero) {
        ObjectTemplate res = makeDefFromDbId("hero", true);

        auto resNameAdv     = hero->getAdventureSprite();
        res.m_animationFile = resNameAdv + "e.def";
        return res;
    };

    getDefFileIndex(makeDefFromDbId("avwmrnd0"));
    getDefFileIndex(makeDefFromDbId("avlholg0"));

    for (auto& fhTown : src.m_towns) {
        auto playerIndex = static_cast<int>(fhTown.m_player);
        if (playerIndex >= 0) {
            auto& h3player = dest.m_players[playerIndex];
            if (fhTown.m_isMain) {
                h3player.m_hasMainTown   = true;
                h3player.m_posOfMainTown = int3fromPos(fhTown.m_pos, -townGateOffset);
                h3player.m_generateHero  = true;
            }
        }
        auto cas1               = std::make_unique<MapTown>(dest.m_features);
        cas1->m_playerOwner     = playerIndex;
        cas1->m_hasFort         = fhTown.m_hasFort;
        cas1->m_questIdentifier = fhTown.m_questIdentifier;
        cas1->m_spellResearch   = fhTown.m_spellResearch;
        //cas1->m_formation       = 0xCC;
        cas1->prepareArrays();
        auto* libraryFaction = factionsContainer->find(fhTown.m_faction);
        assert(libraryFaction);
        ObjectTemplate res;
        if (!fhTown.m_defFile.empty()) {
            res = makeDefFromDbId(fhTown.m_defFile, true);
        } else {
            res = makeDefFromDb(libraryFaction->mapObjectDef);
        }
        res.m_subid = libraryFaction->legacyId;

        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhTown.m_pos), .m_defnum = getDefFileIndex(res), .m_impl = std::move(cas1) });
    }

    for (auto& fhHero : src.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto* libraryHero = database->heroes()->find(fhHero.m_id);
        assert(libraryHero);

        const uint8_t heroId = libraryHero->legacyId;

        if (playerIndex >= 0) {
            auto& h3player = dest.m_players[playerIndex];
            if (fhHero.m_isMain) {
                h3player.m_mainCustomHeroId = heroId;
                h3player.m_generateHero     = false;
            }
            h3player.m_heroesNames.push_back(SHeroName{ .m_heroId = heroId, .m_heroName = "" });
        }

        auto her1               = std::make_unique<MapHero>(dest.m_features);
        her1->m_playerOwner     = playerIndex;
        her1->m_subID           = heroId;
        her1->m_questIdentifier = fhHero.m_questIdentifier;
        her1->prepareArrays();
        dest.m_allowedHeroes[heroId] = 0;

        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhHero.m_pos, +1), .m_defnum = getDefFileIndex(makeHeroDef(libraryHero)), .m_impl = std::move(her1) });
    }

    for (auto& fhRes : src.m_objects.m_resources) {
        auto res1      = std::make_unique<MapResource>(dest.m_features);
        res1->m_amount = fhRes.m_amount;

        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhRes.m_pos), .m_defnum = getDefFileIndex(makeDefFromDb(fhRes.m_resource->mapObjectDef)), .m_impl = std::move(res1) });
    }

    for (auto& allowed : dest.m_allowedHeroes)
        allowed = 1;
    for (auto& allowed : dest.m_allowedArtifacts)
        allowed = 1;
    for (auto& allowed : dest.m_allowedSpells)
        allowed = 1;
    for (auto& allowed : dest.m_allowedSecSkills)
        allowed = 1;

    for (const auto& heroId : src.m_disabledHeroes) {
        const auto legacyId            = database->heroes()->find(heroId)->legacyId;
        dest.m_allowedHeroes[legacyId] = 0;
    }

    if (src.m_version == Core::GameVersion::HOTA) {
        // these artifact ids are just unexistent.
        dest.m_allowedArtifacts[145] = 0;
        dest.m_allowedArtifacts[144] = 0;
    }
    for (const auto& artId : src.m_disabledArtifacts) {
        const auto legacyId               = database->artifacts()->find(artId)->legacyId;
        dest.m_allowedArtifacts[legacyId] = 0;
    }

    for (const auto& spellId : src.m_disabledSpells) {
        const auto legacyId            = database->spells()->find(spellId)->legacyId;
        dest.m_allowedSpells[legacyId] = 0;
    }
    for (const auto& secSkillId : src.m_disabledSkills) {
        const auto legacyId               = database->secSkills()->find(secSkillId)->legacyId;
        dest.m_allowedSecSkills[legacyId] = 0;
    }
    for (auto& customHero : src.m_customHeroes) {
        const auto legacyId = customHero.m_army.hero.library->legacyId;
        auto&      destHero = dest.m_customHeroData[legacyId];
        destHero.prepareArrays();
        destHero.m_enabled                            = true;
        destHero.m_hasExp                             = customHero.m_hasExp;
        destHero.m_hasCustomBio                       = customHero.m_hasCustomBio;
        destHero.m_hasSkills                          = customHero.m_hasSecSkills;
        destHero.m_primSkillSet.m_hasCustomPrimSkills = customHero.m_hasPrimSkills;
        destHero.m_spellSet.m_hasCustomSpells         = customHero.m_hasSpells;
        if (customHero.m_hasPrimSkills) {
            auto& prim = destHero.m_primSkillSet.m_primSkills;
            prim.resize(4);
            std::tie(prim[0], prim[1]) = customHero.m_army.hero.currentBasePrimary.ad.asTuple();
            std::tie(prim[2], prim[3]) = customHero.m_army.hero.currentBasePrimary.magic.asTuple();
        }
        if (customHero.m_hasSecSkills) {
            for (auto& sk : customHero.m_army.hero.secondarySkills) {
                destHero.m_skills.push_back({ static_cast<uint8_t>(sk.skill->legacyId), static_cast<uint8_t>(sk.level) });
            }
        }
    }
}

}
