/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ByteOrderStream.hpp"
#include "PropertyTree.hpp"
#include "MapObjects.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

using si8 = int8_t;
using ui8 = uint8_t;

using si16 = int16_t;
using ui16 = uint16_t;

using si32 = int32_t;
using ui32 = uint32_t;

using si64 = int64_t;
using ui64 = uint64_t;

// Typedef declarations
typedef ui8                   TFaction;
typedef si64                  TExpType;
typedef std::pair<si64, si64> TDmgRange;
typedef si32                  TBonusSubtype;
typedef si32                  TQuantity;
using TeamID = ui8;

constexpr const TeamID NO_TEAM{ 255 };

typedef int TRmgTemplateZoneId;

class int3 {
public:
    uint8_t x{ 0 }, y{ 0 }, z{ 0 };

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

/// The disposed hero struct describes which hero can be hired from which player.
struct DisposedHero {
    ui8         heroId   = 0;
    ui8         portrait = 0xFF; /// The portrait id of the hero, 0xFF is default.
    std::string name;
    ui8         players = 0; /// Who can hire this hero (bitfield).

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct SHeroName {
    uint8_t     heroId;
    std::string heroName;

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

enum class EAiTactic
{
    NONE = -1,
    RANDOM,
    WARRIOR,
    BUILDER,
    EXPLORER
};

/// The player info constains data about which factions are allowed, AI tactical settings,
/// the main hero name, where to generate the hero, whether the faction should be selected randomly,...
struct PlayerInfo {
    bool      canHumanPlay    = false;
    bool      canComputerPlay = false;
    EAiTactic aiTactic        = EAiTactic::RANDOM;

    uint16_t allowedFactionsBitmask{ 0 };
    uint8_t  isFactionRandom{ 0 };

    ///main hero instance (VCMI maps only)
    std::string mainHeroInstance;
    /// Player has a random main hero
    bool hasRandomHero = false;

    uint8_t     mainCustomHeroPortrait = 0xff;
    std::string mainCustomHeroName;

    uint8_t mainCustomHeroId = 0xff; /// ID of custom hero

    bool hasMainTown            = false;
    bool generateHeroAtMainTown = false;
    int3 posOfMainTown;
    ui8  team{ NO_TEAM }; /// The default value NO_TEAM

    ui8 generateHero{ 0 }; /// Unused.
    ui8 p7{ 0 };           /// Unknown and unused.
    /// Unused. Count of hero placeholders containing hero type.
    /// WARNING: powerPlaceholders sometimes gives false 0 (eg. even if there is one placeholder), maybe different meaning ???
    ui8 powerPlaceholders{ 0 };

    std::vector<SHeroName> heroesNames; /// list of placed heroes on the map
};

struct MapTile {
    uint8_t terType      = 0xff;
    uint8_t terView      = 0;
    uint8_t riverType    = 0;
    uint8_t riverDir     = 0;
    uint8_t roadType     = 0;
    uint8_t roadDir      = 0;
    uint8_t extTileFlags = 0;

    enum ExtFlags
    {
        TerrainFlipHor  = 0x01,
        TerrainFlipVert = 0x02,

        RiverFlipHor  = 0x04,
        RiverFlipVert = 0x08,

        RoadFlipHor  = 0x10,
        RoadFlipVert = 0x20,

        Coastal        = 0x40,
        FavorableWinds = 0x80,
    };

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct MapTileSet {
    int32_t m_size           = 0;
    bool    m_hasUnderground = false;

    std::vector<MapTile> m_tiles;

    MapTile& get(int x, int y, int isU)
    {
        const int uOffset = m_size * m_size * isU;
        const int yOffset = m_size * y;
        return m_tiles[uOffset + yOffset + x];
    }
    const MapTile& get(int x, int y, int isU) const
    {
        const int uOffset = m_size * m_size * isU;
        const int yOffset = m_size * y;
        return m_tiles[uOffset + yOffset + x];
    }

    void updateSize()
    {
        m_tiles.resize(m_size * m_size * (1 + m_hasUnderground));
    }
};

struct ObjectTemplate {
    std::string          m_animationFile;
    std::vector<uint8_t> m_visitMask;
    std::vector<uint8_t> m_blockMask;

    enum class Type
    {
        INVALID = 0,
        COMMON,
        CREATURE,
        HERO,
        ARTIFACT,
        RESOURCE,
    };

    uint16_t m_unknownFlag        = 255;
    uint16_t m_allowedTerrainMask = 255;
    uint32_t m_id                 = 0;
    uint32_t m_subid              = 0;
    Type     m_type               = Type::INVALID;
    uint8_t  m_drawPriority       = 0;

    void prepareArrays();

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct Object {
    int3     m_pos;
    uint32_t m_defnum = 0;

    std::shared_ptr<IMapObject> m_impl;
};

struct GlobalMapEvent {
    MapFormat m_format = MapFormat::Invalid;

    std::string m_name;
    std::string m_message;
    ResourceSet m_resourceSet;
    uint8_t     m_players       = 0;
    bool        m_humanAffected = true;

    uint8_t  m_computerAffected = 0;
    uint16_t m_firstOccurence   = 0;
    uint8_t  m_nextOccurence    = 0;

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};
struct FHMap;
struct H3Map {
    MapFormat m_format = MapFormat::Invalid;

    // header
    bool        m_anyPlayers = false;
    std::string m_mapName;
    std::string m_mapDescr;
    uint8_t     m_difficulty = 0;
    uint8_t     m_levelLimit = 0;

    std::vector<PlayerInfo> m_players;

    enum class VictoryConditionType
    {
        ARTIFACT,
        GATHERTROOP,
        GATHERRESOURCE,
        BUILDCITY,
        BUILDGRAIL,
        BEATHERO,
        CAPTURECITY,
        BEATMONSTER,
        TAKEDWELLINGS,
        TAKEMINES,
        TRANSPORTITEM,
        WINSTANDARD = 255
    };
    enum class LossConditionType
    {
        LOSSCASTLE,
        LOSSHERO,
        TIMEEXPIRES,
        LOSSSTANDARD = 255
    };

    VictoryConditionType m_victoryCondition = VictoryConditionType::WINSTANDARD;
    LossConditionType    m_lossCondition    = LossConditionType::LOSSSTANDARD;

    uint8_t                   m_teamCount = 0;
    std::vector<uint8_t>      m_teamSettings;
    std::vector<uint8_t>      m_allowedHeroes;
    uint32_t                  m_placeholderHeroes = 0;
    std::vector<DisposedHero> m_disposedHeroes;
    std::vector<uint8_t>      m_allowedArtifacts; // inversion from default value.
    std::vector<uint8_t>      m_allowedSpells;
    std::vector<uint8_t>      m_allowedSecSkills;

    uint32_t m_rumorCount = 0;

    MapTileSet                  m_tiles;
    std::vector<ObjectTemplate> m_objectDefs;
    std::vector<Object>         m_objects;

    std::vector<GlobalMapEvent> m_events;

    H3Map();

    void prepareArrays();
    void convertFromFH(const FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* rng);

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
    void ToJson(PropertyTree& data) const;
    void FromJson(const PropertyTree& data);
};

}
