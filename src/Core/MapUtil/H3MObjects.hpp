/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/ByteOrderStream.hpp"
#include "MernelPlatform/PropertyTree.hpp"

namespace FreeHeroes {
using ByteOrderDataStreamReader = Mernel::ByteOrderDataStreamReader;
using ByteOrderDataStreamWriter = Mernel::ByteOrderDataStreamWriter;
using PropertyTree              = Mernel::PropertyTree;

enum class MapFormat
{
    Invalid = 0,
    ROE     = 0x0e,
    AB      = 0x15,
    SOD     = 0x1c,
    HC      = 0x1d,
    HOTA1   = 0x1e,
    HOTA2   = 0x1f,
    HOTA3   = 0x20,
    WOG     = 0x33,
    VCMI    = 0xF0,
};

struct MapFormatFeatures {
    int m_factions             = 0;
    int m_artifactsCount       = 0;
    int m_heroesCount          = 0;
    int m_spellsCount          = 0;
    int m_spellsAbilitiesCount = 0;
    int m_spellsRegularCount   = 0;
    int m_creaturesCount       = 0;
    int m_primarySkillsCount   = 0;
    int m_terrainTypes         = 0;
    int m_resourceCount        = 0;
    int m_players              = 0;

    int m_stackSize           = 0;
    int m_secondarySkillCount = 0;

    bool   m_hasQuestIdentifier = false;
    size_t m_stackIdSize        = 2;
    size_t m_artIdSize          = 2;

    bool m_heroHasExp              = false;
    bool m_heroHasBio              = false;
    bool m_heroHasCustomSpells     = false;
    bool m_heroHasOneSpell         = false;
    bool m_heroHasPrimSkills       = false;
    bool m_heroHasSomethingUnknown = false;

    bool m_townHasObligatorySpells = false;
    bool m_townHasSpellResearch    = false;
    bool m_townHasAlignment        = false;
    bool m_townHasNewBuildingLogic = false;

    bool m_artifactHasPickupConditions = false;
    bool m_rewardHasMovePoints         = false;

    bool m_monsterJoinPercent = false;

    bool m_monsterHasQuantityByValue = false;

    bool m_creatureBankSize = false;

    bool m_extraRewardsCustomization = false;
    bool m_mineGuardsCustomization   = false;

    bool m_seerHutExtendedQuest  = false;
    bool m_seerHutMultiQuest     = false;
    bool m_witchHutAllowedSkills = false;

    bool m_garisonRemovableUnits = false;

    bool m_artifactMiscFive = false;

    bool m_eventHasHumanActivate       = false;
    bool m_townEventHasHumanAffected   = false;
    bool m_mapEventsHave4UnknownFields = false;
    bool m_townEventHave4UnknownFields = false;

    bool m_playerP7               = false;
    bool m_playerGenerateHeroInfo = false;
    bool m_playerPlaceholders     = false;

    bool m_mapLevelLimit        = false;
    bool m_mapPlaceholderHeroes = false;
    bool m_mapDisposedHeroes    = false;

    bool m_mapAllowedHeroesSized    = false;
    bool m_mapAllowedArtifacts      = false;
    bool m_mapAllowedArtifactsSized = false;
    bool m_mapAllowedSpells         = false;
    bool m_mapAllowedSecSkills      = false;

    bool m_mapCustomHeroData         = false;
    bool m_mapCustomHeroSize         = false;
    bool m_mapCustomHeroDataExtended = false;

    bool m_mapEventHuman = false;

    bool m_mapHotaUnknown1 = false;

    MapFormatFeatures() = default;
    MapFormatFeatures(MapFormat baseFormat, int hotaVer1);

    void readVarLength(ByteOrderDataStreamReader& stream, size_t byteSize, uint32_t& data) const
    {
        if (byteSize == 4) {
            stream >> data;
            return;
        }
        if (byteSize == 2) {
            uint16_t tmp = 0;
            stream >> tmp;
            if (tmp == uint16_t(-1))
                data = uint32_t(-1);
            else
                data = tmp;
            return;
        }
        uint8_t tmp = 0;
        stream >> tmp;
        if (tmp == uint8_t(-1))
            data = uint32_t(-1);
        else
            data = tmp;
    }
    void writeVarLength(ByteOrderDataStreamWriter& stream, size_t byteSize, const uint32_t& data) const
    {
        if (byteSize == 4) {
            stream << data;
            return;
        }
        if (byteSize == 2) {
            if (data == uint32_t(-1))
                stream << uint16_t(-1);
            else
                stream << static_cast<uint16_t>(data);
            return;
        }
        if (data == uint32_t(-1))
            stream << uint8_t(-1);
        else
            stream << static_cast<uint8_t>(data);
    }

