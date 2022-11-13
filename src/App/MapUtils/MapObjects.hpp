/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ByteOrderStream.hpp"
#include "PropertyTree.hpp"

namespace FreeHeroes {

enum class MapFormat
{
    Invalid = 0,
    ROE     = 0x0e,
    AB      = 0x15,
    SOD     = 0x1c,
    HOTA1   = 0x1e,
    HOTA2   = 0x1f,
    HOTA3   = 0x20,
    WOG     = 0x33,
    VCMI    = 0xF0,
};

struct GameConstants {
    int F_NUMBER;
    int ARTIFACTS_QUANTITY;
    int HEROES_QUANTITY;
    int SPELLS_QUANTITY;
    int ABILITIES_QUANTITY;
    int CREATURES_COUNT;
    int SKILL_QUANTITY;
    int PRIMARY_SKILLS;
    int TERRAIN_TYPES;
    int RESOURCE_QUANTITY;
    int HEROES_PER_TYPE;
    int PLAYER_LIMIT_I;
    int STACK_SIZE;

    GameConstants(MapFormat format);
};

enum class MapObjectType
{
    NO_OBJ                      = -1,
    ALTAR_OF_SACRIFICE          = 2,
    ANCHOR_POINT                = 3,
    ARENA                       = 4,
    ARTIFACT                    = 5,
    PANDORAS_BOX                = 6,
    BLACK_MARKET                = 7,
    BOAT                        = 8,
    BORDERGUARD                 = 9,
    KEYMASTER                   = 10,
    BUOY                        = 11,
    CAMPFIRE                    = 12,
    CARTOGRAPHER                = 13,
    SWAN_POND                   = 14,
    COVER_OF_DARKNESS           = 15,
    CREATURE_BANK               = 16,
    CREATURE_GENERATOR1         = 17,
    CREATURE_GENERATOR2         = 18,
    CREATURE_GENERATOR3         = 19,
    CREATURE_GENERATOR4         = 20,
    CURSED_GROUND1              = 21,
    CORPSE                      = 22,
    MARLETTO_TOWER              = 23,
    DERELICT_SHIP               = 24,
    DRAGON_UTOPIA               = 25,
    EVENT                       = 26,
    EYE_OF_MAGI                 = 27,
    FAERIE_RING                 = 28,
    FLOTSAM                     = 29,
    FOUNTAIN_OF_FORTUNE         = 30,
    FOUNTAIN_OF_YOUTH           = 31,
    GARDEN_OF_REVELATION        = 32,
    GARRISON                    = 33,
    HERO                        = 34,
    HILL_FORT                   = 35,
    GRAIL                       = 36,
    HUT_OF_MAGI                 = 37,
    IDOL_OF_FORTUNE             = 38,
    LEAN_TO                     = 39,
    LIBRARY_OF_ENLIGHTENMENT    = 41,
    LIGHTHOUSE                  = 42,
    MONOLITH_ONE_WAY_ENTRANCE   = 43,
    MONOLITH_ONE_WAY_EXIT       = 44,
    MONOLITH_TWO_WAY            = 45,
    MAGIC_PLAINS1               = 46,
    SCHOOL_OF_MAGIC             = 47,
    MAGIC_SPRING                = 48,
    MAGIC_WELL                  = 49,
    MERCENARY_CAMP              = 51,
    MERMAID                     = 52,
    MINE                        = 53,
    MONSTER                     = 54,
    MYSTICAL_GARDEN             = 55,
    OASIS                       = 56,
    OBELISK                     = 57,
    REDWOOD_OBSERVATORY         = 58,
    OCEAN_BOTTLE                = 59,
    PILLAR_OF_FIRE              = 60,
    STAR_AXIS                   = 61,
    PRISON                      = 62,
    PYRAMID                     = 63, //subtype 0
    WOG_OBJECT                  = 63, //subtype > 0
    RALLY_FLAG                  = 64,
    RANDOM_ART                  = 65,
    RANDOM_TREASURE_ART         = 66,
    RANDOM_MINOR_ART            = 67,
    RANDOM_MAJOR_ART            = 68,
    RANDOM_RELIC_ART            = 69,
    RANDOM_HERO                 = 70,
    RANDOM_MONSTER              = 71,
    RANDOM_MONSTER_L1           = 72,
    RANDOM_MONSTER_L2           = 73,
    RANDOM_MONSTER_L3           = 74,
    RANDOM_MONSTER_L4           = 75,
    RANDOM_RESOURCE             = 76,
    RANDOM_TOWN                 = 77,
    REFUGEE_CAMP                = 78,
    RESOURCE                    = 79,
    SANCTUARY                   = 80,
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
    SIRENS                      = 92,
    SPELL_SCROLL                = 93,
    STABLES                     = 94,
    TAVERN                      = 95,
    TEMPLE                      = 96,
    DEN_OF_THIEVES              = 97,
    TOWN                        = 98,
    TRADING_POST                = 99,
    LEARNING_STONE              = 100,
    TREASURE_CHEST              = 101,
    TREE_OF_KNOWLEDGE           = 102,
    SUBTERRANEAN_GATE           = 103,
    UNIVERSITY                  = 104,
    WAGON                       = 105,
    WAR_MACHINE_FACTORY         = 106,
    SCHOOL_OF_WAR               = 107,
    WARRIORS_TOMB               = 108,
    WATER_WHEEL                 = 109,
    WATERING_HOLE               = 110,
    WHIRLPOOL                   = 111,
    WINDMILL                    = 112,
    WITCH_HUT                   = 113,
    HOLE                        = 124,
    RANDOM_MONSTER_L5           = 162,
    RANDOM_MONSTER_L6           = 163,
    RANDOM_MONSTER_L7           = 164,
    BORDER_GATE                 = 212,
    FREELANCERS_GUILD           = 213,
    HERO_PLACEHOLDER            = 214,
    QUEST_GUARD                 = 215,
    RANDOM_DWELLING             = 216,
    RANDOM_DWELLING_LVL         = 217, //subtype = creature level
    RANDOM_DWELLING_FACTION     = 218, //subtype = faction
    GARRISON2                   = 219,
    ABANDONED_MINE              = 220,
    TRADING_POST_SNOW           = 221,
    CLOVER_FIELD                = 222,
    CURSED_GROUND2              = 223,
    EVIL_FOG                    = 224,
    FAVORABLE_WINDS             = 225,
    FIERY_FIELDS                = 226,
    HOLY_GROUNDS                = 227,
    LUCID_POOLS                 = 228,
    MAGIC_CLOUDS                = 229,
    MAGIC_PLAINS2               = 230,
    ROCKLANDS                   = 231,
};

struct IMapObject {
    virtual ~IMapObject() = default;

