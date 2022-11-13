/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MapObjects.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

namespace FreeHeroes {

namespace Core::Reflection {

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<MapQuest::Mission> = EnumTraits::make(
    MapQuest::Mission::NONE,
    "NONE"          , MapQuest::Mission::NONE,
    "LEVEL"         , MapQuest::Mission::LEVEL,
    "PRIMARY_STAT"  , MapQuest::Mission::PRIMARY_STAT,
    "KILL_HERO"     , MapQuest::Mission::KILL_HERO,
    "KILL_CREATURE" , MapQuest::Mission::KILL_CREATURE,
    "ART"           , MapQuest::Mission::ART,
    "ARMY"          , MapQuest::Mission::ARMY,
    "RESOURCES"     , MapQuest::Mission::RESOURCES,
    "HERO"          , MapQuest::Mission::HERO,
    "PLAYER"        , MapQuest::Mission::PLAYER
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<MapQuest::Progress> = EnumTraits::make(
    MapQuest::Progress::NOT_ACTIVE,
    "NOT_ACTIVE"   , MapQuest::Progress::NOT_ACTIVE,
    "IN_PROGRESS"  , MapQuest::Progress::IN_PROGRESS,
    "COMPLETE"     , MapQuest::Progress::COMPLETE
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<MapSeerHut::RewardType> = EnumTraits::make(
    MapSeerHut::RewardType::NOTHING,
    "NOTHING"         , MapSeerHut::RewardType::NOTHING,
    "EXPERIENCE"      , MapSeerHut::RewardType::EXPERIENCE,
    "MANA_POINTS"     , MapSeerHut::RewardType::MANA_POINTS,
    "MORALE_BONUS"    , MapSeerHut::RewardType::MORALE_BONUS,
    "LUCK_BONUS"      , MapSeerHut::RewardType::LUCK_BONUS,
    "RESOURCES"       , MapSeerHut::RewardType::RESOURCES,
    "PRIMARY_SKILL"   , MapSeerHut::RewardType::PRIMARY_SKILL,
    "SECONDARY_SKILL" , MapSeerHut::RewardType::SECONDARY_SKILL,
    "ARTIFACT"        , MapSeerHut::RewardType::ARTIFACT,
    "SPELL"           , MapSeerHut::RewardType::SPELL,
    "CREATURE"        , MapSeerHut::RewardType::CREATURE
    );

// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<StackBasicDescriptor>{
    Field("id", &StackBasicDescriptor::m_id),
    Field("count", &StackBasicDescriptor::m_count),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<StackSet>{
    Field("stacks", &StackSet::m_stacks),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<StackSetFixed>{
    Field("stacks", &StackSetFixed::m_stacks),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapHeroSkill>{
    Field("id", &MapHeroSkill::m_id),
    Field("level", &MapHeroSkill::m_level),
};

// @todo: deduplicate
template<>
inline constexpr const std::tuple MetaInfo::s_fields<ResourceSet>{
    Field("resourceAmount", &ResourceSet::m_resourceAmount),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<PrimarySkillSet>{
    Field("prim", &PrimarySkillSet::m_prim),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<HeroArtSet>{
    Field("hasArts", &HeroArtSet::m_hasArts),
    Field("main", &HeroArtSet::m_mainSlots),
    Field("cata", &HeroArtSet::m_cata),
    Field("book", &HeroArtSet::m_book),
    Field("misc5", &HeroArtSet::m_misc5),
    Field("bag", &HeroArtSet::m_bagSlots),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapHero>{
    Field("questIdentifier", &MapHero::m_questIdentifier),
    Field("playerOwner", &MapHero::m_playerOwner),
    Field("subID", &MapHero::m_subID),

    Field("hasName", &MapHero::m_hasName),
    Field("name", &MapHero::m_name),
    Field("hasExp", &MapHero::m_hasExp),
    Field("exp", &MapHero::m_exp),
    Field("hasPortrait", &MapHero::m_hasPortrait),
    Field("portrait", &MapHero::m_portrait),
    Field("hasSecSkills", &MapHero::m_hasSecSkills),
    Field("secSkills", &MapHero::m_secSkills),
    Field("hasGarison", &MapHero::m_hasGarison),
    Field("garison", &MapHero::m_garison),
    Field("formation", &MapHero::m_formation),
    Field("arts", &MapHero::m_artSet),
    Field("patrolRadius", &MapHero::m_patrolRadius),
    Field("hasCustomBiography", &MapHero::m_hasCustomBiography),
    Field("bio", &MapHero::m_bio),
    Field("sex", &MapHero::m_sex),
    Field("hasCustomSpells", &MapHero::m_hasCustomSpells),
    Field("hasCustomPrimSkills", &MapHero::m_hasCustomPrimSkills),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTown>{
    Field("questIdentifier", &MapTown::m_questIdentifier),
    Field("playerOwner", &MapTown::m_playerOwner),
    Field("hasName", &MapTown::m_hasName),
    Field("name", &MapTown::m_name),
    Field("hasGarison", &MapTown::m_hasGarison),
    Field("garison", &MapTown::m_garison),
    Field("formation", &MapTown::m_formation),
    Field("hasCustomBuildings", &MapTown::m_hasCustomBuildings),
    Field("hasFort", &MapTown::m_hasFort),
    Field("obligatorySpells", &MapTown::m_obligatorySpells),
    Field("possibleSpells", &MapTown::m_possibleSpells),
    Field("events", &MapTown::m_events),
    Field("alignment", &MapTown::m_alignment),

};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapMonster>{
    Field("questIdentifier", &MapMonster::m_questIdentifier),
    Field("count", &MapMonster::m_count),
    Field("joinAppeal", &MapMonster::m_joinAppeal),
    Field("hasMessage", &MapMonster::m_hasMessage),
    Field("message", &MapMonster::m_message),
    Field("resourceSet", &MapMonster::m_resourceSet),
    Field("artID", &MapMonster::m_artID),

    Field("neverFlees", &MapMonster::m_neverFlees),
    Field("notGrowingTeam", &MapMonster::m_notGrowingTeam),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapQuest>{
    Field("missionType", &MapQuest::m_missionType),
    Field("progress", &MapQuest::m_progress),
    Field("lastDay", &MapQuest::m_lastDay),
    Field("134val", &MapQuest::m_134val),
    Field("2stats", &MapQuest::m_2stats),
    Field("5arts", &MapQuest::m_5arts),
    Field("6creatures", &MapQuest::m_6creatures),
    Field("7resources", &MapQuest::m_7resources),
    Field("89val", &MapQuest::m_89val),
    Field("firstVisitText", &MapQuest::m_firstVisitText),
    Field("nextVisitText", &MapQuest::m_nextVisitText),
    Field("completedText", &MapQuest::m_completedText),

};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapSeerHut>{
    Field("quest", &MapSeerHut::m_quest),
    Field("reward", &MapSeerHut::m_reward),
    Field("rID", &MapSeerHut::m_rID),
    Field("rVal", &MapSeerHut::m_rVal),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapReward>{
    Field("gainedExp", &MapReward::m_gainedExp),
    Field("manaDiff", &MapReward::m_manaDiff),
    Field("moraleDiff", &MapReward::m_moraleDiff),
    Field("luckDiff", &MapReward::m_luckDiff),
    Field("resourceSet", &MapReward::m_resourceSet),
    Field("primSkillSet", &MapReward::m_primSkillSet),
    Field("secSkills", &MapReward::m_secSkills),
    Field("artifacts", &MapReward::m_artifacts),
    Field("spells", &MapReward::m_spells),
    Field("creatures", &MapReward::m_creatures),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapPandora>{
    Field("message", &MapPandora::m_message),
    Field("reward", &MapPandora::m_reward),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapGuards>{
    Field("hasGuards", &MapGuards::m_hasGuards),
    Field("creatures", &MapGuards::m_creatures),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapMessage>{
    Field("hasMessage", &MapMessage::m_hasMessage),
    Field("message", &MapMessage::m_message),
    Field("guards", &MapMessage::m_guards),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapArtifact>{
    Field("message", &MapArtifact::m_message),
    Field("spellId", &MapArtifact::m_spellId),
    Field("isSpell", &MapArtifact::m_isSpell),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapResource>{
    Field("message", &MapResource::m_message),
    Field("amount", &MapResource::m_amount),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapGarison>{
    Field("owner", &MapGarison::m_owner),
    Field("garison", &MapGarison::m_garison),
    Field("removableUnits", &MapGarison::m_removableUnits),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapEvent>{
    Field("message", &MapEvent::m_message),
    Field("reward", &MapEvent::m_reward),

    Field("availableFor", &MapEvent::m_availableFor),
    Field("computerActivate", &MapEvent::m_computerActivate),
    Field("removeAfterVisit", &MapEvent::m_removeAfterVisit),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapDwelling>{
    Field("owner", &MapDwelling::m_owner),
    Field("factionId", &MapDwelling::m_factionId),
    Field("factionMask", &MapDwelling::m_factionMask),
    Field("minLevel", &MapDwelling::m_minLevel),
    Field("maxLevel", &MapDwelling::m_maxLevel),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapQuestGuard>{
    Field("quest", &MapQuestGuard::m_quest),
};

}

GameConstants::GameConstants(MapFormat format)
{
    F_NUMBER = 9;
    if (format == MapFormat::ROE)
        F_NUMBER = 8;

    ARTIFACTS_QUANTITY = 141; //SOD
    if (format == MapFormat::AB)
        ARTIFACTS_QUANTITY = 129;
    if (format == MapFormat::ROE)
        ARTIFACTS_QUANTITY = 128;
    if (format >= MapFormat::HOTA1)
        ARTIFACTS_QUANTITY = 165;

    HEROES_QUANTITY = 156;
    if (format == MapFormat::ROE)
        HEROES_QUANTITY = 128;
    if (format >= MapFormat::HOTA1)
        ARTIFACTS_QUANTITY = 179;

    SPELLS_QUANTITY    = 81;
    ABILITIES_QUANTITY = 11;

    CREATURES_COUNT = 150;
    if (format == MapFormat::ROE)
        CREATURES_COUNT = 115;
    if (format >= MapFormat::HOTA1)
        CREATURES_COUNT = 171;

    SKILL_QUANTITY    = 28;
    PRIMARY_SKILLS    = 4;
    TERRAIN_TYPES     = 10;
    RESOURCE_QUANTITY = 8;
    HEROES_PER_TYPE   = 8; //amount of heroes of each type
    PLAYER_LIMIT_I    = 8;
    STACK_SIZE        = 7;
}

std::unique_ptr<IMapObject> IMapObject::Create(MapObjectType type, MapFormat format)
{
    switch (type) {
        case MapObjectType::EVENT:
        {
            return std::make_unique<MapEvent>(format);
            break;
        }
        case MapObjectType::HERO:
        case MapObjectType::RANDOM_HERO:
        case MapObjectType::PRISON:
        {
            return std::make_unique<MapHero>(format);
            break;
        }
        case MapObjectType::MONSTER: //Monster
        case MapObjectType::RANDOM_MONSTER:
        case MapObjectType::RANDOM_MONSTER_L1:
        case MapObjectType::RANDOM_MONSTER_L2:
        case MapObjectType::RANDOM_MONSTER_L3:
        case MapObjectType::RANDOM_MONSTER_L4:
        case MapObjectType::RANDOM_MONSTER_L5:
        case MapObjectType::RANDOM_MONSTER_L6:
        case MapObjectType::RANDOM_MONSTER_L7:
        {
            return std::make_unique<MapMonster>(format);

            break;
        }
        case MapObjectType::OCEAN_BOTTLE:
        case MapObjectType::SIGN:
        {
            return std::make_unique<MapSignBottle>(format);
            break;
        }
        case MapObjectType::SEER_HUT:
        {
            return std::make_unique<MapSeerHut>(format);
            // nobj = readSeerHut();
            break;
        }
        case MapObjectType::WITCH_HUT:
        {
            return std::make_unique<MapWitchHut>(format);
            break;
        }
        case MapObjectType::SCHOLAR:
        {
            return std::make_unique<MapScholar>(format);
            break;
        }
        case MapObjectType::GARRISON:
        case MapObjectType::GARRISON2:
        {
            return std::make_unique<MapGarison>(format);
            break;
        }
        case MapObjectType::ARTIFACT:
        case MapObjectType::RANDOM_ART:
        case MapObjectType::RANDOM_TREASURE_ART:
        case MapObjectType::RANDOM_MINOR_ART:
        case MapObjectType::RANDOM_MAJOR_ART:
        case MapObjectType::RANDOM_RELIC_ART:
        case MapObjectType::SPELL_SCROLL:
        {
            auto res       = std::make_unique<MapArtifact>(format);
            res->m_isSpell = type == MapObjectType::SPELL_SCROLL;
            return res;
            break;
        }
        case MapObjectType::RANDOM_RESOURCE:
        case MapObjectType::RESOURCE:
        {
            return std::make_unique<MapResource>(format);
            break;
        }
        case MapObjectType::RANDOM_TOWN:
        case MapObjectType::TOWN:
        {
            return std::make_unique<MapTown>(format);
            break;
        }
        case MapObjectType::MINE:
        case MapObjectType::ABANDONED_MINE:
        {
            return std::make_unique<MapObjectWithOwner>(format);
            break;
        }
        case MapObjectType::CREATURE_GENERATOR1:
        case MapObjectType::CREATURE_GENERATOR2:
        case MapObjectType::CREATURE_GENERATOR3:
        case MapObjectType::CREATURE_GENERATOR4:
        {
            return std::make_unique<MapObjectWithOwner>(format);
            break;
        }
        case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
        case MapObjectType::SHRINE_OF_MAGIC_GESTURE:
        case MapObjectType::SHRINE_OF_MAGIC_THOUGHT:
        {
            return std::make_unique<MapShrine>(format);
            break;
        }
        case MapObjectType::PANDORAS_BOX:
        {
            return std::make_unique<MapPandora>(format);
            break;
        }
        case MapObjectType::GRAIL:
        {
            //map->grailPos = objPos;
            //map->grailRadius = stream.ReadScalar<uint32_t>();
            break;
        }
        case MapObjectType::QUEST_GUARD:
        {
            return std::make_unique<MapQuestGuard>(format);
            break;
        }
        case MapObjectType::RANDOM_DWELLING:         //same as castle + level range  216
        case MapObjectType::RANDOM_DWELLING_LVL:     //same as castle, fixed level   217
        case MapObjectType::RANDOM_DWELLING_FACTION: //level range, fixed faction    218
        {
            return std::make_unique<MapDwelling>(format, type);
            break;
        }

        case MapObjectType::SHIPYARD:
        {
            return std::make_unique<MapObjectWithOwner>(format);
            break;
        }
        case MapObjectType::HERO_PLACEHOLDER: //hero placeholder
        {
            /*auto  hp = new CGHeroPlaceholder();
            nobj = hp;
            
            hp->setOwner(PlayerColor(stream.ReadScalar<uint8_t>()));
            
            int htid = stream.ReadScalar<uint8_t>(); //hero type id
            nobj->subID = htid;
            
            if(htid == 0xff)
            {
                hp->power = stream.ReadScalar<uint8_t>();
                logGlobal->info("Hero placeholder: by power at %s", objPos.toString());
            }
            else
            {
                logGlobal->info("Hero placeholder: %s at %s", VLC->heroh->heroes[htid]->name, objPos.toString());
                hp->power = 0;
            }*/

            break;
        }
        case MapObjectType::LIGHTHOUSE: //Lighthouse
        {
            return std::make_unique<MapObjectWithOwner>(format);
            break;
        }
        default: //any other object
        {
            return std::make_unique<MapObjectSimple>(format);
            break;
        }
    }
    return nullptr;
}

void MapHero::ReadInternal(ByteOrderDataStreamReader& stream)
{
    if (m_format > MapFormat::ROE) {
        stream >> m_questIdentifier;
    }
    stream >> m_playerOwner >> m_subID;

    stream >> m_hasName;
    if (m_hasName)
        stream >> m_name;

    if (m_format > MapFormat::AB) {
        stream >> m_hasExp;
        if (m_hasExp)
            stream >> m_exp;

    } else {
        stream >> m_exp;
        //0 means "not set" in <=AB maps
        if (!m_exp)
            m_exp = -1;
    }

    stream >> m_hasPortrait;
    if (m_hasPortrait)
        stream >> m_portrait;

    stream >> m_hasSecSkills;
    if (m_hasSecSkills)
        stream >> m_secSkills;

    stream >> m_hasGarison;
    if (m_hasGarison)
        stream >> m_garison;

    stream >> m_formation;

    stream >> m_artSet;

    stream >> m_patrolRadius;

    if (m_format > MapFormat::ROE) {
        stream >> m_hasCustomBiography;
        if (m_hasCustomBiography)
            stream >> m_bio;

        stream >> m_sex;
    }

    // Spells
    if (m_format > MapFormat::AB) {
        stream >> m_hasCustomSpells;
        if (m_hasCustomSpells)
            throw std::runtime_error("CustomSpells on hero is unsupported");

    } else if (m_format == MapFormat::AB) {
        //we can read one spell
        auto buff = stream.ReadScalar<uint8_t>();
        if (buff != 254) {
            m_hasCustomSpells = true;
            //            nhi->spells.insert(SpellID::PRESET); //placeholder "preset spells"
            //            if (buff < 254)                      //255 means no spells
            //            {
            //                nhi->spells.insert(SpellID(buff));
            //            }
        }
    }

    if (m_format > MapFormat::AB) {
        stream >> m_hasCustomPrimSkills;
        if (m_hasCustomPrimSkills)
            throw std::runtime_error("CustomPrimSkills on hero is unsupported");
    }
    stream.zeroPadding(16);
}

void MapHero::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    if (m_format > MapFormat::ROE) {
        stream << m_questIdentifier;
    }
    stream << m_playerOwner << m_subID;

    stream << m_hasName;
    if (m_hasName)
        stream << m_name;

    if (m_format > MapFormat::AB) {
        stream << m_hasExp;
        if (m_hasExp)
            stream << m_exp;

    } else {
        stream << m_exp;
    }

    stream << m_hasPortrait;
    if (m_hasPortrait)
        stream << m_portrait;

    stream << m_hasSecSkills;
    if (m_hasSecSkills)
        stream << m_secSkills;

    stream << m_hasGarison;
    if (m_hasGarison)
        stream << m_garison;

    stream << m_formation;

    stream << m_artSet;

    stream << m_patrolRadius;

    if (m_format > MapFormat::ROE) {
        stream << m_hasCustomBiography;
        if (m_hasCustomBiography)
            stream << m_bio;

        stream << m_sex;
    }

    // Spells
    if (m_format > MapFormat::AB) {
        stream << m_hasCustomSpells;

    } else if (m_format == MapFormat::AB) {
        uint8_t spell = 254;
        if (m_hasCustomSpells) {
            spell = 255; // todo:
        }
        stream << spell;
    }

    if (m_format > MapFormat::AB) {
        stream << m_hasCustomPrimSkills;
    }
    stream.zeroPadding(16);
}

void MapHero::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapHero::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapTown::prepareArrays()
{
    if (m_format > MapFormat::ROE)
        m_obligatorySpells.resize(9);
    m_possibleSpells.resize(9);
}

void MapTown::ReadInternal(ByteOrderDataStreamReader& stream)
{
    if (m_format > MapFormat::ROE) {
        stream >> m_questIdentifier;
    }
    stream >> m_playerOwner;

    stream >> m_hasName;
    if (m_hasName)
        stream >> m_name;

    stream >> m_hasGarison;
    if (m_hasGarison)
        stream >> m_garison;

    stream >> m_formation;

    stream >> m_hasCustomBuildings;
    if (m_hasCustomBuildings)
        throw std::runtime_error("Buildings in town is unsupported");
    else {
        stream >> m_hasFort;
    }
    prepareArrays();
    for (auto& byte : m_obligatorySpells)
        stream >> byte;

    for (auto& byte : m_possibleSpells)
        stream >> byte;

    stream >> m_events;
    if (m_events)
        throw std::runtime_error("Events in town is unsupported");

    if (m_format > MapFormat::AB) {
        stream >> m_alignment;
    }
    stream.zeroPadding(3);
}

void MapTown::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    if (m_format > MapFormat::ROE) {
        stream << m_questIdentifier;
    }
    stream << m_playerOwner;

    stream << m_hasName;
    if (m_hasName)
        stream << m_name;

    stream << m_hasGarison;
    if (m_hasGarison)
        stream << m_garison;

    stream << m_formation;

    stream << m_hasCustomBuildings;
    if (m_hasCustomBuildings) {
    } else {
        stream << m_hasFort;
    }

    if (m_format > MapFormat::ROE) {
        for (auto& byte : m_obligatorySpells)
            stream << byte;
    }
    for (auto& byte : m_possibleSpells)
        stream << byte;

    stream << m_events;

    if (m_format > MapFormat::AB) {
        stream << m_alignment;
    }
    stream.zeroPadding(3);
}

void MapTown::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapTown::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapHeroSkill::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_id >> m_level;
}

void MapHeroSkill::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_id << m_level;
}

void MapMonster::ReadInternal(ByteOrderDataStreamReader& stream)
{
    if (m_format > MapFormat::ROE) {
        stream >> m_questIdentifier;
    }
    stream >> m_count;
    stream >> m_joinAppeal;
    stream >> m_hasMessage;

    if (m_hasMessage) {
        stream >> m_message;
        stream >> m_resourceSet;

        if (m_format == MapFormat::ROE)
            m_artID = stream.ReadScalar<uint8_t>();
        else
            stream >> m_artID;
    }

    stream >> m_neverFlees >> m_notGrowingTeam;
    stream.zeroPadding(2);
}

void MapMonster::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    if (m_format > MapFormat::ROE) {
        stream << m_questIdentifier;
    }
    stream << m_count;
    stream << m_joinAppeal;
    stream << m_hasMessage;
    if (m_hasMessage) {
        stream << m_message;
        stream << m_resourceSet;

        if (m_format == MapFormat::ROE)
            stream << static_cast<uint8_t>(m_artID);
        else
            stream << m_artID;
    }

    stream << m_neverFlees << m_notGrowingTeam;
    stream.zeroPadding(2);
}

void MapMonster::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapMonster::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapObjectWithOwner::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_owner;
    stream.zeroPadding(3);
}

void MapObjectWithOwner::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_owner;
    stream.zeroPadding(3);
}

void MapObjectWithOwner::ToJson(PropertyTree& data) const
{
    data["owner"] = PropertyTreeScalar(m_owner);
}

void MapObjectWithOwner::FromJson(const PropertyTree& data)
{
    data["owner"].getScalar().convertTo(m_owner);
}

void MapResource::ReadInternal(ByteOrderDataStreamReader& stream)
{
    m_message.ReadInternal(stream);
    stream >> m_amount;
    stream.zeroPadding(4);
}

void MapResource::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    m_message.WriteInternal(stream);
    stream << m_amount;
    stream.zeroPadding(4);
}

void MapResource::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapResource::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapGuards::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_hasGuards;
    if (!m_hasGuards)
        return;

    stream >> m_creatures;
}

void MapGuards::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_hasGuards;

    if (!m_hasGuards)
        return;

    stream << m_creatures;
}

void MapMessage::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_hasMessage;
    if (!m_hasMessage)
        return;

    stream >> m_message;
    stream >> m_guards;
    stream.zeroPadding(4);
}

void MapMessage::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_hasMessage;
    if (!m_hasMessage)
        return;

    stream << m_message;
    stream << m_guards;
    stream.zeroPadding(4);
}

void MapArtifact::ReadInternal(ByteOrderDataStreamReader& stream)
{
    m_message.ReadInternal(stream);
    if (m_isSpell)
        stream >> m_spellId;
}

void MapArtifact::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    m_message.WriteInternal(stream);
    if (m_isSpell)
        stream << m_spellId;
}

void MapArtifact::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapArtifact::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapQuest::ReadInternal(ByteOrderDataStreamReader& stream)
{
    m_missionType = static_cast<Mission>(stream.ReadScalar<uint8_t>());

    switch (m_missionType) {
        case Mission::NONE:
            return;
        case Mission::PRIMARY_STAT:
        {
            m_2stats.resize(4);
            for (int x = 0; x < 4; ++x)
                stream >> m_2stats[x];

        } break;
        case Mission::LEVEL:
        case Mission::KILL_HERO:
        case Mission::KILL_CREATURE:
        {
            stream >> m_134val;
            break;
        }
        case Mission::ART:
        {
            auto lock = stream.SetContainerSizeBytesGuarded(1);
            stream >> m_5arts;

            break;
        }
        case Mission::ARMY:
        {
            auto lock = stream.SetContainerSizeBytesGuarded(1);
            stream >> m_6creatures;
            break;
        }
        case Mission::RESOURCES:
        {
            m_7resources.resize(7);
            for (int x = 0; x < 7; ++x)
                stream >> m_7resources[x];

            break;
        }
        case Mission::HERO:
        case Mission::PLAYER:
        {
            stream >> m_89val;
            break;
        }
        default:
            throw std::runtime_error("unknown Mission value:" + std::to_string((int) m_missionType));
            break;
    }

    stream >> m_lastDay;

    stream >> m_firstVisitText >> m_nextVisitText >> m_completedText;
}

void MapQuest::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << static_cast<uint8_t>(m_missionType);

    switch (m_missionType) {
        case Mission::NONE:
            return;
        case Mission::PRIMARY_STAT:
        {
            for (int x = 0; x < 4; ++x)
                stream << m_2stats[x];

        } break;
        case Mission::LEVEL:
        case Mission::KILL_HERO:
        case Mission::KILL_CREATURE:
        {
            stream << m_134val;
            break;
        }
        case Mission::ART:
        {
            auto lock = stream.SetContainerSizeBytesGuarded(1);
            stream << m_5arts;

            break;
        }
        case Mission::ARMY:
        {
            auto lock = stream.SetContainerSizeBytesGuarded(1);
            stream << m_6creatures;
            break;
        }
        case Mission::RESOURCES:
        {
            for (int x = 0; x < 7; ++x)
                stream << m_7resources[x];

            break;
        }
        case Mission::HERO:
        case Mission::PLAYER:
        {
            stream << m_89val;
            break;
        }
        default:
            throw std::runtime_error("unknown Mission value:" + std::to_string((int) m_missionType));
            break;
    }

    stream << m_lastDay;

    stream << m_firstVisitText << m_nextVisitText << m_completedText;
}

void MapQuest::ToJson(PropertyTree& data) const
{
    assert(0);
}

void MapQuest::FromJson(const PropertyTree& data)
{
    assert(0);
}

void MapSeerHut::ReadInternal(ByteOrderDataStreamReader& stream)
{
    if (m_format > MapFormat::ROE) {
        m_quest.ReadInternal(stream);
    } else {
        //RoE
        uint8_t artID = 0;
        stream >> artID;
        if (artID != 255) {
            m_quest.m_5arts.push_back(artID);
            m_quest.m_missionType = Mission::ART;
        }
    }

    if (m_quest.m_missionType != Mission::NONE) {
        m_reward = static_cast<RewardType>(stream.ReadScalar<uint8_t>());
        switch (m_reward) {
            case RewardType::EXPERIENCE:
            case RewardType::MANA_POINTS:
            {
                m_rVal = stream.ReadScalar<uint32_t>();
                break;
            }
            case RewardType::MORALE_BONUS:
            case RewardType::LUCK_BONUS:
            {
                m_rVal = stream.ReadScalar<uint8_t>();
                break;
            }
            case RewardType::RESOURCES:
            {
                m_rID = stream.ReadScalar<uint8_t>();
                // Only the first 3 bytes are used. Skip the 4th.
                m_rVal = stream.ReadScalar<uint32_t>() & 0x00ffffff;
                break;
            }
            case RewardType::PRIMARY_SKILL:
            case RewardType::SECONDARY_SKILL:
            {
                m_rID  = stream.ReadScalar<uint8_t>();
                m_rVal = stream.ReadScalar<uint8_t>();
                break;
            }
            case RewardType::ARTIFACT:
            {
                if (m_format == MapFormat::ROE) {
                    m_rID = stream.ReadScalar<uint8_t>();
                } else {
                    m_rID = stream.ReadScalar<uint16_t>();
                }
                break;
            }
            case RewardType::SPELL:
            {
                m_rID = stream.ReadScalar<uint8_t>();
                break;
            }
            case RewardType::CREATURE:
            {
                if (m_format > MapFormat::ROE) {
                    m_rID  = stream.ReadScalar<uint16_t>();
                    m_rVal = stream.ReadScalar<uint16_t>();
                } else {
                    m_rID  = stream.ReadScalar<uint8_t>();
                    m_rVal = stream.ReadScalar<uint16_t>();
                }
                break;
            }
            case RewardType::NOTHING:
            {
                break;
            }
        }
        stream.zeroPadding(2);
    } else {
        stream.zeroPadding(3);
    }
}

void MapSeerHut::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    if (m_format > MapFormat::ROE) {
        m_quest.WriteInternal(stream);
    } else {
        uint8_t artID = 255;
        if (!m_quest.m_5arts.empty())
            artID = static_cast<uint8_t>(m_quest.m_5arts[0]);
        stream << artID;
    }

    if (m_quest.m_missionType != Mission::NONE) {
        stream << static_cast<uint8_t>(m_reward);
        switch (m_reward) {
            case RewardType::EXPERIENCE:
            case RewardType::MANA_POINTS:
            {
                stream << m_rVal;
                break;
            }
            case RewardType::MORALE_BONUS:
            case RewardType::LUCK_BONUS:
            {
                stream << static_cast<uint8_t>(m_rVal);
                break;
            }
            case RewardType::RESOURCES:
            {
                stream << static_cast<uint8_t>(m_rID);
                stream << m_rVal;
                break;
            }
            case RewardType::PRIMARY_SKILL:
            case RewardType::SECONDARY_SKILL:
            {
                stream << static_cast<uint8_t>(m_rID);
                stream << static_cast<uint8_t>(m_rVal);
                break;
            }
            case RewardType::ARTIFACT:
            {
                if (m_format > MapFormat::ROE) {
                    stream << static_cast<uint16_t>(m_rID);
                } else {
                    stream << static_cast<uint8_t>(m_rID);
                }
                break;
            }
            case RewardType::SPELL:
            {
                stream << static_cast<uint8_t>(m_rID);
                break;
            }
            case RewardType::CREATURE:
            {
                if (m_format > MapFormat::ROE)
                    stream << static_cast<uint16_t>(m_rID);
                else
                    stream << static_cast<uint8_t>(m_rID);

                stream << static_cast<uint16_t>(m_rVal);
                break;
            }
            case RewardType::NOTHING:
            {
                break;
            }
        }
        stream.zeroPadding(2);
    } else {
        stream.zeroPadding(3);
    }
}

void MapSeerHut::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapSeerHut::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapShrine::ToJson(PropertyTree& data) const
{
    data["spell"] = PropertyTreeScalar(m_spell);
}

void MapShrine::FromJson(const PropertyTree& data)
{
    data["spell"].getScalar().convertTo(m_spell);
}

void MapScholar::ToJson(PropertyTree& data) const
{
    data["bonusType"] = PropertyTreeScalar(m_bonusType);
    data["bonusId"]   = PropertyTreeScalar(m_bonusId);
}

void MapScholar::FromJson(const PropertyTree& data)
{
    data["bonusType"].getScalar().convertTo(m_bonusType);
    data["bonusId"].getScalar().convertTo(m_bonusId);
}

MapWitchHut::MapWitchHut(MapFormat format)
    : MapObjectAbstract(format)
{
    m_allowedSkills.resize(GameConstants(m_format).SKILL_QUANTITY);
}

void MapWitchHut::ReadInternal(ByteOrderDataStreamReader& stream)
{
    if (m_format > MapFormat::ROE)
        stream.readBits(m_allowedSkills);
}

void MapWitchHut::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    if (m_format > MapFormat::ROE)
        stream.writeBits(m_allowedSkills);
}

void MapWitchHut::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(m_allowedSkills, data["allowedSkills"]);
}

void MapWitchHut::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    m_allowedSkills.clear();
    reader.jsonToValue(data["allowedSkills"], m_allowedSkills);
}

void MapReward::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_gainedExp >> m_manaDiff >> m_moraleDiff >> m_luckDiff;