    void readArtifact(ByteOrderDataStreamReader& stream, uint32_t& art, size_t maxSize = 4) const
    {
        readVarLength(stream, std::min(maxSize, m_artIdSize), art);
    }
    void writeArtifact(ByteOrderDataStreamWriter& stream, const uint32_t& art, size_t maxSize = 4) const
    {
        writeVarLength(stream, std::min(maxSize, m_artIdSize), art);
    }

    void readUnit(ByteOrderDataStreamReader& stream, uint32_t& art) const
    {
        readVarLength(stream, m_stackIdSize, art);
    }
    void writeUnit(ByteOrderDataStreamWriter& stream, const uint32_t& art) const
    {
        writeVarLength(stream, m_stackIdSize, art);
    }
    void readUnit(ByteOrderDataStreamReader& stream, uint16_t& art) const
    {
        uint32_t artTmp = 0;
        readVarLength(stream, m_stackIdSize, artTmp);
        art = artTmp;
    }
    void writeUnit(ByteOrderDataStreamWriter& stream, const uint16_t& art) const
    {
        writeVarLength(stream, m_stackIdSize, static_cast<uint32_t>(art));
    }
};

enum class MapObjectType
{
    NO_OBJ = -1,
    //    ALTAR_OF_SACRIFICE          = 2,
    //    ANCHOR_POINT                = 3,
    //    ARENA                       = 4,
    ARTIFACT     = 5,
    PANDORAS_BOX = 6,
    BLACK_MARKET = 7,
    //    BOAT                        = 8,
    //    BORDERGUARD                 = 9,
    //    KEYMASTER                   = 10,
    //    BUOY                        = 11,
    CAMPFIRE = 12,
    //    CARTOGRAPHER                = 13,
    //    SWAN_POND                   = 14,
    //    COVER_OF_DARKNESS           = 15,
    CREATURE_BANK       = 16,
    CREATURE_GENERATOR1 = 17,
    CREATURE_GENERATOR2 = 18,
    CREATURE_GENERATOR3 = 19,
    CREATURE_GENERATOR4 = 20,
    //    CURSED_GROUND1              = 21,
    CORPSE = 22,
    //    MARLETTO_TOWER              = 23,
    DERELICT_SHIP = 24,
    DRAGON_UTOPIA = 25,
    EVENT         = 26,
    //    EYE_OF_MAGI                 = 27,
    //    FAERIE_RING                 = 28,
    FLOTSAM = 29,
    //    FOUNTAIN_OF_FORTUNE         = 30,
    //    FOUNTAIN_OF_YOUTH           = 31,
    //    GARDEN_OF_REVELATION        = 32,
    GARRISON = 33,
    HERO     = 34,
    //    HILL_FORT                   = 35,
    GRAIL = 36,
    //    HUT_OF_MAGI                 = 37,
    //    IDOL_OF_FORTUNE             = 38,
    LEAN_TO = 39,
    //    UNUSED_1                    = 40,
    //    LIBRARY_OF_ENLIGHTENMENT    = 41,
    LIGHTHOUSE = 42,
    //    MONOLITH_ONE_WAY_ENTRANCE   = 43,
    //    MONOLITH_ONE_WAY_EXIT       = 44,
    //    MONOLITH_TWO_WAY            = 45,
    //    MAGIC_PLAINS1               = 46,
    //    SCHOOL_OF_MAGIC             = 47,
    //    MAGIC_SPRING                = 48,
    //    MAGIC_WELL                  = 49,
    //    MERCENARY_CAMP              = 51,
    //    MERMAID                     = 52,
    MINE    = 53,
    MONSTER = 54,
    //    MYSTICAL_GARDEN             = 55,
    //    OASIS                       = 56,
    //    OBELISK                     = 57,
    //    REDWOOD_OBSERVATORY         = 58,
    OCEAN_BOTTLE = 59,
    //    PILLAR_OF_FIRE              = 60,
    //    STAR_AXIS                   = 61,
    PRISON  = 62,
    PYRAMID = 63, //subtype 0
    //    WOG_OBJECT                  = 63, //subtype > 0
    //    RALLY_FLAG                  = 64,
    RANDOM_ART          = 65,
    RANDOM_TREASURE_ART = 66,
    RANDOM_MINOR_ART    = 67,
    RANDOM_MAJOR_ART    = 68,
    RANDOM_RELIC_ART    = 69,
    RANDOM_HERO         = 70,
    RANDOM_MONSTER      = 71,
    RANDOM_MONSTER_L1   = 72,
    RANDOM_MONSTER_L2   = 73,
    RANDOM_MONSTER_L3   = 74,
    RANDOM_MONSTER_L4   = 75,
    RANDOM_RESOURCE     = 76,
    RANDOM_TOWN         = 77,
    //    REFUGEE_CAMP                = 78,
    RESOURCE = 79,
    //    SANCTUARY                   = 80,
    SCHOLAR                     = 81,
    SEA_CHEST                   = 82,
    SEER_HUT                    = 83,
    CRYPT                       = 84,
    SHIPWRECK                   = 85,
    SHIPWRECK_SURVIVOR          = 86,
    SHIPYARD                    = 87,
    SHRINE_OF_MAGIC_INCANTATION = 88,
    SHRINE_OF_MAGIC_GESTURE     = 89,
    SHRINE_OF_MAGIC_THOUGHT     = 90,
    SIGN                        = 91,
    //    SIRENS                      = 92,
    SPELL_SCROLL = 93,
    //    STABLES                     = 94,
    //    TAVERN                      = 95,
    //    TEMPLE                      = 96,
    //    DEN_OF_THIEVES              = 97,
    TOWN = 98,
    //    TRADING_POST                = 99,
    //    LEARNING_STONE              = 100,
    TREASURE_CHEST    = 101,
    TREE_OF_KNOWLEDGE = 102,
    //    SUBTERRANEAN_GATE           = 103,
    UNIVERSITY          = 104,
    WAGON               = 105,
    WAR_MACHINE_FACTORY = 106,
    //    SCHOOL_OF_WAR               = 107,
    WARRIORS_TOMB = 108,
    //    WATER_WHEEL                 = 109,
    //    WATERING_HOLE               = 110,
    //    WHIRLPOOL                   = 111,
    //    WINDMILL                    = 112,
    WITCH_HUT = 113,

