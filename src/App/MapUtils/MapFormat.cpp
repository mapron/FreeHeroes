/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MapFormat.hpp"
#include "FHMap.hpp"
#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibraryTerrain.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"
#include "MapFormatReflection.hpp"
#include "MapObjectsReflection.hpp"

#include <set>

#define assume(cond) \
    if (!(cond)) \
        throw std::runtime_error(#cond " - violated, invalid FH map provided.");

namespace FreeHeroes {

namespace {

int3 int3fromPos(FHPos pos, int xoffset = 0)
{
    return { (uint8_t) (pos.m_x + xoffset), (uint8_t) pos.m_y, (uint8_t) pos.m_z };
}

constexpr const int g_terrainDirt = 0;
constexpr const int g_terrainSand = 1;

}

void H3Map::VictoryCondition::ReadInternal(ByteOrderDataStreamReader& stream)
{
    uint8_t winCondition = 0;
    stream >> winCondition;
    m_type = static_cast<VictoryConditionType>(winCondition);
    if (m_type == VictoryConditionType::WINSTANDARD)
        return;

    stream >> m_allowNormalVictory >> m_appliesToAI;

    switch (m_type) {
        case VictoryConditionType::WINSTANDARD:
            assert(0);
            break;
        case VictoryConditionType::ARTIFACT:
        {
            if (m_features->m_artId16Bit)
                stream >> m_artID;
            else
                m_artID = stream.ReadScalar<uint8_t>();
            break;
        }
        case VictoryConditionType::GATHERTROOP:
        {
            if (m_features->m_stackId16Bit)
                stream >> m_creatureID;
            else
                m_creatureID = stream.ReadScalar<uint8_t>();
            stream >> m_creatureCount;
            break;
        }
        case VictoryConditionType::GATHERRESOURCE:
        {
            stream >> m_resourceID >> m_resourceAmount;
            break;
        }
        case VictoryConditionType::BUILDCITY:
        {
            stream >> m_pos >> m_hallLevel >> m_castleLevel;
        }
        case VictoryConditionType::BUILDGRAIL:
        {
            stream >> m_pos;
            break;
        }
        case VictoryConditionType::BEATHERO:
        {
            stream >> m_pos;
            break;
        }
        case VictoryConditionType::CAPTURECITY:
        {
            stream >> m_pos;
            break;
        }
        case VictoryConditionType::BEATMONSTER:
        {
            stream >> m_pos;
            break;
        }
        case VictoryConditionType::TAKEDWELLINGS:
        {
            break;
        }
        case VictoryConditionType::TAKEMINES:
        {
            break;
        }
        case VictoryConditionType::TRANSPORTITEM:
        {
            m_artID = stream.ReadScalar<uint8_t>();
            stream >> m_pos;
            break;
        }
    }
}

void H3Map::VictoryCondition::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << static_cast<uint8_t>(m_type);
    if (m_type == VictoryConditionType::WINSTANDARD)
        return;

    stream << m_allowNormalVictory << m_appliesToAI;

    switch (m_type) {
        case VictoryConditionType::WINSTANDARD:
            assert(0);
            break;
        case VictoryConditionType::ARTIFACT:
        {
            if (m_features->m_artId16Bit)
                stream << m_artID;
            else
                stream << static_cast<uint8_t>(m_artID);
            break;
        }
        case VictoryConditionType::GATHERTROOP:
        {
            if (m_features->m_stackId16Bit)
                stream << m_creatureID;
            else
                stream << static_cast<uint8_t>(m_creatureID);
            stream << m_creatureCount;
            break;
        }
        case VictoryConditionType::GATHERRESOURCE:
        {
            stream << m_resourceID << m_resourceAmount;
            break;
        }
        case VictoryConditionType::BUILDCITY:
        {
            stream << m_pos << m_hallLevel << m_castleLevel;
        }
        case VictoryConditionType::BUILDGRAIL:
        {
            stream << m_pos;
            break;
        }
        case VictoryConditionType::BEATHERO:
        {
            stream << m_pos;
            break;
        }
        case VictoryConditionType::CAPTURECITY:
        {
            stream << m_pos;
            break;
        }
        case VictoryConditionType::BEATMONSTER:
        {
            stream << m_pos;
            break;
        }
        case VictoryConditionType::TAKEDWELLINGS:
        {
            break;
        }
        case VictoryConditionType::TAKEMINES:
        {
            break;
        }
        case VictoryConditionType::TRANSPORTITEM:
        {
            stream << static_cast<uint8_t>(m_artID);
            stream << m_pos;
            break;
        }
    }
}

void H3Map::LossCondition::ReadInternal(ByteOrderDataStreamReader& stream)
{
    uint8_t lossCondition = 0;
    stream >> lossCondition;
    m_type = static_cast<LossConditionType>(lossCondition);
    if (m_type == LossConditionType::LOSSSTANDARD)
        return;

    switch (m_type) {
        case LossConditionType::LOSSSTANDARD:
            assert(0);
            break;
        case LossConditionType::LOSSCASTLE:
        {
            stream >> m_pos;
            break;
        }
        case LossConditionType::LOSSHERO:
        {
            stream >> m_pos;
            break;
        }
        case LossConditionType::TIMEEXPIRES:
        {
            stream >> m_daysPassed;
            break;
        }
    }
}

void H3Map::LossCondition::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << static_cast<uint8_t>(m_type);
    if (m_type == LossConditionType::LOSSSTANDARD)
        return;

    switch (m_type) {
        case LossConditionType::LOSSSTANDARD:
            assert(0);
            break;
        case LossConditionType::LOSSCASTLE:
        {
            stream << m_pos;
            break;
        }
        case LossConditionType::LOSSHERO:
        {
            stream << m_pos;
            break;
        }
        case LossConditionType::TIMEEXPIRES:
        {
            stream << m_daysPassed;
            break;
        }
    }
}