    virtual void ReadInternal(ByteOrderDataStreamReader& stream)        = 0;
    virtual void WriteInternal(ByteOrderDataStreamWriter& stream) const = 0;
    virtual void ToJson(PropertyTree& data) const                       = 0;
    virtual void FromJson(const PropertyTree& data)                     = 0;

    static std::unique_ptr<IMapObject> Create(MapObjectType type, MapFormat format);
};

struct StackBasicDescriptor {
    uint16_t m_id    = 0;
    uint16_t m_count = 0;
};

struct StackSet {
    MapFormat m_format = MapFormat::Invalid;

    std::vector<StackBasicDescriptor> m_stacks;

    StackSet(MapFormat format)
        : m_format(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream)
    {
        auto lock = stream.SetContainerSizeBytesGuarded(1);
        m_stacks.resize(stream.readSize());

        for (auto& stack : m_stacks) {
            if (m_format > MapFormat::ROE)
                stream >> stack.m_id;
            else
                stack.m_id = stream.ReadScalar<uint8_t>();
            stream >> stack.m_count;
        }
    }
    void WriteInternal(ByteOrderDataStreamWriter& stream) const
    {
        auto lock = stream.SetContainerSizeBytesGuarded(1);
        stream.writeSize(m_stacks.size());
        for (auto& stack : m_stacks) {
            if (m_format > MapFormat::ROE)
                stream << stack.m_id;
            else
                stream << static_cast<uint8_t>(stack.m_id);
            stream << stack.m_count;
        }
    }
};

struct StackSetFixed {
    MapFormat m_format = MapFormat::Invalid;

    std::vector<StackBasicDescriptor> m_stacks;

    StackSetFixed(MapFormat format)
        : m_format(format)
        , m_stacks(GameConstants(m_format).STACK_SIZE)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream)
    {
        for (auto& stack : m_stacks) {
            if (m_format > MapFormat::ROE)
                stream >> stack.m_id;
            else
                stack.m_id = stream.ReadScalar<uint8_t>();
            stream >> stack.m_count;
        }
    }
    void WriteInternal(ByteOrderDataStreamWriter& stream) const
    {
        for (auto& stack : m_stacks) {
            if (m_format > MapFormat::ROE)
                stream << stack.m_id;
            else
                stream << static_cast<uint8_t>(stack.m_id);
            stream << stack.m_count;
        }
    }
};

struct ResourceSet {
    ResourceSet()
        : m_resourceAmount(7)
    {}

    std::vector<uint32_t> m_resourceAmount;