    //    BRUSH           = 114,
    //    BUSH            = 115,
    //    CACTUS          = 116,
    //    CANYON          = 117,
    //    CRATER          = 118,
    //    DEAD_VEGETATION = 119,
    //    FLOWERS         = 120,
    //    FROZEN_LAKE     = 121,
    //    HEDGE           = 122,
    //    HILL            = 123,

    HOLE = 124,
    // KELP = 125,
    //    LAKE        = 126,
    //    LAVA_FLOW   = 127,
    //    LAVA_LAKE   = 128,
    //    MUSHROOMS   = 129,
    //    LOG         = 130,
    //    MANDRAKE    = 131,
    //    MOSS        = 132,
    //    MOUND       = 133,
    //    MOUNTAIN    = 134,
    //    OAK_TREES   = 135,
    //    OUTCROPPING = 136,
    //    PINE_TREES  = 137,
    //    PLANT       = 138,

    //    DECORATION_1 = 139,
    //    DECORATION_2 = 140,
    //    UNUSED_2                  = 141,

    //    HOTA_STORAGE = 142,

    //    RIVER_DELTA = 143,

    //    HOTA_VISITABLE_1 = 144,
    HOTA_VISITABLE_2              = 145,
    HOTA_VISITABLE_2_ANCIENT_LAMP = 0,
    HOTA_VISITABLE_2_WATER_BARREL = 1,
    HOTA_VISITABLE_2_JETSAM       = 2,
    HOTA_VISITABLE_2_MANA_VIAL    = 3,