H3Map::H3Map()
{
    m_features                    = std::make_shared<MapFormatFeatures>();
    m_victoryCondition.m_features = m_features;
    m_lossCondition.m_features    = m_features;
}

void H3Map::prepareArrays()
{
    *m_features = MapFormatFeatures(m_format, m_hotaVer.m_ver1);

    m_players.resize(m_features->m_players);
    m_allowedHeroes.resize(m_features->m_heroesCount);
    m_allowedArtifacts.resize(m_features->m_artifactsCount);
    m_allowedSpells.resize(m_features->m_spellsRegularCount);
    m_allowedSecSkills.resize(m_features->m_secondarySkillCount);
    m_customHeroData.resize(m_features->m_heroesCount);

    for (auto& heroData : m_customHeroData) {
        heroData.m_artSet.m_features       = m_features;
        heroData.m_spellSet.m_features     = m_features;
        heroData.m_primSkillSet.m_features = m_features;
    }
}

void H3Map::convertFromFH(const FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* rng)
{
    *this      = {};
    m_mapName  = map.m_name;
    m_mapDescr = map.m_descr;
    assume(map.m_width == map.m_height && map.m_width > 0);
    assume(map.m_version == Core::GameVersion::SOD);
    m_format                 = MapFormat::SOD;
    m_anyPlayers             = true;
    m_tiles.m_size           = map.m_width;
    m_tiles.m_hasUnderground = map.m_hasUnderground;
    m_tiles.updateSize();
    prepareArrays();

    auto fillZoneTerrain = [database, this](const FHZone& zone) {
        if (zone.m_terrain.empty())
            return;

        auto* terrain = database->terrains()->find(zone.m_terrain);
        assert(terrain);
        const int  offset = terrain->presentationParams.centerTilesOffset;
        const auto id     = static_cast<uint8_t>(terrain->legacyId);

        std::set<FHPos> tilesPos = zone.getResolved();
        for (const auto& pos : tilesPos) {
            auto& tile   = m_tiles.get(pos.m_x, pos.m_y, pos.m_z);
            tile.terView = offset;
            tile.terType = id;
        }
    };

    auto terViews = rng->genSmallSequence(m_tiles.m_tiles.size(), 23);
    for (auto& view : terViews) {
        if (view >= 20)
            view = rng->genSmall(23);
    }

    fillZoneTerrain(FHZone{ .m_terrain = map.m_defaultTerrain, .m_rect{ .m_pos{ 0, 0, 0 }, .m_width = map.m_width, .m_height = map.m_height } });
    for (auto& zone : map.m_zones)
        fillZoneTerrain(zone);

    for (size_t i = 0; auto& tile : m_tiles.m_tiles)
        tile.terView += terViews[i++];

    for (uint8_t z = 0; z <= uint8_t(map.m_hasUnderground); ++z) {
        for (uint32_t y = 0; y < map.m_height; ++y) {
            const bool yNotStart = y > 0;
            const bool yNotEnd   = y < map.m_height - 1;
            for (uint32_t x = 0; x < map.m_width; ++x) {
                const bool xNotStart = x > 0;
                const bool xNotEnd   = x < map.m_width - 1;

                auto& tile = m_tiles.get(x, y, z);
                /* TL  T  TR
                 *  L  X   R
                 * BL  B  BR
                 */

                const auto X  = tile.terType;
                const auto TL = yNotStart && xNotStart ? m_tiles.get(x - 1, y - 1, z).terType : X;
                const auto T  = yNotStart && true ? m_tiles.get(x, y - 1, z).terType : X;
                const auto TR = yNotStart && xNotEnd ? m_tiles.get(x + 1, y - 1, z).terType : X;

                const auto L = xNotStart ? m_tiles.get(x - 1, y, z).terType : X;
                const auto R = xNotEnd ? m_tiles.get(x + 1, y, z).terType : X;

                const auto BL = yNotEnd && xNotStart ? m_tiles.get(x - 1, y + 1, z).terType : X;
                const auto B  = yNotEnd && true ? m_tiles.get(x, y + 1, z).terType : X;
                const auto BR = yNotEnd && xNotEnd ? m_tiles.get(x + 1, y + 1, z).terType : X;

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

    m_difficulty             = 0x01;
    const int townGateOffset = 2;

    auto* factionsContainer = database->factions();

    for (auto& [playerId, fhPlayer] : map.m_players) {
        auto  index    = static_cast<int>(playerId);
        auto& h3player = m_players[index];

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
    for (auto& bit : m_allowedHeroes)
        bit = 1;

    std::map<std::string, uint32_t> defs;
    auto                            getDefFileIndex = [this, &defs](ObjectTemplate def) -> uint32_t {
        if (defs.contains(def.m_animationFile))
            return defs.at(def.m_animationFile);
        m_objectDefs.push_back(std::move(def));
        auto index                = static_cast<uint32_t>(m_objectDefs.size() - 1);
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

    for (auto& fhTown : map.m_towns) {
        auto  playerIndex = static_cast<int>(fhTown.m_player);
        auto& h3player    = m_players[playerIndex];
        if (fhTown.m_isMain) {
            h3player.hasMainTown   = true;
            h3player.posOfMainTown = int3fromPos(fhTown.m_pos, -townGateOffset);
        }

        auto cas1           = std::make_unique<MapTown>(m_features);
        cas1->m_playerOwner = playerIndex;
        cas1->m_hasFort     = fhTown.m_hasFort;
        cas1->prepareArrays();
        auto* libraryFaction = factionsContainer->find(fhTown.m_faction);
        assert(libraryFaction);
        Core::LibraryFaction::Presentation::TownIndex index = Core::LibraryFaction::Presentation::TownIndex::Village;
        if (fhTown.m_hasFort)
            index = Core::LibraryFaction::Presentation::TownIndex::Fort;
        m_objects.push_back(Object{ .m_pos = int3fromPos(fhTown.m_pos), .m_defnum = getDefFileIndex(makeCastleDef(libraryFaction, index)), .m_impl = std::move(cas1) });
    }

    for (auto& fhHero : map.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto& h3player    = m_players[playerIndex];

        auto* libraryHero = database->heroes()->find(fhHero.m_id);
        assert(libraryHero);

        if (fhHero.m_isMain)
            h3player.mainCustomHeroId = libraryHero->legacyId;

        auto her1                              = std::make_unique<MapHero>(m_features);
        her1->m_playerOwner                    = playerIndex;
        her1->m_subID                          = libraryHero->legacyId;
        m_allowedHeroes[libraryHero->legacyId] = 0;

        m_objects.push_back(Object{ .m_pos = int3fromPos(fhHero.m_pos, +1), .m_defnum = getDefFileIndex(makeHeroDef(libraryHero)), .m_impl = std::move(her1) });
    }
}

void H3Map::ReadInternal(ByteOrderDataStreamReader& stream)
{
    *this = {};
    {
        int32_t format = 0;
        stream >> format;
        m_format = static_cast<MapFormat>(format);
        static const std::set<MapFormat> s_known{
            MapFormat::ROE,
            MapFormat::AB,
            MapFormat::SOD,
            MapFormat::HOTA1,
            MapFormat::HOTA2,
            MapFormat::HOTA3,
            MapFormat::WOG,
            MapFormat::VCMI,
        };
        if (!s_known.contains(m_format))
            m_format = MapFormat::Invalid;

        if (m_format == MapFormat::Invalid)
            throw std::runtime_error("Invalid map format:" + std::to_string(format));
    }
    if (m_format >= MapFormat::HOTA1) {
        stream >> m_hotaVer.m_ver1 >> m_hotaVer.m_ver2;
        if (m_hotaVer.m_ver1 == 3)
            stream >> m_hotaVer.m_ver3;
    }
    prepareArrays();

    stream >> m_anyPlayers >> m_tiles.m_size >> m_tiles.m_hasUnderground;
    stream >> m_mapName;
    stream >> m_mapDescr;
    stream >> m_difficulty;
    m_levelLimit = 0;
    if (m_features->m_mapLevelLimit)
        stream >> m_levelLimit;

    for (PlayerInfo& playerInfo : m_players) {
        stream >> playerInfo.canHumanPlay >> playerInfo.canComputerPlay;

        playerInfo.aiTactic = static_cast<EAiTactic>(stream.ReadScalar<uint8_t>());

        if (m_features->m_playerP7) {
            stream >> playerInfo.p7;
        } else {
            playerInfo.p7 = -1;
        }

        // Factions this player can choose
        if (m_features->m_factions16Bit)
            stream >> playerInfo.allowedFactionsBitmask;
        else
            playerInfo.allowedFactionsBitmask = stream.ReadScalar<uint8_t>();

        stream >> playerInfo.isFactionRandom >> playerInfo.hasMainTown;
        if (playerInfo.hasMainTown) {
            playerInfo.generateHeroAtMainTown = true;
            playerInfo.generateHero           = false;
            if (m_features->m_playerGenerateHeroInfo)
                stream >> playerInfo.generateHeroAtMainTown >> playerInfo.generateHero;

            stream >> playerInfo.posOfMainTown;
        }

        stream >> playerInfo.hasRandomHero >> playerInfo.mainCustomHeroId;

        if (playerInfo.mainCustomHeroId != 0xff) {
            stream >> playerInfo.mainCustomHeroPortrait;
            stream >> playerInfo.mainCustomHeroName;
        }

        if (m_features->m_playerPlaceholders)
            stream >> playerInfo.powerPlaceholders >> playerInfo.heroesNames;
    }

    stream >> m_victoryCondition >> m_lossCondition;

    stream >> m_teamCount;
    if (m_teamCount > 0) {
        m_teamSettings.resize(m_features->m_players);
        for (auto& player : m_teamSettings)
            stream >> player;
    }

    auto readBitsSized = [&stream](std::vector<uint8_t>& bitArray, bool sized) {
        if (sized) {
            auto s = stream.readSize();
            if (bitArray.size() != s) {
                throw std::runtime_error("Inconsistent bit array size, expected:" + std::to_string(bitArray.size()) + ", found:" + std::to_string(s));
            }
        }
        stream.readBits(bitArray);
    };

    readBitsSized(m_allowedHeroes, m_features->m_mapAllowedHeroesSized);

    if (m_features->m_mapPlaceholderHeroes) {
        stream >> m_placeholderHeroes;
        stream.zeroPadding(m_placeholderHeroes);
    }

    if (m_features->m_mapDisposedHeroes) {
        const auto disp = stream.ReadScalar<uint8_t>();
        m_disposedHeroes.resize(disp);
        for (auto& hero : m_disposedHeroes)
            stream >> hero;
    }

    //omitting NULLS
    stream.zeroPadding(31);

    if (m_features->m_mapHotaUnknown1) {
        uint32_t unknown1; // == 1;
        uint16_t unknown2; // == 16;
        uint32_t unknown3; // == 0;
        uint32_t unknown4; // == 0xffffffff;
        stream >> unknown1 >> unknown2 >> unknown3;
        if (m_hotaVer.m_ver1 == 3)
            stream >> unknown4;
    }

    if (m_features->m_mapAllowedArtifacts)
        readBitsSized(m_allowedArtifacts, m_features->m_mapAllowedArtifactsSized);

    if (m_features->m_mapAllowedSpells)
        stream.readBits(m_allowedSpells);

    if (m_features->m_mapAllowedSecSkills)
        stream.readBits(m_allowedSecSkills);

    stream >> m_rumors;

    if (m_features->m_mapCustomHeroData) {
        if (m_features->m_mapCustomHeroSize) {
            auto s = stream.readSize();
            if (m_customHeroData.size() != s)
                throw std::runtime_error("heroCount check failed for HOTA header");
        }
        int index = 0;
        for (auto& hero : m_customHeroData) {
            if (index == 12) {
                int a = 1;
            }
            stream >> hero;
            index++;
        }
    }

    m_tiles.updateSize();
    for (int ground = 0; ground < (1 + m_tiles.m_hasUnderground); ++ground) {
        for (int y = 0; y < m_tiles.m_size; y++) {
            for (int x = 0; x < m_tiles.m_size; x++) {
                stream >> m_tiles.get(x, y, ground);
            }
        }
    }

    stream >> m_objectDefs;

    {
        uint32_t count = 0;
        stream >> count;
        m_objects.resize(count);
        [[maybe_unused]] int objCounter = 0; // debug
        for (auto& obj : m_objects) {
            stream >> obj.m_pos >> obj.m_defnum;
            [[maybe_unused]] const auto offsetRead = stream.GetBuffer().GetOffsetRead(); // debug

            if (objCounter == 19930) {
                int a = 1;
            }

            const ObjectTemplate& objTempl = m_objectDefs.at(obj.m_defnum);
            MapObjectType         type     = static_cast<MapObjectType>(objTempl.m_id);
            stream.zeroPadding(5);
            obj.m_impl = IMapObject::Create(type, m_features);
            if (!obj.m_impl)
                throw std::runtime_error("Unsupported map object type:" + std::to_string(objTempl.m_id));

            obj.m_impl->ReadInternal(stream);
            objCounter++;
        }
    }

    m_events.resize(stream.readSize());
    for (auto& event : m_events) {
        event.m_features = m_features;
        stream >> event;
    }

    stream.zeroPadding(124);
}

void H3Map::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    const auto format = static_cast<int32_t>(m_format);
    stream << format;

    if (m_format >= MapFormat::HOTA1) {
        stream << m_hotaVer.m_ver1 << m_hotaVer.m_ver2;
        if (m_hotaVer.m_ver1 == 3)
            stream << m_hotaVer.m_ver3;
    }

    stream << m_anyPlayers << m_tiles.m_size << m_tiles.m_hasUnderground;
    stream << m_mapName;
    stream << m_mapDescr;
    stream << m_difficulty;
    if (m_features->m_mapLevelLimit)
        stream << m_levelLimit;

    for (const PlayerInfo& playerInfo : m_players) {
        stream << playerInfo.canHumanPlay << playerInfo.canComputerPlay;

        stream << static_cast<uint8_t>(playerInfo.aiTactic);

        if (m_features->m_playerP7) {
            stream << playerInfo.p7;
        }

        if (m_features->m_factions16Bit)
            stream << playerInfo.allowedFactionsBitmask;
        else
            stream << uint8_t(playerInfo.allowedFactionsBitmask);

        stream << playerInfo.isFactionRandom << playerInfo.hasMainTown;
        if (playerInfo.hasMainTown) {
            if (m_features->m_playerGenerateHeroInfo)
                stream << playerInfo.generateHeroAtMainTown << playerInfo.generateHero;

            stream << playerInfo.posOfMainTown;
        }

        stream << playerInfo.hasRandomHero << playerInfo.mainCustomHeroId;

        if (playerInfo.mainCustomHeroId != 0xff) {
            stream << playerInfo.mainCustomHeroPortrait;
            stream << playerInfo.mainCustomHeroName;
        }

        if (m_features->m_playerPlaceholders)
            stream << playerInfo.powerPlaceholders << playerInfo.heroesNames;
    }

    stream << m_victoryCondition << m_lossCondition;

    stream << m_teamCount;

    if (m_teamCount > 0) {
        for (auto& player : m_teamSettings)
            stream << player;
    }

    auto writeBitsSized = [&stream](const std::vector<uint8_t>& bitArray, bool sized) {
        if (sized)
            stream.writeSize(bitArray.size());
        stream.writeBits(bitArray);
    };

    writeBitsSized(m_allowedHeroes, m_features->m_mapAllowedHeroesSized);

    if (m_features->m_mapPlaceholderHeroes) {
        stream << m_placeholderHeroes;
        stream.zeroPadding(m_placeholderHeroes);
    }

    if (m_features->m_mapDisposedHeroes) {
        stream << static_cast<uint8_t>(m_disposedHeroes.size());
        for (auto& hero : m_disposedHeroes)
            stream << hero;
    }

    stream.zeroPadding(31);

    if (m_features->m_mapHotaUnknown1) {
        uint32_t unknown1 = 1;
        uint16_t unknown2 = 16;
        uint32_t unknown3 = 0;
        uint32_t unknown4 = 0xffffffff;
        stream << unknown1 << unknown2 << unknown3;
        if (m_hotaVer.m_ver1 == 3)
            stream << unknown4;
    }

    if (m_features->m_mapAllowedArtifacts)
        writeBitsSized(m_allowedArtifacts, m_features->m_mapAllowedArtifactsSized);

    if (m_features->m_mapAllowedSpells)
        stream.writeBits(m_allowedSpells);

    if (m_features->m_mapAllowedSecSkills)
        stream.writeBits(m_allowedSecSkills);

    stream << m_rumors;

    if (m_features->m_mapCustomHeroData) {
        if (m_features->m_mapCustomHeroSize)
            stream.writeSize(m_customHeroData.size());

        for (auto& hero : m_customHeroData)
            stream << hero;
    }

    for (int ground = 0; ground < (1 + m_tiles.m_hasUnderground); ++ground) {
        for (int y = 0; y < m_tiles.m_size; y++) {
            for (int x = 0; x < m_tiles.m_size; x++) {
                stream << m_tiles.get(x, y, ground);
            }
        }
    }

    stream << m_objectDefs;
    {
        uint32_t count = m_objects.size();
        stream << count;
        for (auto& obj : m_objects) {
            stream << obj.m_pos << obj.m_defnum;
            stream.zeroPadding(5);
            if (!obj.m_impl)
                throw std::runtime_error("Empty object impl on write!");

            obj.m_impl->WriteInternal(stream);
        }
    }

    stream << m_events;

    stream.zeroPadding(124);
}

void H3Map::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
    PropertyTree& objList = data["objects"];
    objList.convertToList();
    for (auto& obj : m_objects) {
        PropertyTree objJson;
        objJson.convertToMap();
        writer.valueToJson(obj.m_pos, objJson["pos"]);
        writer.valueToJson(obj.m_defnum, objJson["defnum"]);
        if (!obj.m_impl)
            throw std::runtime_error("Empty object impl on write!");
        obj.m_impl->ToJson(objJson["impl"]);
        objList.append(std::move(objJson));
    }
}

void H3Map::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
    *m_features = MapFormatFeatures(m_format, m_hotaVer.m_ver1);

    for (const PropertyTree& objJson : data["objects"].getList()) {
        Object obj;
        reader.jsonToValue(objJson["pos"], obj.m_pos);
        reader.jsonToValue(objJson["defnum"], obj.m_defnum);

        const ObjectTemplate& objTempl = m_objectDefs.at(obj.m_defnum);
        MapObjectType         type     = static_cast<MapObjectType>(objTempl.m_id);
        obj.m_impl                     = IMapObject::Create(type, m_features);
        if (!obj.m_impl)
            throw std::runtime_error("Unsupported map object type:" + std::to_string(objTempl.m_id));

        obj.m_impl->FromJson(objJson["impl"]);

        m_objects.push_back(std::move(obj));
    }
    for (auto& event : m_events)
        event.m_features = m_features;
    for (auto& heroData : m_customHeroData) {
        heroData.m_artSet.m_features       = m_features;
        heroData.m_spellSet.m_features     = m_features;
        heroData.m_primSkillSet.m_features = m_features;
    }
}

void int3::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> x >> y >> z;
}

void int3::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << x << y << z;
}