    stream >> m_resourceSet >> m_primSkillSet;

    auto lock = stream.SetContainerSizeBytesGuarded(1);
    stream >> m_secSkills;

    if (m_format > MapFormat::ROE) {
        stream >> m_artifacts;
    } else {
        std::vector<uint8_t> artifacts;
        stream >> artifacts;
        m_artifacts = std::vector<uint16_t>(artifacts.cbegin(), artifacts.cend());
    }

    stream >> m_spells;
    stream >> m_creatures;
}

void MapReward::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_gainedExp << m_manaDiff << m_moraleDiff << m_luckDiff;

    stream << m_resourceSet << m_primSkillSet;

    auto lock = stream.SetContainerSizeBytesGuarded(1);
    stream << m_secSkills;

    if (m_format > MapFormat::ROE) {
        stream << m_artifacts;
    } else {
        std::vector<uint8_t> artifacts;
        for (auto art : m_artifacts)
            artifacts.push_back(static_cast<uint8_t>(art));
        stream << artifacts;
    }

    stream << m_spells;
    stream << m_creatures;
}

void MapPandora::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_message;
    stream >> m_reward;

    stream.zeroPadding(8);
}

void MapPandora::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_message;
    stream << m_reward;

    stream.zeroPadding(8);
}

void MapPandora::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapPandora::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapGarison::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_owner;
    stream.zeroPadding(3);

    stream >> m_garison;

    if (m_format > MapFormat::ROE)
        stream >> m_removableUnits;
    stream.zeroPadding(8);
}