    HOTA_VISITABLE_3                  = 146,
    HOTA_VISITABLE_3_WATER_ACADEMY    = 0,
    HOTA_VISITABLE_3_WATER_OBSRVATORY = 1,
    HOTA_VISITABLE_3_ALTAR_OF_MANA    = 2,
    HOTA_VISITABLE_3_TOWN_GATE        = 3,

    //    ROCK          = 147,
    //    SAND_DUNE     = 148,
    //    SAND_PIT      = 149,
    //    SHRUB         = 150,
    //    SKULL         = 151,
    //    STALAGMITE    = 152,
    //    STUMP         = 153,
    //    TAR_PIT       = 154,
    //    TREES         = 155,
    //    VINE          = 156,
    //    VOLCANIC_VENT = 157,
    //    VOLCANO       = 158,
    //    WILLOW_TREES  = 159,
    //    YUCCA_TREES   = 160,
    //    REEF          = 161,

    RANDOM_MONSTER_L5 = 162,
    RANDOM_MONSTER_L6 = 163,
    RANDOM_MONSTER_L7 = 164,

    BORDER_GATE = 212,
    //    FREELANCERS_GUILD           = 213,
    HERO_PLACEHOLDER        = 214,
    QUEST_GUARD             = 215,
    RANDOM_DWELLING         = 216,
    RANDOM_DWELLING_LVL     = 217, //subtype = creature level
    RANDOM_DWELLING_FACTION = 218, //subtype = faction
    GARRISON2               = 219,
    ABANDONED_MINE          = 220,
    //    TRADING_POST_SNOW           = 221,
    //    CLOVER_FIELD                = 222,
    //    CURSED_GROUND2              = 223,
    //    EVIL_FOG                    = 224,
    //    FAVORABLE_WINDS             = 225,
    //    FIERY_FIELDS                = 226,
    //    HOLY_GROUNDS                = 227,
    //    LUCID_POOLS                 = 228,
    //    MAGIC_CLOUDS                = 229,
    //    MAGIC_PLAINS2               = 230,
    //    ROCKLANDS                   = 231,
};

inline const MapFormatFeatures* getFeaturesFromStream(Mernel::ByteOrderDataStream& stream)
{
    return stream.getUserData<const MapFormatFeatures>();
}

using MapFormatFeaturesPtr = std::shared_ptr<const MapFormatFeatures>;

struct IMapObject {
    virtual ~IMapObject() = default;

    virtual void readBinary(ByteOrderDataStreamReader& stream)        = 0;
    virtual void writeBinary(ByteOrderDataStreamWriter& stream) const = 0;
    virtual void toJson(PropertyTree& data) const                     = 0;
    virtual void fromJson(const PropertyTree& data)                   = 0;

    static std::unique_ptr<IMapObject> Create(MapObjectType type, uint32_t subid);
};

struct StackBasicDescriptor {
    uint16_t m_id    = uint16_t(-1);
    uint16_t m_count = 0;

    constexpr auto operator<=>(const StackBasicDescriptor&) const = default;
};

struct StackSet {
    std::vector<StackBasicDescriptor> m_stacks;

    void readBinary(ByteOrderDataStreamReader& stream)
    {
        auto* m_features = getFeaturesFromStream(stream);
        m_stacks.resize(stream.readSize());

        for (auto& stack : m_stacks) {
            m_features->readUnit(stream, stack.m_id);
            stream >> stack.m_count;
        }
    }
    void writeBinary(ByteOrderDataStreamWriter& stream) const
    {
        auto* m_features = getFeaturesFromStream(stream);
        auto  lock       = stream.setContainerSizeBytesGuarded(1);
        stream.writeSize(m_stacks.size());
        for (auto& stack : m_stacks) {
            m_features->writeUnit(stream, stack.m_id);
            stream << stack.m_count;
        }
    }
};

struct StackSetFixed {
    std::vector<StackBasicDescriptor> m_stacks;

    auto operator<=>(const StackSetFixed&) const = default;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        m_stacks.resize(m_features->m_stackSize);
    }