void SHeroName::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> heroId;
    stream >> heroName;
}

void SHeroName::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << heroId;
    stream << heroName;
}

void DisposedHero::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> heroId >> portrait;
    stream >> name;
    stream >> players;
}

void DisposedHero::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << heroId << portrait;
    stream << name;
    stream << players;
}

void MapTile::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> terType >> terView >> riverType >> riverDir >> roadType >> roadDir >> extTileFlags;
}

void MapTile::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << terType << terView << riverType << riverDir << roadType << roadDir << extTileFlags;
}

void ObjectTemplate::prepareArrays()
{
    m_visitMask.resize(6);
    m_blockMask.resize(6);
}

void ObjectTemplate::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_animationFile;

    prepareArrays();
    for (auto& byte : m_blockMask)
        stream >> byte;
    for (auto& byte : m_visitMask)
        stream >> byte;

    stream >> m_unknownFlag;
    stream >> m_allowedTerrainMask;
    stream >> m_id >> m_subid;
    m_type = static_cast<Type>(stream.ReadScalar<uint8_t>());
    stream >> m_drawPriority;

    stream.zeroPadding(16);
}

void ObjectTemplate::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_animationFile;

    for (auto& byte : m_blockMask)
        stream << byte;
    for (auto& byte : m_visitMask)
        stream << byte;

    stream << m_unknownFlag;
    stream << m_allowedTerrainMask;
    stream << m_id << m_subid;
    stream << static_cast<uint8_t>(m_type) << m_drawPriority;

    stream.zeroPadding(16);
}