void MapGarison::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_owner;
    stream.zeroPadding(3);

    stream << m_garison;

    if (m_format > MapFormat::ROE)
        stream << m_removableUnits;
    stream.zeroPadding(8);
}

void MapGarison::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapGarison::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapSignBottle::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_message;
    stream.zeroPadding(4);
}

void MapSignBottle::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_message;
    stream.zeroPadding(4);
}

void MapSignBottle::ToJson(PropertyTree& data) const
{
    data = PropertyTreeScalar(m_message);
}

void MapSignBottle::FromJson(const PropertyTree& data)
{
    data.getScalar().convertTo(m_message);
}

void HeroArtSet::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_hasArts;

    if (!m_hasArts)
        return;

    auto loadArtifact = [&stream, this]() -> uint16_t {
        uint16_t result;
        if (m_format == MapFormat::ROE)
            result = stream.ReadScalar<uint8_t>();
        else
            stream >> result;
        return result;
    };

    m_mainSlots.resize(16);
    for (auto& art : m_mainSlots)
        art = loadArtifact();

    m_cata = loadArtifact();

    m_book = loadArtifact();

    if (m_format > MapFormat::ROE)
        m_misc5 = loadArtifact();

    uint16_t amount = 0;
    stream >> amount;
    m_bagSlots.resize(amount);
    for (auto& art : m_bagSlots)
        art = loadArtifact();
}

