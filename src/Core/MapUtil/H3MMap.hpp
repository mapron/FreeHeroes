/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/ByteOrderStream.hpp"
#include "MernelPlatform/PropertyTree.hpp"
#include "H3MObjects.hpp"

#include <set>

namespace FreeHeroes {

class H3Pos {
public:
    uint8_t m_x{ 0 }, m_y{ 0 }, m_z{ 0 };

    auto operator<=>(const H3Pos&) const = default;

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

/// The disposed hero struct describes which hero can be hired from which player.
struct DisposedHero {
    uint8_t     m_heroId   = 0;
    uint8_t     m_portrait = 0xFF; /// The portrait id of the hero, 0xFF is default.
    std::string m_name;
    uint8_t     m_players = 0; /// Who can hire this hero (bitfield).

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct SHeroName {
    uint8_t     m_heroId;
    std::string m_heroName;

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

enum class AiTactic
{
    NONE = -1,
    RANDOM,
    WARRIOR,
    BUILDER,
    EXPLORER
};

struct PlayerInfo {
    bool     m_canHumanPlay    = false;
    bool     m_canComputerPlay = false;
    AiTactic m_aiTactic        = AiTactic::RANDOM;

    uint16_t m_allowedFactionsBitmask{ 0 };
    uint8_t  m_isFactionRandom{ 0 };

    /// Player has a random main hero
    bool m_hasRandomHero = false;

    uint8_t     m_mainCustomHeroPortrait = 0xff;
    std::string m_mainCustomHeroName;

    uint8_t m_mainCustomHeroId = 0xff; /// ID of custom hero

    bool    m_hasMainTown            = false;
    bool    m_generateHeroAtMainTown = false;
    H3Pos   m_posOfMainTown;
    uint8_t m_team{ 0xff }; /// The default value NO_TEAM

    uint8_t m_generatedHeroTownFaction{ 0 }; /// faction of town generated hero will be placed. That can differ from starting faction.
    uint8_t m_unused1{ 0 };
    uint8_t m_placeholder{ 0 };

    std::vector<SHeroName> m_heroesNames; /// list of placed heroes on the map
};

struct MapTileH3M {
    uint8_t m_terType      = 0xff;
    uint8_t m_terView      = 0;
    uint8_t m_riverType    = 0;
    uint8_t m_riverDir     = 0;
    uint8_t m_roadType     = 0;
    uint8_t m_roadDir      = 0;
    uint8_t m_extTileFlags = 0;

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

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapTileSet {
    int32_t m_size           = 0;
    bool    m_hasUnderground = false;

    std::vector<MapTileH3M> m_tiles;

    MapTileH3M& get(int x, int y, int isU)
    {
        const int uOffset = m_size * m_size * isU;
        const int yOffset = m_size * y;
        return m_tiles[uOffset + yOffset + x];
    }
    const MapTileH3M& get(int x, int y, int isU) const
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
    MapFormatFeaturesPtr m_features;

    std::string          m_animationFile;
    std::vector<uint8_t> m_blockMask;
    std::vector<uint8_t> m_visitMask;
    std::vector<uint8_t> m_terrainsHard;
    std::vector<uint8_t> m_terrainsSoft;

    enum class Type
    {
        INVALID = 0,
        COMMON,
        CREATURE,
        HERO,
        ARTIFACT,
        RESOURCE,
    };

    uint32_t m_id           = 0;
    uint32_t m_subid        = 0;
    Type     m_type         = Type::INVALID;
    uint8_t  m_drawPriority = 0;

    void prepareArrays();

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct Object {
    int      m_order = 0;
    H3Pos    m_pos;
    uint32_t m_defnum = 0;

    std::shared_ptr<IMapObject> m_impl;
};

struct GlobalMapEvent {
    MapFormatFeaturesPtr m_features;

    std::string m_name;
    std::string m_message;
    ResourceSet m_resourceSet;
    uint8_t     m_players       = 0;
    bool        m_humanAffected = true;

    uint8_t  m_computerAffected = 0;
    uint16_t m_firstOccurence   = 0;
    uint8_t  m_nextOccurence    = 0;

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};
struct CustomHeroData {
    struct SecSkill {
        uint8_t m_id    = 0;
        uint8_t m_level = 0;

        constexpr auto operator<=>(const SecSkill&) const = default;
    };
    MapFormatFeaturesPtr m_features;

    uint8_t m_enabled = 0;

    bool     m_hasExp = false;
    uint32_t m_exp    = 0;

    bool                  m_hasSkills = false;
    std::vector<SecSkill> m_skills;

    HeroArtSet       m_artSet;
    bool             m_hasCustomBio = false;
    std::string      m_bio;
    uint8_t          m_sex = 0xFF;
    HeroSpellSet     m_spellSet;
    HeroPrimSkillSet m_primSkillSet;

    void prepareArrays();

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;

    auto operator<=>(const CustomHeroData&) const = default;
};

struct FHMap;
struct H3Map {
    MapFormat m_format = MapFormat::Invalid;
    struct HotaVersion {
        uint32_t m_ver1 = 3;
        uint16_t m_ver2 = 0;
        uint32_t m_ver3 = 12;

        uint32_t m_allowSpecialWeeks = 1;
        uint32_t m_roundLimit        = 0xffffffff;
    };
    HotaVersion                        m_hotaVer;
    std::shared_ptr<MapFormatFeatures> m_features;

    // header
    bool        m_anyPlayers = true;
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

    struct VictoryCondition {
        MapFormatFeaturesPtr m_features;
        VictoryConditionType m_type = VictoryConditionType::WINSTANDARD;

        bool m_allowNormalVictory = false;
        bool m_appliesToAI        = false;

        uint16_t m_artID = 0;

        uint16_t m_creatureID    = 0;
        uint16_t m_creatureCount = 0;

        uint8_t  m_resourceID     = 0;
        uint32_t m_resourceAmount = 0;

        H3Pos   m_pos;
        uint8_t m_hallLevel   = 0;
        uint8_t m_castleLevel = 0;

        auto operator<=>(const VictoryCondition&) const = default;

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
    };
    struct LossCondition {
        MapFormatFeaturesPtr m_features;
        LossConditionType    m_type       = LossConditionType::LOSSSTANDARD;
        uint16_t             m_daysPassed = 0;
        H3Pos                m_pos;

        auto operator<=>(const LossCondition&) const = default;

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
    };

    VictoryCondition m_victoryCondition;
    LossCondition    m_lossCondition;

    uint8_t                   m_teamCount = 0;
    std::vector<uint8_t>      m_allowedHeroes;
    std::vector<uint8_t>      m_placeholderHeroes;
    std::vector<DisposedHero> m_disposedHeroes;
    std::vector<uint8_t>      m_allowedArtifacts; // inversion from default value.
    std::vector<uint8_t>      m_allowedSpells;
    std::vector<uint8_t>      m_allowedSecSkills;

    struct Rumor {
        std::string m_name;
        std::string m_text;

        void readBinary(ByteOrderDataStreamReader& stream) { stream >> m_name >> m_text; }
        void writeBinary(ByteOrderDataStreamWriter& stream) const { stream << m_name << m_text; }
    };

    std::vector<Rumor>          m_rumors;
    std::vector<CustomHeroData> m_customHeroData;

    MapTileSet                  m_tiles;
    std::vector<ObjectTemplate> m_objectDefs;
    std::vector<Object>         m_objects;

    std::vector<GlobalMapEvent> m_events;

    // when saving from SoD editor, some bytes contains random garbage data.
    // we will save offsets in file for that, to perform byte equality check as close as possible.
    std::set<size_t> m_ignoredOffsets;

    H3Map();

    void prepareArrays();

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);
};

}