    void readBinary(ByteOrderDataStreamReader& stream)
    {
        auto* m_features = getFeaturesFromStream(stream);
        prepareArrays(m_features);
        for (auto& stack : m_stacks) {
            m_features->readUnit(stream, stack.m_id);
            stream >> stack.m_count;
        }
    }
    void writeBinary(ByteOrderDataStreamWriter& stream) const
    {
        auto* m_features = getFeaturesFromStream(stream);
        assert(m_features->m_stackSize == (int) m_stacks.size());
        for (auto& stack : m_stacks) {
            m_features->writeUnit(stream, stack.m_id);
            stream << stack.m_count;
        }
    }
};

struct ResourceSet {
    std::vector<uint32_t> m_resourceAmount;

    auto operator<=>(const ResourceSet&) const = default;

    void prepareArrays(const MapFormatFeatures* m_features) { m_resourceAmount.resize(m_features->m_resourceCount); }

    void readBinary(ByteOrderDataStreamReader& stream)
    {
        for (auto& res : m_resourceAmount)
            stream >> res;
    }
    void writeBinary(ByteOrderDataStreamWriter& stream) const
    {
        for (auto& res : m_resourceAmount)
            stream << res;
    }
};

struct PrimarySkillSet {
    std::vector<uint8_t> m_prim;

    auto operator<=>(const PrimarySkillSet&) const = default;

    void prepareArrays(const MapFormatFeatures* m_features) { m_prim.resize(m_features->m_primarySkillsCount); }

    void readBinary(ByteOrderDataStreamReader& stream)
    {
        auto* m_features = getFeaturesFromStream(stream);
        prepareArrays(m_features);
        for (auto& res : m_prim)
            stream >> res;
    }
    void writeBinary(ByteOrderDataStreamWriter& stream) const
    {
        for (auto& res : m_prim)
            stream << res;
    }
};

struct HeroArtSet {
    bool m_hasArts = false;

    std::vector<uint32_t> m_mainSlots;
    uint32_t              m_cata  = 0;
    uint32_t              m_book  = 0;
    uint32_t              m_misc5 = 0;
    std::vector<uint32_t> m_bagSlots;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        m_mainSlots.resize(16);
    }

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;

    auto operator<=>(const HeroArtSet&) const = default;
};

struct HeroSpellSet {
    bool                 m_hasCustomSpells = false;
    std::vector<uint8_t> m_spells;

    auto operator<=>(const HeroSpellSet&) const = default;