void GlobalMapEvent::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_name >> m_message;
    stream >> m_resourceSet;
    stream >> m_players;

    if (m_features->m_mapEventHuman)
        stream >> m_humanAffected;

    stream >> m_computerAffected
        >> m_firstOccurence
        >> m_nextOccurence;

    stream.zeroPadding(17);
}

void GlobalMapEvent::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_name << m_message;
    stream << m_resourceSet;
    stream << m_players;

    if (m_features->m_mapEventHuman)
        stream << m_humanAffected;

    stream << m_computerAffected
           << m_firstOccurence
           << m_nextOccurence;

    stream.zeroPadding(17);
}

void CustomHeroData::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_enabled;
    if (!m_enabled)
        return;
    m_spellSet.prepareArrays();
    m_primSkillSet.prepareArrays();

    [[maybe_unused]] auto offset = stream.GetBuffer().GetOffsetRead();
    assert(m_enabled == 1U);
    stream >> m_hasExp;
    if (m_hasExp)
        stream >> m_exp;

    stream >> m_hasSkills;
    if (m_hasSkills) {
        auto size = stream.readSize();
        m_skills.resize(size);
        for (auto& sk : m_skills)
            stream >> sk.m_id >> sk.m_level;
    }

    stream >> m_artSet;
    stream >> m_hasCustomBio;
    if (m_hasCustomBio)
        stream >> m_bio;
    stream >> m_sex;

    [[maybe_unused]] auto offset2 = stream.GetBuffer().GetOffsetRead();

    stream >> m_spellSet >> m_primSkillSet;
}

void CustomHeroData::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_enabled;
    if (!m_enabled)
        return;

    stream << m_hasExp;
    if (m_hasExp)
        stream << m_exp;

    stream << m_hasSkills;
    if (m_hasSkills) {
        stream.writeSize(m_skills.size());
        for (auto& sk : m_skills)
            stream << sk.m_id << sk.m_level;
    }

    stream << m_artSet << m_hasCustomBio;
    if (m_hasCustomBio)
        stream << m_bio;
    stream << m_sex << m_spellSet << m_primSkillSet;
}

}