void HeroArtSet::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_hasArts;

    if (!m_hasArts)
        return;

    auto saveArtifact = [&stream, this](uint16_t art) {
        if (m_format == MapFormat::ROE)
            stream << static_cast<uint8_t>(art);
        else
            stream << art;
    };

    for (auto& art : m_mainSlots)
        saveArtifact(art);

    saveArtifact(m_cata);

    saveArtifact(m_book);

    if (m_format > MapFormat::ROE)
        saveArtifact(m_misc5);

    stream << static_cast<uint16_t>(m_bagSlots.size());
    for (auto& art : m_bagSlots)
        saveArtifact(art);
}

void MapEvent::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_message;
    stream >> m_reward;
    stream.zeroPadding(8);
    stream >> m_availableFor >> m_computerActivate >> m_removeAfterVisit;
    stream.zeroPadding(4);
}

void MapEvent::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_message;
    stream << m_reward;

    stream.zeroPadding(8);
    stream << m_availableFor << m_computerActivate << m_removeAfterVisit;
    stream.zeroPadding(4);
}

void MapEvent::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapEvent::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

void MapDwelling::ReadInternal(ByteOrderDataStreamReader& stream)
{
    stream >> m_owner;
    stream.zeroPadding(3);

    //216 and 217
    if (m_objectType == MapObjectType::RANDOM_DWELLING || m_objectType == MapObjectType::RANDOM_DWELLING_LVL) {
        stream >> m_factionId;
        if (!m_factionId)
            stream >> m_factionMask;
    }

    //216 and 218
    if (m_objectType == MapObjectType::RANDOM_DWELLING || m_objectType == MapObjectType::RANDOM_DWELLING_FACTION)
        stream >> m_minLevel >> m_maxLevel;
}

void MapDwelling::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    stream << m_owner;
    stream.zeroPadding(3);

    //216 and 217
    if (m_objectType == MapObjectType::RANDOM_DWELLING || m_objectType == MapObjectType::RANDOM_DWELLING_LVL) {
        stream << m_factionId;
        if (!m_factionId)
            stream << m_factionMask;
    }

    //216 and 218
    if (m_objectType == MapObjectType::RANDOM_DWELLING || m_objectType == MapObjectType::RANDOM_DWELLING_FACTION)
        stream << m_minLevel << m_maxLevel;
}

void MapDwelling::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapDwelling::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format, m_objectType };
    reader.jsonToValue(data, *this);
}

void MapQuestGuard::ReadInternal(ByteOrderDataStreamReader& stream)
{
    m_quest.ReadInternal(stream);
}

void MapQuestGuard::WriteInternal(ByteOrderDataStreamWriter& stream) const
{
    m_quest.WriteInternal(stream);
}

void MapQuestGuard::ToJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapQuestGuard::FromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = { m_format };
    reader.jsonToValue(data, *this);
}

}