    void prepareArrays(const MapFormatFeatures* m_features) { m_spells.resize(m_features->m_spellsRegularCount); }

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct HeroPrimSkillSet {
    bool                 m_hasCustomPrimSkills = false;
    std::vector<uint8_t> m_primSkills;

    auto operator<=>(const HeroPrimSkillSet&) const = default;

    void prepareArrays(const MapFormatFeatures* m_features) { m_primSkills.resize(m_features->m_primarySkillsCount); }

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapGuards {
    bool          m_hasGuards = false;
    StackSetFixed m_creatures;

    void prepareArrays(const MapFormatFeatures* m_features) { m_creatures.prepareArrays(m_features); }

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapMessage {
    bool        m_hasMessage = false;
    std::string m_message;
    MapGuards   m_guards;

    void prepareArrays(const MapFormatFeatures* m_features) { m_guards.prepareArrays(m_features); }

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapObjectAbstract : public IMapObject {
};

struct MapObjectSimple : public MapObjectAbstract {
    void readBinary(ByteOrderDataStreamReader& stream) override {}
    void writeBinary(ByteOrderDataStreamWriter& stream) const override {}
    void toJson(PropertyTree& data) const override {}
    void fromJson(const PropertyTree& data) override {}
};

struct MapObjectWithOwner : public MapObjectAbstract {
    uint8_t m_owner = 0;

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapObjectCreatureBank : public MapObjectAbstract {
    uint32_t              m_content  = 0xffffffffU;
    uint8_t               m_upgraded = 0xffU;
    std::vector<uint32_t> m_artifacts;

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapHeroSkill {
    uint8_t m_id    = 0;
    uint8_t m_level = 0; //  (1 - basic, 2 - adv., 3 - expert)

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapHero : public MapObjectAbstract {
    uint32_t    m_questIdentifier = 0;
    uint8_t     m_playerOwner     = 0;
    uint8_t     m_subID           = 0;
    bool        m_hasName         = false;
    std::string m_name;
    bool        m_hasExp       = false;
    int32_t     m_exp          = -1;
    bool        m_hasPortrait  = false;
    uint8_t     m_portrait     = 0;
    bool        m_hasSecSkills = false;

    std::vector<MapHeroSkill> m_secSkills;

    bool          m_hasArmy = false;
    StackSetFixed m_garison;

    uint8_t    m_formation = 0;
    HeroArtSet m_artSet;

    uint8_t     m_patrolRadius = 0xff;
    bool        m_hasCustomBio = false;
    std::string m_bio;
    uint8_t     m_sex = 0xff;

    HeroSpellSet     m_spellSet;
    HeroPrimSkillSet m_primSkillSet;

    uint16_t m_unknown1 = 0;
    uint32_t m_unknown2 = 0;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapTownEvent {
    std::string m_name;
    std::string m_message;
    ResourceSet m_resourceSet;
    uint8_t     m_players          = 0;
    bool        m_humanAffected    = true;
    bool        m_computerAffected = false;
    uint16_t    m_firstOccurence   = 0;
    uint8_t     m_nextOccurence    = 0;

    std::vector<uint8_t>  m_buildings;
    std::vector<uint16_t> m_creaturesAmounts;

    uint32_t m_unknown1 = 0;
    uint32_t m_unknown2 = 0;
    uint32_t m_unknown3 = 0;
    uint16_t m_unknown4 = 0;

    auto operator<=>(const MapTownEvent&) const = default;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapTown : public MapObjectAbstract {
    uint32_t m_questIdentifier = 0;
    uint8_t  m_playerOwner     = 0;

    bool        m_hasName = false;
    std::string m_name;

    bool          m_hasGarison = false;
    StackSetFixed m_garison;

    uint8_t m_formation = 0;

    bool                 m_hasCustomBuildings = false;
    std::vector<uint8_t> m_builtBuildings;
    std::vector<uint8_t> m_forbiddenBuildings;
    bool                 m_hasFort = true;

    std::vector<uint8_t> m_obligatorySpells;
    std::vector<uint8_t> m_possibleSpells;

    bool m_spellResearch = false;

    std::vector<MapTownEvent> m_events;

    uint8_t m_alignment = 0xff;

    std::vector<uint8_t> m_somethingBuildingRelated;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapMonster : public MapObjectAbstract {
    uint32_t m_questIdentifier = 0;

    uint16_t m_count      = 0;
    uint8_t  m_joinAppeal = 0;

    bool        m_hasMessage = false;
    std::string m_message;
    ResourceSet m_resourceSet;
    uint32_t    m_artID          = uint32_t(-1);
    bool        m_neverFlees     = false;
    bool        m_notGrowingTeam = false;
    //uint16_t    m_unknown1       = 0;

    uint32_t m_aggressionExact  = 0xffffffffU;
    uint32_t m_joinPercent      = 100;
    bool     m_joinOnlyForMoney = false;
    uint32_t m_upgradedStack    = 0xffffffffU;
    uint32_t m_splitStack       = 0xffffffffU;

    uint8_t  m_quantityMode      = 0;
    uint32_t m_quantityByAiValue = 0;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapResource : public MapObjectAbstract {
    MapMessage m_message;
    uint32_t   m_amount = 0;

    void prepareArrays(const MapFormatFeatures* m_features) { m_message.prepareArrays(m_features); }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapArtifact : public MapObjectAbstract {
    MapMessage m_message;
    uint32_t   m_spellId = 0;
    bool       m_isSpell = false;

    uint32_t m_pickupCondition1 = 0;
    uint8_t  m_pickupCondition2 = 0;

    MapArtifact(bool isSpell)
        : m_isSpell(isSpell)
    {}

    void prepareArrays(const MapFormatFeatures* m_features) { m_message.prepareArrays(m_features); }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapQuest : public MapObjectAbstract {
    enum class Mission
    {
        NONE          = 0,
        LEVEL         = 1,
        PRIMARY_STAT  = 2,
        KILL_HERO     = 3,
        KILL_CREATURE = 4,
        ART           = 5,
        ARMY          = 6,
        RESOURCES     = 7,
        HERO          = 8,
        PLAYER        = 9,

        WAIT_FOR_DAY_OR_BE_CLASS = 10,
    };
    enum class Progress
    {
        NOT_ACTIVE,
        IN_PROGRESS,
        COMPLETE
    };

    Mission  m_missionType = Mission::NONE;
    Progress m_progress    = Progress::NOT_ACTIVE;
    int32_t  m_lastDay     = -1; //after this day (first day is 0) mission cannot be completed; if -1 - no limit
    int32_t  m_minimumDay  = -1;

    uint32_t              m_134val = 0;
    std::vector<uint8_t>  m_2stats;
    std::vector<uint32_t> m_5arts;      //artifacts id
    StackSet              m_6creatures; //pair[cre id, cre count], CreatureSet info irrelevant
    std::vector<uint32_t> m_7resources; //TODO: use resourceset?
    uint8_t               m_89val = 0;

    std::vector<uint8_t> m_expectClasses;
    uint32_t             m_dayClassDetermine = 0; // choose between new HotA missions. 0 = be class. 1 - wait for day.

    std::string m_firstVisitText;
    std::string m_nextVisitText;
    std::string m_completedText;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        m_2stats.resize(m_features->m_primarySkillsCount);
        m_7resources.resize(m_features->m_resourceCount);
        m_expectClasses.resize(m_features->m_factions * 2);
    }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapSeerHut : public MapObjectAbstract {
    struct MapQuestWithReward {
        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
        void toJson(PropertyTree& data) const;
        void fromJson(const PropertyTree& data);

        enum class RewardType
        {
            NOTHING,
            EXPERIENCE,
            MANA_POINTS,
            MORALE_BONUS,
            LUCK_BONUS,
            RESOURCES,
            PRIMARY_SKILL   = 6,
            SECONDARY_SKILL = 7,
            ARTIFACT,
            SPELL,
            CREATURE
        };

        RewardType m_reward = RewardType::NOTHING;
        uint32_t   m_rID    = 0; //reward ID
        uint32_t   m_rVal   = 0; //reward value

        using Mission  = MapQuest::Mission;
        using Progress = MapQuest::Progress;

        MapQuest m_quest;
    };

    std::vector<MapQuestWithReward> m_questsOneTime;
    std::vector<MapQuestWithReward> m_questsRecurring;

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapShrine : public MapObjectAbstract {
    uint8_t m_spell = 0;

    void readBinary(ByteOrderDataStreamReader& stream) override
    {
        stream >> m_spell;
        stream.zeroPadding(3);
    }
    void writeBinary(ByteOrderDataStreamWriter& stream) const override
    {
        stream << m_spell;
        stream.zeroPadding(3);
    }
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapScholar : public MapObjectAbstract {
    uint8_t m_bonusType = 0xFFU;
    uint8_t m_bonusId   = 0;

    void readBinary(ByteOrderDataStreamReader& stream) override
    {
        stream >> m_bonusType >> m_bonusId;
        stream.zeroPadding(6);
    }
    void writeBinary(ByteOrderDataStreamWriter& stream) const override
    {
        stream << m_bonusType << m_bonusId;
        stream.zeroPadding(6);
    }
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapWitchHut : public MapObjectAbstract {
    std::vector<uint8_t> m_allowedSkills;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        m_allowedSkills.resize(m_features->m_secondarySkillCount);
    }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapReward {
    uint32_t m_gainedExp  = 0;
    uint32_t m_manaDiff   = 0;
    int8_t   m_moraleDiff = 0;
    int8_t   m_luckDiff   = 0;

    ResourceSet               m_resourceSet;
    PrimarySkillSet           m_primSkillSet;
    std::vector<MapHeroSkill> m_secSkills;
    std::vector<uint32_t>     m_artifacts;
    std::vector<uint8_t>      m_spells;
    StackSet                  m_creatures;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
};

struct MapPandora : public MapObjectAbstract {
    MapMessage m_message;
    MapReward  m_reward;

    uint8_t  m_unknown1      = 0;
    uint32_t m_movePointMode = 0;
    uint32_t m_movePoints    = 0;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapGarison : public MapObjectAbstract {
    uint8_t       m_owner = 0;
    StackSetFixed m_garison;
    bool          m_removableUnits = true;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        m_garison.prepareArrays(m_features);
    }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapSignBottle : public MapObjectAbstract {
    std::string m_message;

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapEvent : public MapObjectAbstract {
    MapMessage m_message;
    MapReward  m_reward;

    uint32_t m_movePointMode = 0; // yes different from pandora
    uint32_t m_movePoints    = 0;

    std::vector<uint8_t> m_players;
    uint8_t              m_computerActivate = 0;
    uint8_t              m_removeAfterVisit = 0;
    uint8_t              m_humanActivate    = 1;

    void prepareArrays(const MapFormatFeatures* m_features);

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapDwelling : public MapObjectAbstract {
    bool m_hasFaction = true;
    bool m_hasLevel   = true;

    uint8_t  m_owner       = 0;
    uint32_t m_factionId   = 0;
    uint16_t m_factionMask = 0;
    uint8_t  m_minLevel    = 0;
    uint8_t  m_maxLevel    = 0;

    MapDwelling(bool hasFaction, bool hasLevel)
        : m_hasFaction(hasFaction)
        , m_hasLevel(hasLevel)
    {
    }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapQuestGuard : public MapObjectAbstract {
    MapQuest m_quest;

    void prepareArrays(const MapFormatFeatures* m_features) { m_quest.prepareArrays(m_features); }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

// diggable grave or tomb of warrior
struct MapVisitableWithReward : public MapObjectAbstract {
    enum class Type
    {
        Invalid,
        Grave,
        TreasureChest,
        SeaChest,
        Flotsam,
        TreeOfKnowledge,
        Lean,
        Jetsam,
        ManaVial,
        CampFire,
        Corpse,
        AncientLamp,
        Wagon,
        WaterBarrel,
        Survivor,
        WarriorTomb,
        Pyramid,
    };
    Type m_type = Type::Invalid;

    MapVisitableWithReward() = default;
    MapVisitableWithReward(Type type)
        : m_type(type)
    {}

    uint32_t m_customIndex    = 0; // for TC 0-2 is choice between 1000/1500/2000. for grave it is -1 (random) or 0 (custom)
    uint32_t m_artId          = 0; // for pyramid it is spell id
    uint32_t m_goldOrResource = 0; // for grave its is gold, for lean it's resource. for Lamp it's genie count

    // unused fields can be filled with random garbage. careful.
    uint8_t  m_resourceId           = 0;
    uint32_t m_goldOrResourceSecond = 0; // used by CampFire
    uint8_t  m_unknown1             = 0;

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapBlackMarket : public MapObjectAbstract {
    std::vector<uint32_t> m_artifacts;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        if (m_features->m_extraRewardsCustomization)
            m_artifacts.resize(7);
    }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapUniversity : public MapObjectAbstract {
    uint32_t             m_customIndex = 0;
    std::vector<uint8_t> m_allowedSkills;

    void prepareArrays(const MapFormatFeatures* m_features)
    {
        if (m_features->m_extraRewardsCustomization)
            m_allowedSkills.resize(m_features->m_secondarySkillCount);
    }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapGrail : public MapObjectAbstract {
    uint32_t m_radius = 0;

    void readBinary(ByteOrderDataStreamReader& stream) override { stream >> m_radius; }
    void writeBinary(ByteOrderDataStreamWriter& stream) const override { stream << m_radius; }
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapHeroPlaceholder : public MapObjectWithOwner {
    uint8_t m_hero      = 0;
    uint8_t m_powerRank = 0;

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

struct MapAbandonedMine : public MapObjectAbstract {
    std::vector<uint8_t> m_resourceBits;
    uint8_t              m_customGuards = 0;
    uint32_t             m_creatureId   = 0;
    uint32_t             m_countMin     = 0;
    uint32_t             m_countMax     = 0;

    void prepareArrays(const MapFormatFeatures* m_features) { m_resourceBits.resize(32); }

    void readBinary(ByteOrderDataStreamReader& stream) override;
    void writeBinary(ByteOrderDataStreamWriter& stream) const override;
    void toJson(PropertyTree& data) const override;
    void fromJson(const PropertyTree& data) override;
};

}