    void ReadInternal(ByteOrderDataStreamReader& stream)
    {
        for (auto& res : m_resourceAmount)
            stream >> res;
    }
    void WriteInternal(ByteOrderDataStreamWriter& stream) const
    {
        for (auto& res : m_resourceAmount)
            stream << res;
    }
};

struct PrimarySkillSet {
    PrimarySkillSet()
        : m_prim(4)
    {}

    std::vector<uint8_t> m_prim;

    void ReadInternal(ByteOrderDataStreamReader& stream)
    {
        for (auto& res : m_prim)
            stream >> res;
    }
    void WriteInternal(ByteOrderDataStreamWriter& stream) const
    {
        for (auto& res : m_prim)
            stream << res;
    }
};

struct HeroArtSet {
    MapFormat m_format  = MapFormat::Invalid;
    bool      m_hasArts = false;

    std::vector<uint16_t> m_mainSlots;
    uint16_t              m_cata  = 0;
    uint16_t              m_book  = 0;
    uint16_t              m_misc5 = 0;
    std::vector<uint16_t> m_bagSlots;

    HeroArtSet(MapFormat format)
        : m_format(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct MapGuards {
    bool          m_hasGuards = false;
    StackSetFixed m_creatures;

    MapGuards(MapFormat format)
        : m_creatures(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct MapMessage {
    bool        m_hasMessage = false;
    std::string m_message;
    MapGuards   m_guards;

    MapMessage(MapFormat format)
        : m_guards(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct MapObjectAbstract : public IMapObject {
    MapFormat m_format = MapFormat::Invalid;

    MapObjectAbstract(MapFormat format)
        : m_format(format)
    {}
};

struct MapObjectSimple : public MapObjectAbstract {
    using MapObjectAbstract::MapObjectAbstract;

    void ReadInternal(ByteOrderDataStreamReader& stream) override {}
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override {}
    void ToJson(PropertyTree& data) const override {}
    void FromJson(const PropertyTree& data) override {}
};

struct MapObjectWithOwner : public MapObjectAbstract {
    using MapObjectAbstract::MapObjectAbstract;

    uint8_t m_owner = 0;

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapHeroSkill {
    uint8_t m_id    = 0;
    uint8_t m_level = 0; //  (1 - basic, 2 - adv., 3 - expert)

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
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

    bool          m_hasGarison = false;
    StackSetFixed m_garison;

    uint8_t    m_formation = 0;
    HeroArtSet m_artSet;

    uint8_t     m_patrolRadius       = 0xff;
    bool        m_hasCustomBiography = false;
    std::string m_bio;
    uint8_t     m_sex = 0xff;

    bool m_hasCustomSpells     = false;
    bool m_hasCustomPrimSkills = false;

    MapHero(MapFormat format)
        : MapObjectAbstract(format)
        , m_garison(format)
        , m_artSet(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapTown : public MapObjectAbstract {
    uint32_t m_questIdentifier = 0;
    uint8_t  m_playerOwner     = 0;

    bool        m_hasName = false;
    std::string m_name;

    bool          m_hasGarison = false;
    StackSetFixed m_garison;

    uint8_t m_formation = 0;

    bool m_hasCustomBuildings = false;
    // todo Buildings
    bool m_hasFort = false;

    std::vector<uint8_t> m_obligatorySpells;
    std::vector<uint8_t> m_possibleSpells;

    uint32_t m_events    = 0;
    uint8_t  m_alignment = 0xff;

    MapTown(MapFormat format)
        : MapObjectAbstract(format)
        , m_garison(format)
    {}

    void prepareArrays();

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapMonster : public MapObjectAbstract {
    uint32_t m_questIdentifier = 0;

    uint16_t m_count      = 0;
    uint8_t  m_joinAppeal = 0;

    bool        m_hasMessage = false;
    std::string m_message;
    ResourceSet m_resourceSet;
    uint16_t    m_artID          = 0;
    bool        m_neverFlees     = false;
    bool        m_notGrowingTeam = false;

    using MapObjectAbstract::MapObjectAbstract;

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapResource : public MapObjectAbstract {
    MapMessage m_message;
    uint32_t   m_amount = 0;

    MapResource(MapFormat format)
        : MapObjectAbstract(format)
        , m_message(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapArtifact : public MapObjectAbstract {
    MapMessage m_message;
    uint32_t   m_spellId = 0;
    bool       m_isSpell = false;

    MapArtifact(MapFormat format)
        : MapObjectAbstract(format)
        , m_message(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
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
        PLAYER        = 9
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

    uint32_t              m_134val = 0;
    std::vector<uint8_t>  m_2stats;
    std::vector<uint16_t> m_5arts;      //artifacts id
    StackSet              m_6creatures; //pair[cre id, cre count], CreatureSet info irrelevant
    std::vector<uint32_t> m_7resources; //TODO: use resourceset?
    uint8_t               m_89val = 0;

    std::string m_firstVisitText;
    std::string m_nextVisitText;
    std::string m_completedText;

    MapQuest(MapFormat format)
        : MapObjectAbstract(format)
        , m_6creatures(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapSeerHut : public MapObjectAbstract {
    MapQuest m_quest;

    enum class RewardType
    {
        NOTHING,
        EXPERIENCE,
        MANA_POINTS,
        MORALE_BONUS,
        LUCK_BONUS,
        RESOURCES,
        PRIMARY_SKILL,
        SECONDARY_SKILL,
        ARTIFACT,
        SPELL,
        CREATURE
    };

    RewardType m_reward = RewardType::NOTHING;
    uint32_t   m_rID    = 0; //reward ID
    uint32_t   m_rVal   = 0; //reward value

    using Mission  = MapQuest::Mission;
    using Progress = MapQuest::Progress;

    MapSeerHut(MapFormat format)
        : MapObjectAbstract(format)
        , m_quest(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapShrine : public MapObjectAbstract {
    uint8_t m_spell = 0;

    using MapObjectAbstract::MapObjectAbstract;

    void ReadInternal(ByteOrderDataStreamReader& stream) override
    {
        stream >> m_spell;
        stream.zeroPadding(3);
    }
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override
    {
        stream << m_spell;
        stream.zeroPadding(3);
    }
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapScholar : public MapObjectAbstract {
    uint8_t m_bonusType = 0;
    uint8_t m_bonusId   = 0;

    using MapObjectAbstract::MapObjectAbstract;

    void ReadInternal(ByteOrderDataStreamReader& stream) override
    {
        stream >> m_bonusType >> m_bonusId;
        stream.zeroPadding(6);
    }
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override
    {
        stream << m_bonusType << m_bonusId;
        stream.zeroPadding(6);
    }
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapWitchHut : public MapObjectAbstract {
    std::vector<uint8_t> m_allowedSkills;

    MapWitchHut(MapFormat format);

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapReward {
    MapFormat m_format = MapFormat::Invalid;

    uint32_t m_gainedExp  = 0;
    uint32_t m_manaDiff   = 0;
    int8_t   m_moraleDiff = 0;
    int8_t   m_luckDiff   = 0;

    ResourceSet               m_resourceSet;
    PrimarySkillSet           m_primSkillSet;
    std::vector<MapHeroSkill> m_secSkills;
    std::vector<uint16_t>     m_artifacts;
    std::vector<uint8_t>      m_spells;
    StackSet                  m_creatures;

    MapReward(MapFormat format)
        : m_format(format)
        , m_creatures(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream);
    void WriteInternal(ByteOrderDataStreamWriter& stream) const;
};

struct MapPandora : public MapObjectAbstract {
    MapMessage m_message;
    MapReward  m_reward;

    MapPandora(MapFormat format)
        : MapObjectAbstract(format)
        , m_message(format)
        , m_reward(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapGarison : public MapObjectAbstract {
    uint8_t       m_owner = 0;
    StackSetFixed m_garison;
    bool          m_removableUnits = true;

    MapGarison(MapFormat format)
        : MapObjectAbstract(format)
        , m_garison(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapSignBottle : public MapObjectAbstract {
    std::string m_message;

    using MapObjectAbstract::MapObjectAbstract;

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapEvent : public MapObjectAbstract {
    MapMessage m_message;
    MapReward  m_reward;

    uint8_t m_availableFor     = 0;
    uint8_t m_computerActivate = 0;
    uint8_t m_removeAfterVisit = 0;

    MapEvent(MapFormat format)
        : MapObjectAbstract(format)
        , m_message(format)
        , m_reward(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapDwelling : public MapObjectAbstract {
    MapObjectType m_objectType;

    uint8_t  m_owner       = 0;
    uint32_t m_factionId   = 0;
    uint16_t m_factionMask = 0;
    uint8_t  m_minLevel    = 0;
    uint8_t  m_maxLevel    = 0;

    MapDwelling(MapFormat format, MapObjectType objectType)
        : MapObjectAbstract(format)
        , m_objectType(objectType)
    {
    }

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

struct MapQuestGuard : public MapObjectAbstract {
    MapQuest m_quest;

    MapQuestGuard(MapFormat format)
        : MapObjectAbstract(format)
        , m_quest(format)
    {}

    void ReadInternal(ByteOrderDataStreamReader& stream) override;
    void WriteInternal(ByteOrderDataStreamWriter& stream) const override;
    void ToJson(PropertyTree& data) const override;
    void FromJson(const PropertyTree& data) override;
};

}
