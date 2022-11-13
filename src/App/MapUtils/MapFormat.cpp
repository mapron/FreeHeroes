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

namespace Core::Reflection {

template<>
inline constexpr const std::tuple MetaInfo::s_fields<int3>{
    Field("x", &int3::x),
    Field("y", &int3::y),
    Field("z", &int3::z),
};

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<EAiTactic> = EnumTraits::make(
    EAiTactic::NONE,
    "NONE", EAiTactic::NONE,
    "RANDOM", EAiTactic::RANDOM,
    "WARRIOR", EAiTactic::WARRIOR,
    "BUILDER", EAiTactic::BUILDER,
    "EXPLORER", EAiTactic::EXPLORER
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<SHeroName>{
    Field("heroId", &SHeroName::heroId),
    Field("heroName", &SHeroName::heroName),

};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<DisposedHero>{
    Field("heroId", &DisposedHero::heroId),
    Field("portrait", &DisposedHero::portrait),
    Field("name", &DisposedHero::name),
    Field("players", &DisposedHero::players),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<PlayerInfo>{
    Field("canHumanPlay", &PlayerInfo::canHumanPlay),
    Field("canComputerPlay", &PlayerInfo::canComputerPlay),
    Field("aiTactic", &PlayerInfo::aiTactic),
    Field("allowedFactionsBitmask", &PlayerInfo::allowedFactionsBitmask),
    Field("isFactionRandom", &PlayerInfo::isFactionRandom),
    Field("mainHeroInstance", &PlayerInfo::mainHeroInstance),
    Field("hasRandomHero", &PlayerInfo::hasRandomHero),
    Field("mainCustomHeroPortrait", &PlayerInfo::mainCustomHeroPortrait),
    Field("mainCustomHeroName", &PlayerInfo::mainCustomHeroName),
    Field("mainCustomHeroId", &PlayerInfo::mainCustomHeroId),
    Field("hasMainTown", &PlayerInfo::hasMainTown),
    Field("generateHeroAtMainTown", &PlayerInfo::generateHeroAtMainTown),
    Field("posOfMainTown", &PlayerInfo::posOfMainTown),
    Field("team", &PlayerInfo::team),
    Field("generateHero", &PlayerInfo::generateHero),
    Field("p7", &PlayerInfo::p7),
    Field("powerPlaceholders", &PlayerInfo::powerPlaceholders),
    Field("heroesNames", &PlayerInfo::heroesNames),

};

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<MapFormat> = EnumTraits::make(
    MapFormat::Invalid,
    "INVALID"  , MapFormat::Invalid,
    "ROE"      , MapFormat::ROE,
    "AB"       , MapFormat::AB,
    "SOD"      , MapFormat::SOD,
    "HOTA1"    , MapFormat::HOTA1,
    "HOTA2"    , MapFormat::HOTA2,
    "HOTA3"    , MapFormat::HOTA3,
    "WOG"      , MapFormat::WOG,
    "VCMI"     , MapFormat::VCMI
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<H3Map::VictoryConditionType> = EnumTraits::make(
    H3Map::VictoryConditionType::WINSTANDARD,
    "ARTIFACT"       , H3Map::VictoryConditionType::ARTIFACT,
    "GATHERTROOP"    , H3Map::VictoryConditionType::GATHERTROOP,
    "GATHERRESOURCE" , H3Map::VictoryConditionType::GATHERRESOURCE,
    "BUILDCITY"      , H3Map::VictoryConditionType::BUILDCITY,
    "BUILDGRAIL"     , H3Map::VictoryConditionType::BUILDGRAIL,
    "BEATHERO"       , H3Map::VictoryConditionType::BEATHERO,
    "CAPTURECITY"    , H3Map::VictoryConditionType::CAPTURECITY,
    "BEATMONSTER"    , H3Map::VictoryConditionType::BEATMONSTER,
    "TAKEDWELLINGS"  , H3Map::VictoryConditionType::TAKEDWELLINGS,
    "TAKEMINES"      , H3Map::VictoryConditionType::TAKEMINES,
    "TRANSPORTITEM"  , H3Map::VictoryConditionType::TRANSPORTITEM,
    "WINSTANDARD"    , H3Map::VictoryConditionType::WINSTANDARD
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<H3Map::LossConditionType> = EnumTraits::make(
    H3Map::LossConditionType::LOSSSTANDARD,
    "LOSSCASTLE"   , H3Map::LossConditionType::LOSSCASTLE,
    "LOSSHERO"     , H3Map::LossConditionType::LOSSHERO,
    "TIMEEXPIRES"  , H3Map::LossConditionType::TIMEEXPIRES,
    "LOSSSTANDARD" , H3Map::LossConditionType::LOSSSTANDARD
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<ObjectTemplate::Type> = EnumTraits::make(
    ObjectTemplate::Type::INVALID,
    "INVALID"  , ObjectTemplate::Type::INVALID,
    "COMMON"   , ObjectTemplate::Type::COMMON,
    "CREATURE" , ObjectTemplate::Type::CREATURE,
    "HERO"     , ObjectTemplate::Type::HERO,
    "ARTIFACT" , ObjectTemplate::Type::ARTIFACT,
    "RESOURCE" , ObjectTemplate::Type::RESOURCE
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTile>{
    Field("ex", &MapTile::extTileFlags),
    Field("rdd", &MapTile::roadDir),
    Field("rdt", &MapTile::roadType),
    Field("rid", &MapTile::riverDir),
    Field("rit", &MapTile::riverType),
    Field("v", &MapTile::terView),
    Field("t", &MapTile::terType),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTileSet>{
    Field("size", &MapTileSet::m_size),
    Field("hasUnderground", &MapTileSet::m_hasUnderground),
    Field("tiles", &MapTileSet::m_tiles),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<ObjectTemplate>{
    Field("animationFile", &ObjectTemplate::m_animationFile),
    Field("visitMask", &ObjectTemplate::m_visitMask),
    Field("blockMask", &ObjectTemplate::m_blockMask),

    Field("unknownFlag", &ObjectTemplate::m_unknownFlag),
    Field("allowedTerrainMask", &ObjectTemplate::m_allowedTerrainMask),
    Field("id", &ObjectTemplate::m_id),
    Field("subid", &ObjectTemplate::m_subid),
    Field("type", &ObjectTemplate::m_type),
    Field("drawPriority", &ObjectTemplate::m_drawPriority),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<GlobalMapEvent>{
    Field("name", &GlobalMapEvent::m_name),
    Field("message", &GlobalMapEvent::m_message),
    Field("resourceSet", &GlobalMapEvent::m_resourceSet),
    Field("players", &GlobalMapEvent::m_players),
    Field("humanAffected", &GlobalMapEvent::m_humanAffected),
    Field("computerAffected", &GlobalMapEvent::m_computerAffected),
    Field("firstOccurence", &GlobalMapEvent::m_firstOccurence),
    Field("nextOccurence", &GlobalMapEvent::m_nextOccurence),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Map>{

    Field("format", &H3Map::m_format),
    Field("anyPlayers", &H3Map::m_anyPlayers),
    Field("mapName", &H3Map::m_mapName),
    Field("mapDescr", &H3Map::m_mapDescr),
    Field("difficulty", &H3Map::m_difficulty),
    Field("levelLimit", &H3Map::m_levelLimit),

    Field("players", &H3Map::m_players),
    Field("victoryCondition", &H3Map::m_victoryCondition),
    Field("lossCondition", &H3Map::m_lossCondition),
    Field("teamCount", &H3Map::m_teamCount),
    Field("teamSettings", &H3Map::m_teamSettings),
    Field("allowedHeroes", &H3Map::m_allowedHeroes),
    Field("placeholderHeroes", &H3Map::m_placeholderHeroes),
    Field("disposedHeroes", &H3Map::m_disposedHeroes),
    Field("allowedArtifacts", &H3Map::m_allowedArtifacts),
    Field("allowedSpells", &H3Map::m_allowedSpells),
    Field("allowedSecSkills", &H3Map::m_allowedSecSkills),

    Field("rumors", &H3Map::m_rumorCount),
    Field("tiles", &H3Map::m_tiles),
    Field("objectDefs", &H3Map::m_objectDefs),

    Field("events", &H3Map::m_events),
};
// @todo: deduplicate
template<>
inline constexpr const std::tuple MetaInfo::s_fields<ResourceSet>{
    Field("resourceAmount", &ResourceSet::m_resourceAmount),
};
}

H3Map::H3Map()
{
}

void H3Map::prepareArrays()
{
    m_players.resize(8);
    m_allowedHeroes.resize(GameConstants(m_format).HEROES_QUANTITY);
    m_allowedArtifacts.resize(GameConstants(m_format).ARTIFACTS_QUANTITY);
    m_allowedSpells.resize(GameConstants(m_format).SPELLS_QUANTITY - GameConstants(m_format).ABILITIES_QUANTITY);
    m_allowedSecSkills.resize(GameConstants(m_format).SKILL_QUANTITY);
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

        auto cas1           = std::make_unique<MapTown>(m_format);
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

        auto her1                              = std::make_unique<MapHero>(m_format);
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
        uint32_t unknown1; // == 3;
        uint16_t unknown2; // == 0;
        uint32_t unknown3; // == 12;
        stream >> unknown1 >> unknown2 >> unknown3;
    }
    prepareArrays();

    stream >> m_anyPlayers >> m_tiles.m_size >> m_tiles.m_hasUnderground;
    stream >> m_mapName;
    stream >> m_mapDescr;
    stream >> m_difficulty;
    m_levelLimit = 0;
    if (m_format != MapFormat::ROE)
        stream >> m_levelLimit;

    for (PlayerInfo& playerInfo : m_players) {
        stream >> playerInfo.canHumanPlay >> playerInfo.canComputerPlay;

        playerInfo.aiTactic = static_cast<EAiTactic>(stream.ReadScalar<uint8_t>());

        if (m_format == MapFormat::SOD || m_format == MapFormat::WOG) {
            stream >> playerInfo.p7;
        } else {
            playerInfo.p7 = -1;
        }

        // Factions this player can choose
        if (m_format == MapFormat::ROE)
            playerInfo.allowedFactionsBitmask = stream.ReadScalar<uint8_t>();
        else
            stream >> playerInfo.allowedFactionsBitmask;

        stream >> playerInfo.isFactionRandom >> playerInfo.hasMainTown;
        if (playerInfo.hasMainTown) {
            playerInfo.generateHeroAtMainTown = true;
            playerInfo.generateHero           = false;
            if (m_format != MapFormat::ROE)
                stream >> playerInfo.generateHeroAtMainTown >> playerInfo.generateHero;

            stream >> playerInfo.posOfMainTown;
        }

        stream >> playerInfo.hasRandomHero >> playerInfo.mainCustomHeroId;

        if (playerInfo.mainCustomHeroId != 0xff) {
            stream >> playerInfo.mainCustomHeroPortrait;

            stream >> playerInfo.mainCustomHeroName;
        }

        if (m_format != MapFormat::ROE)
            stream >> playerInfo.powerPlaceholders >> playerInfo.heroesNames;
    }

    {
        uint8_t winCondition = 0;
        stream >> winCondition;
        m_victoryCondition = static_cast<VictoryConditionType>(winCondition);
        static const std::set<VictoryConditionType> s_supportedVC{
            VictoryConditionType::WINSTANDARD
        };
        if (!s_supportedVC.contains(m_victoryCondition))
            throw std::runtime_error("Unsupported victory condition:" + std::to_string(winCondition));
    }
    {
        uint8_t lossCondition = 0;
        stream >> lossCondition;
        m_lossCondition = static_cast<LossConditionType>(lossCondition);
        static const std::set<LossConditionType> s_supportedLC{
            LossConditionType::LOSSSTANDARD
        };
        if (!s_supportedLC.contains(m_lossCondition))
            throw std::runtime_error("Unsupported loss condition:" + std::to_string(lossCondition));
    }
    stream >> m_teamCount;
    if (m_teamCount > 0) {
        m_teamSettings.resize(GameConstants(m_format).PLAYER_LIMIT_I);
        for (auto& player : m_teamSettings)
            stream >> player;
    }

    stream.readBits(m_allowedHeroes);

    // Probably reserved for further heroes
    if (m_format > MapFormat::ROE) {
        stream >> m_placeholderHeroes;
        stream.zeroPadding(m_placeholderHeroes);
    }

    if (m_format >= MapFormat::SOD) {
        const auto disp = stream.ReadScalar<uint8_t>();
        m_disposedHeroes.resize(disp);
        for (auto& hero : m_disposedHeroes)
            stream >> hero;
    }

    //omitting NULLS
    stream.zeroPadding(31);

    if (m_format > MapFormat::ROE)
        stream.readBits(m_allowedArtifacts);

    if (m_format >= MapFormat::SOD) {
        stream.readBits(m_allowedSpells);
        stream.readBits(m_allowedSecSkills);
    }
    stream >> m_rumorCount;
    if (m_rumorCount > 0)
        throw std::runtime_error("Unsupported rumorCount:" + std::to_string(m_rumorCount));

    if (m_format >= MapFormat::SOD) {
        for (int z = 0; z < GameConstants(m_format).HEROES_QUANTITY; z++) {
            int custom = stream.ReadScalar<uint8_t>();
            if (!custom)
                continue;

            throw std::runtime_error("Unsupported custom heroes!");
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
        int objCounter = 0;
        for (auto& obj : m_objects) {
            stream >> obj.m_pos >> obj.m_defnum;

            const ObjectTemplate& objTempl = m_objectDefs.at(obj.m_defnum);
            MapObjectType         type     = static_cast<MapObjectType>(objTempl.m_id);
            stream.zeroPadding(5);
            obj.m_impl = IMapObject::Create(type, m_format);
            if (!obj.m_impl)
                throw std::runtime_error("Unsupported map object type:" + std::to_string(objTempl.m_id));

            obj.m_impl->ReadInternal(stream);
            objCounter++;
        }
    }

    m_events.resize(stream.readSize());
    for (auto& event : m_events) {
        event.m_format = m_format;
        stream >> event;
    }

    stream.zeroPadding(124);
}

void H3Map::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    const auto format = static_cast<int32_t>(m_format);
    stream << format;

    stream << m_anyPlayers << m_tiles.m_size << m_tiles.m_hasUnderground;
    stream << m_mapName;
    stream << m_mapDescr;
    stream << m_difficulty;
    if (m_format != MapFormat::ROE)
        stream << m_levelLimit;

    for (const PlayerInfo& playerInfo : m_players) {
        stream << playerInfo.canHumanPlay << playerInfo.canComputerPlay;

        stream << static_cast<uint8_t>(playerInfo.aiTactic);

        if (m_format == MapFormat::SOD || m_format == MapFormat::WOG) {
            stream << playerInfo.p7;
        }

        // Factions this player can choose
        if (m_format == MapFormat::ROE)
            stream << uint8_t(playerInfo.allowedFactionsBitmask);
        else
            stream << playerInfo.allowedFactionsBitmask;

        stream << playerInfo.isFactionRandom << playerInfo.hasMainTown;
        if (playerInfo.hasMainTown) {
            if (m_format != MapFormat::ROE)
                stream << playerInfo.generateHeroAtMainTown << playerInfo.generateHero;

            stream << playerInfo.posOfMainTown;
        }

        stream << playerInfo.hasRandomHero << playerInfo.mainCustomHeroId;

        if (playerInfo.mainCustomHeroId != 0xff) {
            stream << playerInfo.mainCustomHeroPortrait;

            stream << playerInfo.mainCustomHeroName;
        }

        if (m_format != MapFormat::ROE)
            stream << playerInfo.powerPlaceholders << playerInfo.heroesNames;
    }

    stream << static_cast<uint8_t>(m_victoryCondition) << static_cast<uint8_t>(m_lossCondition);
    stream << m_teamCount;

    if (m_teamCount > 0) {
        for (auto& player : m_teamSettings)
            stream << player;
    }

    stream.writeBits(m_allowedHeroes);

    if (m_format > MapFormat::ROE) {
        stream << m_placeholderHeroes;
        stream.zeroPadding(m_placeholderHeroes);
    }

    if (m_format >= MapFormat::SOD) {
        stream << static_cast<uint8_t>(m_disposedHeroes.size());
        for (auto& hero : m_disposedHeroes)
            stream << hero;
    }

    stream.zeroPadding(31);

    if (m_format > MapFormat::ROE)
        stream.writeBits(m_allowedArtifacts);

    if (m_format >= MapFormat::SOD) {
        stream.writeBits(m_allowedSpells);
        stream.writeBits(m_allowedSecSkills);
    }

    stream << m_rumorCount;

    if (m_format >= MapFormat::SOD) {
        for (int z = 0, cnt = GameConstants(m_format).HEROES_QUANTITY; z < cnt; z++) {
            uint8_t isCustom = 0;
            stream << isCustom;
        }
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
    for (const PropertyTree& objJson : data["objects"].getList()) {
        Object obj;
        reader.jsonToValue(objJson["pos"], obj.m_pos);
        reader.jsonToValue(objJson["defnum"], obj.m_defnum);

        const ObjectTemplate& objTempl = m_objectDefs.at(obj.m_defnum);
        MapObjectType         type     = static_cast<MapObjectType>(objTempl.m_id);
        obj.m_impl                     = IMapObject::Create(type, m_format);
        if (!obj.m_impl)
            throw std::runtime_error("Unsupported map object type:" + std::to_string(objTempl.m_id));

        obj.m_impl->FromJson(objJson["impl"]);

        m_objects.push_back(std::move(obj));
    }
    for (auto& event : m_events)
        event.m_format = m_format;
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

    if (m_format > MapFormat::AB)
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

    if (m_format > MapFormat::AB)
        stream << m_humanAffected;

    stream << m_computerAffected
           << m_firstOccurence
           << m_nextOccurence;

    stream.zeroPadding(17);
}

}
