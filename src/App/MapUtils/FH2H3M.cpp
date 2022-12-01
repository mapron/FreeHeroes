/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FH2H3M.hpp"

#include "FHMap.hpp"
#include "MapFormat.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryTerrain.hpp"

#define assume(cond) \
    if (!(cond)) \
        throw std::runtime_error(#cond " - violated, invalid FH map provided.");

namespace FreeHeroes {

namespace {

constexpr const std::string_view g_terrainDirt  = "sod.terrain.dirt";
constexpr const std::string_view g_terrainSand  = "sod.terrain.sand";
constexpr const std::string_view g_terrainWater = "sod.terrain.water";

int3 int3fromPos(FHPos pos, int xoffset = 0)
{
    return { (uint8_t) (pos.m_x + xoffset), (uint8_t) pos.m_y, (uint8_t) pos.m_z };
}
}

void convertFH2H3M(const FHMap& src, H3Map& dest, const Core::IGameDatabase* database, Core::IRandomGenerator* rng)
{
    dest            = {};
    dest.m_mapName  = src.m_name;
    dest.m_mapDescr = src.m_descr;
    auto tileMap    = src.m_tileMap;
    tileMap.updateSize();
    assume(tileMap.m_width == tileMap.m_height && tileMap.m_width > 0);

    dest.m_format                 = src.m_version == Core::GameVersion::SOD ? MapFormat::SOD : MapFormat::HOTA3;
    dest.m_anyPlayers             = true;
    dest.m_tiles.m_size           = tileMap.m_width;
    dest.m_tiles.m_hasUnderground = tileMap.m_depth > 1;
    dest.m_tiles.updateSize();
    dest.prepareArrays();

    dest.m_difficulty = 0x04; // @todo:

    auto fillZoneTerrain = [&tileMap](const FHZone& zone) {
        if (!zone.m_terrain)
            return;

        zone.placeOnMap(tileMap);
    };

    fillZoneTerrain(FHZone{ .m_terrain = src.m_defaultTerrain, .m_rect{ FHZone::Rect{ .m_pos{ 0, 0, 0 }, .m_width = tileMap.m_width, .m_height = tileMap.m_height } } });
    for (auto& zone : src.m_zones)
        fillZoneTerrain(zone);

    const auto* dirtTerrain  = database->terrains()->find(std::string(g_terrainDirt));
    const auto* sandTerrain  = database->terrains()->find(std::string(g_terrainSand));
    const auto* waterTerrain = database->terrains()->find(std::string(g_terrainWater));

    tileMap.correctTerrainTypes(dirtTerrain, sandTerrain, waterTerrain);
    tileMap.rngTiles(rng);

    for (uint8_t z = 0; z < tileMap.m_depth; ++z) {
        for (uint32_t y = 0; y < tileMap.m_height; ++y) {
            for (uint32_t x = 0; x < tileMap.m_width; ++x) {
                auto& tile            = tileMap.get(x, y, z);
                auto& destTile        = dest.m_tiles.get(x, y, z);
                destTile.terType      = static_cast<uint8_t>(tile.m_terrain->legacyId);
                destTile.terView      = tile.m_view;
                destTile.extTileFlags = 0;
                if (tile.m_flipHor)
                    destTile.extTileFlags |= MapTile::TerrainFlipHor;
                if (tile.m_flipVert)
                    destTile.extTileFlags |= MapTile::TerrainFlipVert;
                if (tile.m_coastal)
                    destTile.extTileFlags |= MapTile::Coastal;
            }
        }
    }

    const int townGateOffset = 2;

    auto* factionsContainer = database->factions();

    for (auto& [playerId, fhPlayer] : src.m_players) {
        auto  index    = static_cast<int>(playerId);
        auto& h3player = dest.m_players[index];

        h3player.canHumanPlay    = fhPlayer.m_humanPossible;
        h3player.canComputerPlay = fhPlayer.m_aiPossible;

        uint16_t factionsBitmask = 0;
        for (std::string allowedFaction : fhPlayer.m_startingFactions) {
            auto* faction = factionsContainer->find(allowedFaction);
            assume(faction != nullptr);
            factionsBitmask |= 1U << uint32_t(faction->legacyId);
        }
        h3player.allowedFactionsBitmask = factionsBitmask;
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

    auto makeRandomMonsterDef = []() -> ObjectTemplate {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 128 }, .m_blockMask = { 255, 255, 255, 255, 255, 127 }, .m_id = static_cast<uint32_t>(MapObjectType::RANDOM_MONSTER), .m_type = ObjectTemplate::Type::CREATURE };
        res.m_animationFile      = "AVWmrnd0.def";
        res.m_allowedTerrainMask = 1;
        res.m_unknownFlag        = 255;
        //res.m_drawPriority       = 0xCC;
        return res;
    };
    auto makeHoleDef = []() -> ObjectTemplate {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 0 }, .m_blockMask = { 255, 255, 255, 255, 255, 255 }, .m_id = static_cast<uint32_t>(MapObjectType::HOLE), .m_type = ObjectTemplate::Type::INVALID };
        res.m_animationFile      = "AVLholg0.def";
        res.m_allowedTerrainMask = 4;
        res.m_unknownFlag        = 4;
        res.m_drawPriority       = 1;
        return res;
    };

    auto makeCastleDef = [](Core::LibraryFactionConstPtr libraryFaction, Core::LibraryFaction::Presentation::TownIndex index) -> ObjectTemplate {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 32 }, .m_blockMask = { 255, 255, 255, 143, 7, 7 }, .m_id = static_cast<uint32_t>(MapObjectType::TOWN), .m_type = ObjectTemplate::Type::COMMON };
        res.m_subid = libraryFaction->legacyId;
        assert(!libraryFaction->presentationParams.townAnimations.empty());
        res.m_animationFile      = libraryFaction->presentationParams.townAnimations[static_cast<int>(index)] + ".def";
        res.m_allowedTerrainMask = 3327;
        res.m_unknownFlag        = 3327;
        return res;
    };

    auto makeHeroDef = [](Core::LibraryHeroConstPtr hero) {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 64 }, .m_blockMask = { 255, 255, 255, 255, 255, 191 }, .m_id = static_cast<uint32_t>(MapObjectType::HERO), .m_type = ObjectTemplate::Type::HERO };

        auto resNameAdv          = hero->getAdventureSprite();
        res.m_animationFile      = resNameAdv + "e.def";
        res.m_allowedTerrainMask = 3327;
        res.m_unknownFlag        = 3327;
        return res;
    };

    auto makeGoldDef = []() -> ObjectTemplate {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 128 }, .m_blockMask = { 255, 255, 255, 255, 255, 127 }, .m_id = static_cast<uint32_t>(MapObjectType::RESOURCE), .m_type = ObjectTemplate::Type::RESOURCE };
        res.m_animationFile      = "AVTgold0.def";
        res.m_allowedTerrainMask = 1;
        res.m_unknownFlag        = 3583;
        res.m_subid              = 6;
        return res;
    };

    getDefFileIndex(makeRandomMonsterDef());
    getDefFileIndex(makeHoleDef());

    for (auto& fhTown : src.m_towns) {
        auto  playerIndex = static_cast<int>(fhTown.m_player);
        auto& h3player    = dest.m_players[playerIndex];
        if (fhTown.m_isMain) {
            h3player.hasMainTown   = true;
            h3player.posOfMainTown = int3fromPos(fhTown.m_pos, -townGateOffset);
            h3player.generateHero  = true;
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
        Core::LibraryFaction::Presentation::TownIndex index = Core::LibraryFaction::Presentation::TownIndex::Village;
        if (fhTown.m_hasFort)
            index = Core::LibraryFaction::Presentation::TownIndex::Castle; // weird. HotA use def files for castle anyway in H3M, whatever is really placed on map.
        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhTown.m_pos), .m_defnum = getDefFileIndex(makeCastleDef(libraryFaction, index)), .m_impl = std::move(cas1) });
    }

    for (auto& fhHero : src.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto& h3player    = dest.m_players[playerIndex];

        auto* libraryHero = database->heroes()->find(fhHero.m_id);
        assert(libraryHero);

        const uint8_t heroId = libraryHero->legacyId;

        if (fhHero.m_isMain) {
            h3player.mainCustomHeroId = heroId;
            h3player.generateHero     = false;
        }
        h3player.heroesNames.push_back(SHeroName{ .heroId = heroId, .heroName = "" });

        auto her1                    = std::make_unique<MapHero>(dest.m_features);
        her1->m_playerOwner          = playerIndex;
        her1->m_subID                = heroId;
        her1->m_questIdentifier      = fhHero.m_questIdentifier;
        dest.m_allowedHeroes[heroId] = 0;

        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhHero.m_pos, +1), .m_defnum = getDefFileIndex(makeHeroDef(libraryHero)), .m_impl = std::move(her1) });
    }

    for (auto& fhRes : src.m_objects.m_resources) {
        auto res1      = std::make_unique<MapResource>(dest.m_features);
        res1->m_amount = fhRes.m_amount;

        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhRes.m_pos), .m_defnum = getDefFileIndex(makeGoldDef()), .m_impl = std::move(res1) });
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
        const auto legacyId                           = customHero.m_army.hero.library->legacyId;
        auto&      destHero                           = dest.m_customHeroData[legacyId];
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
