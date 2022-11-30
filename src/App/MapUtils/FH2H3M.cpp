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

#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibraryTerrain.hpp"

#define assume(cond) \
    if (!(cond)) \
        throw std::runtime_error(#cond " - violated, invalid FH map provided.");

namespace FreeHeroes {

namespace {

constexpr const int g_terrainDirt = 0;
constexpr const int g_terrainSand = 1;

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
    assume(src.m_width == src.m_height && src.m_width > 0);
    assume(src.m_version == Core::GameVersion::SOD);
    dest.m_format                 = MapFormat::SOD;
    dest.m_anyPlayers             = true;
    dest.m_tiles.m_size           = src.m_width;
    dest.m_tiles.m_hasUnderground = src.m_hasUnderground;
    dest.m_tiles.updateSize();
    dest.prepareArrays();

    auto fillZoneTerrain = [database, &dest](const FHZone& zone) {
        if (zone.m_terrain.empty())
            return;

        auto* terrain = database->terrains()->find(zone.m_terrain);
        assert(terrain);
        const int  offset = terrain->presentationParams.centerTilesOffset;
        const auto id     = static_cast<uint8_t>(terrain->legacyId);

        std::set<FHPos> tilesPos = zone.getResolved();
        for (const auto& pos : tilesPos) {
            auto& tile   = dest.m_tiles.get(pos.m_x, pos.m_y, pos.m_z);
            tile.terView = offset;
            tile.terType = id;
        }
    };

    auto terViews = rng->genSmallSequence(dest.m_tiles.m_tiles.size(), 23);
    for (auto& view : terViews) {
        if (view >= 20)
            view = rng->genSmall(23);
    }

    fillZoneTerrain(FHZone{ .m_terrain = src.m_defaultTerrain, .m_rect{ .m_pos{ 0, 0, 0 }, .m_width = src.m_width, .m_height = src.m_height } });
    for (auto& zone : src.m_zones)
        fillZoneTerrain(zone);

    for (size_t i = 0; auto& tile : dest.m_tiles.m_tiles)
        tile.terView += terViews[i++];

    for (uint8_t z = 0; z <= uint8_t(src.m_hasUnderground); ++z) {
        for (uint32_t y = 0; y < src.m_height; ++y) {
            const bool yNotStart = y > 0;
            const bool yNotEnd   = y < src.m_height - 1;
            for (uint32_t x = 0; x < src.m_width; ++x) {
                const bool xNotStart = x > 0;
                const bool xNotEnd   = x < src.m_width - 1;

                auto& tile = dest.m_tiles.get(x, y, z);
                /* TL  T  TR
                 *  L  X   R
                 * BL  B  BR
                 */

                const auto X  = tile.terType;
                const auto TL = yNotStart && xNotStart ? dest.m_tiles.get(x - 1, y - 1, z).terType : X;
                const auto T  = yNotStart && true ? dest.m_tiles.get(x, y - 1, z).terType : X;
                const auto TR = yNotStart && xNotEnd ? dest.m_tiles.get(x + 1, y - 1, z).terType : X;

                const auto L = xNotStart ? dest.m_tiles.get(x - 1, y, z).terType : X;
                const auto R = xNotEnd ? dest.m_tiles.get(x + 1, y, z).terType : X;

                const auto BL = yNotEnd && xNotStart ? dest.m_tiles.get(x - 1, y + 1, z).terType : X;
                const auto B  = yNotEnd && true ? dest.m_tiles.get(x, y + 1, z).terType : X;
                const auto BR = yNotEnd && xNotEnd ? dest.m_tiles.get(x + 1, y + 1, z).terType : X;

                const auto dR = X != R;
                const auto dL = X != L;
                const auto dT = X != T;
                const auto dB = X != B;

                const auto dTL = X != TL;
                const auto dTR = X != TR;
                const auto dBL = X != BL;
                const auto dBR = X != BR;

                if (X == g_terrainSand || X == g_terrainDirt)
                    continue;

                if (false) {
                } else if (dR && dT) {
                    tile.terView      = 0 + 20 * (R == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipHor;
                } else if (dL && dT) {
                    tile.terView      = 0 + 20 * (L == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = 0;
                } else if (dR && dB) {
                    tile.terView      = 0 + 20 * (R == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipHor | MapTile::TerrainFlipVert;
                } else if (dL && dB) {
                    tile.terView      = 0 + 20 * (L == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipVert;
                } else if (dR) {
                    tile.terView      = 4 + 20 * (R == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipHor;
                } else if (dL) {
                    tile.terView      = 4 + 20 * (L == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = 0;
                } else if (dT) {
                    tile.terView      = 8 + 20 * (T == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = 0;
                } else if (dB) {
                    tile.terView      = 8 + 20 * (B == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipVert;
                } else if (dTL) {
                    tile.terView      = 12 + 20 * (TL == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipHor | MapTile::TerrainFlipVert;
                } else if (dTR) {
                    tile.terView      = 12 + 20 * (TR == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipVert;
                } else if (dBL) {
                    tile.terView      = 12 + 20 * (BL == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = MapTile::TerrainFlipHor;
                } else if (dBR) {
                    tile.terView      = 12 + 20 * (BR == g_terrainSand) + rng->genSmall(3);
                    tile.extTileFlags = 0;
                }
            }
        }
    }

    dest.m_difficulty        = 0x01;
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
        if (defs.contains(def.m_animationFile))
            return defs.at(def.m_animationFile);
        dest.m_objectDefs.push_back(std::move(def));
        auto index                = static_cast<uint32_t>(dest.m_objectDefs.size() - 1);
        defs[def.m_animationFile] = index;
        return index;
    };

    auto makeCastleDef = [](Core::LibraryFactionConstPtr libraryFaction, Core::LibraryFaction::Presentation::TownIndex index) -> ObjectTemplate {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 32 }, .m_blockMask = { 255, 255, 255, 143, 7, 7 }, .m_id = static_cast<uint32_t>(MapObjectType::TOWN), .m_type = ObjectTemplate::Type::COMMON };
        res.m_subid = libraryFaction->legacyId;
        assert(!libraryFaction->presentationParams.townAnimations.empty());
        res.m_animationFile = libraryFaction->presentationParams.townAnimations[static_cast<int>(index)] + ".def";
        return res;
    };

    auto makeHeroDef = [](Core::LibraryHeroConstPtr hero) {
        ObjectTemplate res{ .m_visitMask = { 0, 0, 0, 0, 0, 64 }, .m_blockMask = { 255, 255, 255, 255, 255, 191 }, .m_id = static_cast<uint32_t>(MapObjectType::HERO), .m_type = ObjectTemplate::Type::HERO };

        auto resNameAdv     = hero->getAdventureSprite();
        res.m_animationFile = resNameAdv + "e.def";
        return res;
    };

    for (auto& fhTown : src.m_towns) {
        auto  playerIndex = static_cast<int>(fhTown.m_player);
        auto& h3player    = dest.m_players[playerIndex];
        if (fhTown.m_isMain) {
            h3player.hasMainTown   = true;
            h3player.posOfMainTown = int3fromPos(fhTown.m_pos, -townGateOffset);
        }

        auto cas1           = std::make_unique<MapTown>(dest.m_features);
        cas1->m_playerOwner = playerIndex;
        cas1->m_hasFort     = fhTown.m_hasFort;
        cas1->prepareArrays();
        auto* libraryFaction = factionsContainer->find(fhTown.m_faction);
        assert(libraryFaction);
        Core::LibraryFaction::Presentation::TownIndex index = Core::LibraryFaction::Presentation::TownIndex::Village;
        if (fhTown.m_hasFort)
            index = Core::LibraryFaction::Presentation::TownIndex::Fort;
        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhTown.m_pos), .m_defnum = getDefFileIndex(makeCastleDef(libraryFaction, index)), .m_impl = std::move(cas1) });
    }

    for (auto& fhHero : src.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto& h3player    = dest.m_players[playerIndex];

        auto* libraryHero = database->heroes()->find(fhHero.m_id);
        assert(libraryHero);

        if (fhHero.m_isMain)
            h3player.mainCustomHeroId = libraryHero->legacyId;

        auto her1                                   = std::make_unique<MapHero>(dest.m_features);
        her1->m_playerOwner                         = playerIndex;
        her1->m_subID                               = libraryHero->legacyId;
        dest.m_allowedHeroes[libraryHero->legacyId] = 0;

        dest.m_objects.push_back(Object{ .m_pos = int3fromPos(fhHero.m_pos, +1), .m_defnum = getDefFileIndex(makeHeroDef(libraryHero)), .m_impl = std::move(her1) });
    }
}

}
