/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "H3MObjects.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"
#include "MernelPlatform/Logger.hpp"

#include "H3MObjectsReflection.hpp"

namespace FreeHeroes {
using namespace Mernel;

namespace {
#ifdef NDEBUG
constexpr const bool g_enablePaddingCheck = false;
#else
constexpr const bool g_enablePaddingCheck = true;
#endif
}

MapFormatFeatures::MapFormatFeatures(MapFormat format, int hotaVer1)
{
    const bool isHotaFactory = format >= MapFormat::HOTA1 && format <= MapFormat::HOTA3 && hotaVer1 >= 5;

    m_factions = 8; //ROE
    if (format >= MapFormat::AB)
        m_factions = 9;
    if (format >= MapFormat::HOTA1)
        m_factions = 10;

    m_artifactsCount = 128; //ROE
    if (format >= MapFormat::AB)
        m_artifactsCount = 129;
    if (format >= MapFormat::SOD)
        m_artifactsCount = 141;
    if (format >= MapFormat::HOTA1)
        m_artifactsCount = 165;
    if (isHotaFactory)
        m_artifactsCount = 166; // +1

    m_heroesCount = 128; // ROE
    if (format >= MapFormat::AB)
        m_heroesCount = 156;
    if (format >= MapFormat::HOTA1)
        m_heroesCount = 179;
    if (isHotaFactory)
        m_heroesCount = 198; // + 19

    m_spellsCount          = 81;
    m_spellsAbilitiesCount = 11;
    m_spellsRegularCount   = m_spellsCount - m_spellsAbilitiesCount;

    m_creaturesCount = 115;
    if (format >= MapFormat::AB)
        m_creaturesCount = 150;
    if (format >= MapFormat::HOTA1)
        m_creaturesCount = 171;
    if (isHotaFactory)
        m_creaturesCount = 187; // +16

    m_secondarySkillCount = 28;
    if (format >= MapFormat::HOTA1)
        m_secondarySkillCount = 29;

    m_primarySkillsCount = 4;
    m_terrainTypes       = 10;
    if (format >= MapFormat::HOTA1)
        m_terrainTypes = 12;

    m_resourceCount = 7;
    m_players       = 8;
    m_stackSize     = 7;

    if (format == MapFormat::HOTA3 && hotaVer1 < 3)
        m_heroesCount--;

    if (format == MapFormat::HOTA3 && hotaVer1 < 3)
        m_artifactsCount -= 2;

    m_hasQuestIdentifier = format > MapFormat::ROE;
    m_stackId16Bit       = format > MapFormat::ROE;
    m_artId16Bit         = format > MapFormat::ROE;
    m_factions16Bit      = format > MapFormat::ROE;

    m_heroHasExp          = format > MapFormat::AB;
    m_heroHasBio          = format > MapFormat::ROE;
    m_heroHasCustomSpells = format > MapFormat::AB;
    m_heroHasOneSpell     = format == MapFormat::AB;
    m_heroHasPrimSkills   = format > MapFormat::AB;

    m_heroHasSomethingUnknown = isHotaFactory;

    m_townHasObligatorySpells = format > MapFormat::ROE;
    m_townHasSpellResearch    = format >= MapFormat::HOTA1;
    m_townHasAlignment        = format > MapFormat::AB;

    m_townHasNewBuildingLogic = isHotaFactory;

    m_artifactHasPickupConditions = isHotaFactory;

    m_monsterJoinPercent        = format == MapFormat::HOTA3 && hotaVer1 >= 3;
    m_monsterHasQuantityByValue = isHotaFactory;

    m_creatureBankSize = format == MapFormat::HOTA3 && hotaVer1 >= 3;

    m_seerHutExtendedQuest  = format > MapFormat::ROE;
    m_seerHutMultiQuest     = format == MapFormat::HOTA3 && hotaVer1 >= 3;
    m_witchHutAllowedSkills = format > MapFormat::ROE;

    m_garisonRemovableUnits = format > MapFormat::ROE;

    m_artifactMiscFive = format > MapFormat::AB;

    m_eventHasHumanActivate     = format == MapFormat::HOTA3 && hotaVer1 >= 3;
    m_townEventHasHumanAffected = format > MapFormat::AB;

    m_playerP7               = format >= MapFormat::SOD;
    m_playerGenerateHeroInfo = format > MapFormat::ROE;
    m_playerPlaceholders     = format > MapFormat::ROE;

    m_mapLevelLimit        = format > MapFormat::ROE;
    m_mapPlaceholderHeroes = format > MapFormat::ROE;
    m_mapDisposedHeroes    = format >= MapFormat::SOD;

    m_mapAllowedHeroesSized    = format >= MapFormat::HOTA1;
    m_mapAllowedArtifacts      = format > MapFormat::ROE;
    m_mapAllowedArtifactsSized = format >= MapFormat::HOTA1;
    m_mapAllowedSpells         = format >= MapFormat::SOD;
    m_mapAllowedSecSkills      = format >= MapFormat::SOD;

    m_mapCustomHeroData         = format >= MapFormat::SOD;
    m_mapCustomHeroSize         = format >= MapFormat::HOTA1;
    m_mapCustomHeroDataExtended = isHotaFactory;

    m_mapEventHuman = format > MapFormat::AB;

    m_mapHotaUnknown1 = format >= MapFormat::HOTA1;
}

std::unique_ptr<IMapObject> IMapObject::Create(MapObjectType type, uint32_t subid)
{
    switch (type) {
        case MapObjectType::EVENT:
        {
            return std::make_unique<MapEvent>();
        }
        case MapObjectType::HERO:
        case MapObjectType::RANDOM_HERO:
        case MapObjectType::PRISON:
        {
            return std::make_unique<MapHero>();
        }
        case MapObjectType::MONSTER:
        case MapObjectType::RANDOM_MONSTER:
        case MapObjectType::RANDOM_MONSTER_L1:
        case MapObjectType::RANDOM_MONSTER_L2:
        case MapObjectType::RANDOM_MONSTER_L3:
        case MapObjectType::RANDOM_MONSTER_L4:
        case MapObjectType::RANDOM_MONSTER_L5:
        case MapObjectType::RANDOM_MONSTER_L6:
        case MapObjectType::RANDOM_MONSTER_L7:
        {
            return std::make_unique<MapMonster>();
        }
        case MapObjectType::OCEAN_BOTTLE:
        case MapObjectType::SIGN:
        {
            return std::make_unique<MapSignBottle>();
        }
        case MapObjectType::SEER_HUT:
        {
            return std::make_unique<MapSeerHut>();
        }
        case MapObjectType::WITCH_HUT:
        {
            return std::make_unique<MapWitchHut>();
        }
        case MapObjectType::SCHOLAR:
        {
            return std::make_unique<MapScholar>();
        }
        case MapObjectType::GARRISON:
        case MapObjectType::GARRISON2:
        {
            return std::make_unique<MapGarison>();
        }
        case MapObjectType::ARTIFACT:
        case MapObjectType::RANDOM_ART:
        case MapObjectType::RANDOM_TREASURE_ART:
        case MapObjectType::RANDOM_MINOR_ART:
        case MapObjectType::RANDOM_MAJOR_ART:
        case MapObjectType::RANDOM_RELIC_ART:
        case MapObjectType::SPELL_SCROLL:
        {
            return std::make_unique<MapArtifact>(type == MapObjectType::SPELL_SCROLL);
        }
        case MapObjectType::RANDOM_RESOURCE:
        case MapObjectType::RESOURCE:
        {
            return std::make_unique<MapResource>();
        }
        case MapObjectType::RANDOM_TOWN:
        case MapObjectType::TOWN:
        {
            return std::make_unique<MapTown>();
        }
        case MapObjectType::MINE:
        case MapObjectType::ABANDONED_MINE:
        {
            if (subid >= 7)
                return std::make_unique<MapAbandonedMine>();
            return std::make_unique<MapObjectWithOwner>();
        }
        case MapObjectType::CREATURE_GENERATOR1:
        case MapObjectType::CREATURE_GENERATOR2:
        case MapObjectType::CREATURE_GENERATOR3:
        case MapObjectType::CREATURE_GENERATOR4:
        case MapObjectType::SHIPYARD:
        case MapObjectType::LIGHTHOUSE:
        {
            return std::make_unique<MapObjectWithOwner>();
        }
        case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
        case MapObjectType::SHRINE_OF_MAGIC_GESTURE:
        case MapObjectType::SHRINE_OF_MAGIC_THOUGHT:
        {
            return std::make_unique<MapShrine>();
        }
        case MapObjectType::PANDORAS_BOX:
        {
            return std::make_unique<MapPandora>();
        }
        case MapObjectType::GRAIL:
        {
            return std::make_unique<MapGrail>();
        }
        case MapObjectType::QUEST_GUARD:
        {
            return std::make_unique<MapQuestGuard>();
        }
        case MapObjectType::RANDOM_DWELLING:         //same as castle + level range  216
        case MapObjectType::RANDOM_DWELLING_LVL:     //same as castle, fixed level   217
        case MapObjectType::RANDOM_DWELLING_FACTION: //level range, fixed faction    218
        {
            bool hasFaction = type == MapObjectType::RANDOM_DWELLING || type == MapObjectType::RANDOM_DWELLING_LVL;
            bool hasLevel   = type == MapObjectType::RANDOM_DWELLING || type == MapObjectType::RANDOM_DWELLING_FACTION;
            return std::make_unique<MapDwelling>(hasFaction, hasLevel);
        }

        case MapObjectType::HERO_PLACEHOLDER:
        {
            return std::make_unique<MapHeroPlaceholder>();
        }
        case MapObjectType::CREATURE_BANK:
        case MapObjectType::DERELICT_SHIP:
        case MapObjectType::DRAGON_UTOPIA:
        case MapObjectType::CRYPT:
        case MapObjectType::SHIPWRECK:
        {
            return std::make_unique<MapObjectCreatureBank>();
        }
        case MapObjectType::BORDER_GATE:
        {
            if (subid == 1000)
                return std::make_unique<MapQuestGuard>();
            return std::make_unique<MapObjectSimple>();
        }
        default:
        {
            return std::make_unique<MapObjectSimple>();
        }
    }
}

void MapHero::prepareArrays(const MapFormatFeatures* m_features)
{
    m_spellSet.prepareArrays(m_features);
    m_primSkillSet.prepareArrays(m_features);
    m_artSet.prepareArrays(m_features);
    m_garison.prepareArrays(m_features);
}

void MapHero::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);

    if (m_features->m_hasQuestIdentifier) {
        stream >> m_questIdentifier;
    }
    stream >> m_playerOwner >> m_subID;

    stream >> m_hasName;
    if (m_hasName)
        stream >> m_name;

    if (m_features->m_heroHasExp) {
        stream >> m_hasExp;
        if (m_hasExp)
            stream >> m_exp;

    } else {
        stream >> m_exp;
        if (m_exp >= 0)
            m_hasExp = true;
    }

    stream >> m_hasPortrait;
    if (m_hasPortrait)
        stream >> m_portrait;

    stream >> m_hasSecSkills;
    if (m_hasSecSkills)
        stream >> m_secSkills;

    stream >> m_hasArmy;
    if (m_hasArmy)
        stream >> m_garison;

    stream >> m_formation;

    stream >> m_artSet;

    stream >> m_patrolRadius;

    if (m_features->m_heroHasBio) {
        stream >> m_hasCustomBio;
        if (m_hasCustomBio)
            stream >> m_bio;

        stream >> m_sex;
    }

    stream >> m_spellSet;
    stream >> m_primSkillSet;

    stream.zeroPaddingChecked(16, g_enablePaddingCheck);

    if (m_features->m_heroHasSomethingUnknown) {
        stream >> m_unknown1 >> m_unknown2;
    }
}

void MapHero::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_hasQuestIdentifier) {
        stream << m_questIdentifier;
    }
    stream << m_playerOwner << m_subID;

    stream << m_hasName;
    if (m_hasName)
        stream << m_name;

    if (m_features->m_heroHasExp) {
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

    stream << m_hasArmy;
    if (m_hasArmy)
        stream << m_garison;

    stream << m_formation;

    stream << m_artSet;

    stream << m_patrolRadius;

    if (m_features->m_heroHasBio) {
        stream << m_hasCustomBio;
        if (m_hasCustomBio)
            stream << m_bio;

        stream << m_sex;
    }

    stream << m_spellSet;
    stream << m_primSkillSet;

    stream.zeroPadding(16);

    if (m_features->m_heroHasSomethingUnknown) {
        stream << m_unknown1 << m_unknown2;
    }
}

void MapHero::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapHero::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapTownEvent::prepareArrays(const MapFormatFeatures* m_features)
{
    m_buildings.resize(48);
    m_creaturesAmounts.resize(m_features->m_stackSize);
    m_resourceSet.prepareArrays(m_features);
}

void MapTownEvent::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    stream >> m_name >> m_message;
    stream >> m_resourceSet;
    stream >> m_players;
    if (m_features->m_townEventHasHumanAffected)
        stream >> m_humanAffected;

    stream >> m_computerAffected >> m_firstOccurence >> m_nextOccurence;

    stream.zeroPaddingChecked(17, g_enablePaddingCheck);

    stream.readBits(m_buildings);

    for (auto& amount : m_creaturesAmounts)
        stream >> amount;

    stream.zeroPaddingChecked(4, g_enablePaddingCheck);
}

void MapTownEvent::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    stream << m_name << m_message;
    stream << m_resourceSet;
    stream << m_players;
    if (m_features->m_townEventHasHumanAffected)
        stream << m_humanAffected;

    stream << m_computerAffected << m_firstOccurence << m_nextOccurence;

    stream.zeroPadding(17);

    stream.writeBits(m_buildings);

    for (auto& amount : m_creaturesAmounts)
        stream << amount;

    stream.zeroPadding(4);
}

void MapTown::prepareArrays(const MapFormatFeatures* m_features)
{
    if (m_features->m_townHasObligatorySpells)
        m_obligatorySpells.resize(9);
    m_possibleSpells.resize(9);

    m_builtBuildings.resize(48);
    m_forbiddenBuildings.resize(48);
    m_garison.prepareArrays(m_features);
}

void MapTown::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    if (m_features->m_hasQuestIdentifier)
        stream >> m_questIdentifier;

    stream >> m_playerOwner;

    stream >> m_hasName;
    if (m_hasName)
        stream >> m_name;

    stream >> m_hasGarison;
    if (m_hasGarison)
        stream >> m_garison;

    stream >> m_formation;

    stream >> m_hasCustomBuildings;

    if (m_hasCustomBuildings) {
        stream.readBits(m_builtBuildings);
        stream.readBits(m_forbiddenBuildings);
    } else {
        stream >> m_hasFort;
    }

    if (m_features->m_townHasObligatorySpells) {
        for (auto& byte : m_obligatorySpells)
            stream >> byte;
    }

    for (auto& byte : m_possibleSpells)
        stream >> byte;

    if (m_features->m_townHasSpellResearch)
        stream >> m_spellResearch;

    if (m_features->m_townHasNewBuildingLogic) {
        stream >> m_somethingBuildingRelated;
    }

    m_events.resize(stream.readSize());
    for (auto& event : m_events) {
        stream >> event;
    }

    if (m_features->m_townHasAlignment)
        stream >> m_alignment;

    stream.zeroPaddingChecked(3, g_enablePaddingCheck);
}

void MapTown::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_hasQuestIdentifier) {
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
        stream.writeBits(m_builtBuildings);
        stream.writeBits(m_forbiddenBuildings);
    } else {
        stream << m_hasFort;
    }

    if (m_features->m_townHasObligatorySpells) {
        for (auto& byte : m_obligatorySpells)
            stream << byte;
    }
    for (auto& byte : m_possibleSpells)
        stream << byte;

    if (m_features->m_townHasSpellResearch)
        stream << m_spellResearch;

    if (m_features->m_townHasNewBuildingLogic) {
        stream << m_somethingBuildingRelated;
    }

    stream << m_events;

    if (m_features->m_townHasAlignment)
        stream << m_alignment;

    stream.zeroPadding(3);
}

void MapTown::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapTown::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapHeroSkill::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_id >> m_level;
}

void MapHeroSkill::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_id << m_level;
}

void MapMonster::prepareArrays(const MapFormatFeatures* m_features)
{
    m_resourceSet.prepareArrays(m_features);
}

void MapMonster::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    if (m_features->m_hasQuestIdentifier)
        stream >> m_questIdentifier;

    stream >> m_count;
    stream >> m_joinAppeal;
    stream >> m_hasMessage;

    if (m_hasMessage) {
        stream >> m_message;
        stream >> m_resourceSet;
        m_features->readArtifact(stream, m_artID);
    }

    stream >> m_neverFlees >> m_notGrowingTeam;
    stream.zeroPaddingChecked(2, g_enablePaddingCheck);

    if (m_features->m_monsterJoinPercent) {
        stream >> m_aggressionExact >> m_joinOnlyForMoney;
        stream >> m_joinPercent;
        stream >> m_upgradedStack >> m_splitStack;
    }
    if (m_features->m_monsterHasQuantityByValue) {
        stream >> m_quantityMode >> m_quantityByAiValue;
    }
}

void MapMonster::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_hasQuestIdentifier) {
        stream << m_questIdentifier;
    }
    stream << m_count;
    stream << m_joinAppeal;
    stream << m_hasMessage;
    if (m_hasMessage) {
        stream << m_message;
        stream << m_resourceSet;

        m_features->writeArtifact(stream, m_artID);
    }

    stream << m_neverFlees << m_notGrowingTeam;
    stream.zeroPadding(2);

    if (m_features->m_monsterJoinPercent) {
        stream << m_aggressionExact << m_joinOnlyForMoney;
        stream << m_joinPercent;
        stream << m_upgradedStack << m_splitStack;
    }
    if (m_features->m_monsterHasQuantityByValue) {
        stream << m_quantityMode << m_quantityByAiValue;
    }
}

void MapMonster::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapMonster::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapObjectWithOwner::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_owner;
    stream.zeroPaddingChecked(3, g_enablePaddingCheck);
}

void MapObjectWithOwner::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_owner;
    stream.zeroPadding(3);
}

void MapObjectWithOwner::toJson(PropertyTree& data) const
{
    data["owner"] = PropertyTreeScalar(m_owner);
}

void MapObjectWithOwner::fromJson(const PropertyTree& data)
{
    data["owner"].getScalar().convertTo(m_owner);
}

void MapObjectCreatureBank::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_creatureBankSize) {
        stream >> m_content >> m_upgraded >> m_artifacts;
    }
}

void MapObjectCreatureBank::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_creatureBankSize) {
        stream << m_content << m_upgraded << m_artifacts;
    }
}

void MapObjectCreatureBank::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapObjectCreatureBank::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapResource::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);

    m_message.readBinary(stream);
    stream >> m_amount;
    stream.zeroPaddingChecked(4, g_enablePaddingCheck);
}

void MapResource::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    m_message.writeBinary(stream);
    stream << m_amount;
    stream.zeroPadding(4);
}

void MapResource::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapResource::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapGuards::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_hasGuards;
    if (!m_hasGuards)
        return;

    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);

    stream >> m_creatures;
}

void MapGuards::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_hasGuards;

    if (!m_hasGuards)
        return;

    stream << m_creatures;
}

void MapMessage::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_hasMessage;
    if (!m_hasMessage)
        return;

    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);

    stream >> m_message;
    stream >> m_guards;
    stream.zeroPaddingChecked(4, g_enablePaddingCheck);
}

void MapMessage::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_hasMessage;
    if (!m_hasMessage)
        return;

    stream << m_message;
    stream << m_guards;
    stream.zeroPadding(4);
}

void MapArtifact::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    m_message.readBinary(stream);

    if (m_isSpell)
        stream >> m_spellId;

    if (m_features->m_artifactHasPickupConditions) {
        stream >> m_pickupCondition1 >> m_pickupCondition2;
    }
}

void MapArtifact::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    m_message.writeBinary(stream);
    if (m_isSpell)
        stream << m_spellId;

    if (m_features->m_artifactHasPickupConditions) {
        stream << m_pickupCondition1 << m_pickupCondition2;
    }
}

void MapArtifact::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapArtifact::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = { m_isSpell };
    reader.jsonToValue(data, *this);
}

void MapQuest::readBinary(ByteOrderDataStreamReader& stream)
{
    m_missionType = static_cast<Mission>(stream.readScalar<uint8_t>());

    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);

    switch (m_missionType) {
        case Mission::NONE:
            return;
        case Mission::PRIMARY_STAT:
        {
            for (auto& stat : m_2stats)
                stream >> stat;

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
            auto lock = stream.setContainerSizeBytesGuarded(1);
            stream >> m_5arts;

            break;
        }
        case Mission::ARMY:
        {
            auto lock = stream.setContainerSizeBytesGuarded(1);
            stream >> m_6creatures;
            break;
        }
        case Mission::RESOURCES:
        {
            for (auto& res : m_7resources)
                stream >> res;

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

void MapQuest::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << static_cast<uint8_t>(m_missionType);

    switch (m_missionType) {
        case Mission::NONE:
            return;
        case Mission::PRIMARY_STAT:
        {
            for (auto& stat : m_2stats)
                stream << stat;

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
            auto lock = stream.setContainerSizeBytesGuarded(1);
            stream << m_5arts;

            break;
        }
        case Mission::ARMY:
        {
            auto lock = stream.setContainerSizeBytesGuarded(1);
            stream << m_6creatures;
            break;
        }
        case Mission::RESOURCES:
        {
            for (auto& res : m_7resources)
                stream << res;

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

void MapQuest::toJson(PropertyTree& data) const
{
    assert(0);
}

void MapQuest::fromJson(const PropertyTree& data)
{
    assert(0);
}

void MapSeerHut::MapQuestWithReward::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_seerHutExtendedQuest) {
        stream >> m_quest;
    } else {
        m_quest.prepareArrays(m_features);
        //RoE
        uint8_t artID = 0;
        stream >> artID;
        if (artID != 255) {
            m_quest.m_5arts.push_back(artID);
            m_quest.m_missionType = Mission::ART;
        }
    }

    m_reward = static_cast<RewardType>(stream.readScalar<uint8_t>());
    switch (m_reward) {
        case RewardType::EXPERIENCE:
        case RewardType::MANA_POINTS:
        {
            m_rVal = stream.readScalar<uint32_t>();
            break;
        }
        case RewardType::MORALE_BONUS:
        case RewardType::LUCK_BONUS:
        {
            m_rVal = stream.readScalar<uint8_t>();
            break;
        }
        case RewardType::RESOURCES:
        {
            m_rID = stream.readScalar<uint8_t>();
            // Only the first 3 bytes are used. Skip the 4th.
            m_rVal = stream.readScalar<uint32_t>() & 0x00ffffff;
            break;
        }
        case RewardType::PRIMARY_SKILL:
        case RewardType::SECONDARY_SKILL:
        {
            m_rID  = stream.readScalar<uint8_t>();
            m_rVal = stream.readScalar<uint8_t>();
            break;
        }
        case RewardType::ARTIFACT:
        {
            m_features->readArtifact(stream, m_rID);
            break;
        }
        case RewardType::SPELL:
        {
            m_rID = stream.readScalar<uint8_t>();
            break;
        }
        case RewardType::CREATURE:
        {
            m_features->readUnit(stream, m_rID);
            m_rVal = stream.readScalar<uint16_t>();
            break;
        }
        case RewardType::NOTHING:
        {
            break;
        }
    }
    stream.zeroPaddingChecked(2, g_enablePaddingCheck);
}

void MapSeerHut::MapQuestWithReward::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_seerHutExtendedQuest) {
        stream << m_quest;
    } else {
        uint8_t artID = 255;
        if (!m_quest.m_5arts.empty())
            artID = static_cast<uint8_t>(m_quest.m_5arts[0]);
        stream << artID;
    }

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
            m_features->writeArtifact(stream, m_rID);
            break;
        }
        case RewardType::SPELL:
        {
            stream << static_cast<uint8_t>(m_rID);
            break;
        }
        case RewardType::CREATURE:
        {
            m_features->writeUnit(stream, m_rID);

            stream << static_cast<uint16_t>(m_rVal);
            break;
        }
        case RewardType::NOTHING:
        {
            break;
        }
    }
    stream.zeroPadding(2);
}

void MapSeerHut::MapQuestWithReward::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapSeerHut::MapQuestWithReward::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapSeerHut::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);

    if (m_features->m_seerHutMultiQuest) {
        stream >> m_questsOneTime >> m_questsRecurring;
    } else {
        m_questsOneTime.resize(1);
        stream >> m_questsOneTime[0];
    }
}

void MapSeerHut::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_seerHutMultiQuest) {
        stream << m_questsOneTime << m_questsRecurring;
    } else {
        stream << m_questsOneTime.at(0);
    }
}

void MapSeerHut::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapSeerHut::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapShrine::toJson(PropertyTree& data) const
{
    data["spell"] = PropertyTreeScalar(m_spell);
}

void MapShrine::fromJson(const PropertyTree& data)
{
    data["spell"].getScalar().convertTo(m_spell);
}

void MapScholar::toJson(PropertyTree& data) const
{
    data["bonusType"] = PropertyTreeScalar(m_bonusType);
    data["bonusId"]   = PropertyTreeScalar(m_bonusId);
}

void MapScholar::fromJson(const PropertyTree& data)
{
    data["bonusType"].getScalar().convertTo(m_bonusType);
    data["bonusId"].getScalar().convertTo(m_bonusId);
}

void MapWitchHut::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    if (m_features->m_witchHutAllowedSkills) {
        stream.readBits(m_allowedSkills);
    }
}

void MapWitchHut::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);

    if (m_features->m_witchHutAllowedSkills)
        stream.writeBits(m_allowedSkills);
}

void MapWitchHut::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(m_allowedSkills, data["allowedSkills"]);
}

void MapWitchHut::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    m_allowedSkills.clear();
    reader.jsonToValue(data["allowedSkills"], m_allowedSkills);
}

void MapReward::prepareArrays(const MapFormatFeatures* m_features)
{
    m_resourceSet.prepareArrays(m_features);
}

void MapReward::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    stream >> m_gainedExp >> m_manaDiff >> m_moraleDiff >> m_luckDiff;

    stream >> m_resourceSet >> m_primSkillSet;

    auto lock = stream.setContainerSizeBytesGuarded(1);
    stream >> m_secSkills;

    auto artsize = stream.readSize();
    m_artifacts.resize(artsize);
    for (auto& art : m_artifacts)
        m_features->readArtifact(stream, art);

    stream >> m_spells;
    stream >> m_creatures;
}

void MapReward::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    stream << m_gainedExp << m_manaDiff << m_moraleDiff << m_luckDiff;

    stream << m_resourceSet << m_primSkillSet;

    auto lock = stream.setContainerSizeBytesGuarded(1);
    stream << m_secSkills;

    stream.writeSize(m_artifacts.size());
    for (auto& art : m_artifacts)
        m_features->writeArtifact(stream, art);

    stream << m_spells;
    stream << m_creatures;
}

void MapPandora::prepareArrays(const MapFormatFeatures* m_features)
{
    m_message.prepareArrays(m_features);
}

void MapPandora::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    stream >> m_message;
    stream >> m_reward;

    stream.zeroPaddingChecked(8, g_enablePaddingCheck);
}

void MapPandora::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_message;
    stream << m_reward;

    stream.zeroPadding(8);
}

void MapPandora::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapPandora::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapGarison::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    stream >> m_owner;
    stream.zeroPaddingChecked(3, g_enablePaddingCheck);

    stream >> m_garison;

    if (m_features->m_garisonRemovableUnits)
        stream >> m_removableUnits;
    stream.zeroPaddingChecked(8, g_enablePaddingCheck);
}

void MapGarison::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    stream << m_owner;
    stream.zeroPadding(3);

    stream << m_garison;

    if (m_features->m_garisonRemovableUnits)
        stream << m_removableUnits;
    stream.zeroPadding(8);
}

void MapGarison::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapGarison::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapSignBottle::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_message;
    stream.zeroPaddingChecked(4, g_enablePaddingCheck);
}

void MapSignBottle::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_message;
    stream.zeroPadding(4);
}

void MapSignBottle::toJson(PropertyTree& data) const
{
    data = PropertyTreeScalar(m_message);
}

void MapSignBottle::fromJson(const PropertyTree& data)
{
    data.getScalar().convertTo(m_message);
}

void HeroArtSet::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);

    stream >> m_hasArts;

    if (!m_hasArts)
        return;

    auto loadArtifact = [&stream, m_features]() -> uint16_t {
        uint16_t result = 0;
        m_features->readArtifact(stream, result);
        return result;
    };

    for (auto& art : m_mainSlots)
        art = loadArtifact();

    m_cata = loadArtifact();

    m_book = loadArtifact();

    if (m_features->m_artifactMiscFive)
        m_misc5 = loadArtifact();

    uint16_t amount = 0;
    stream >> amount;
    m_bagSlots.resize(amount);
    for (auto& art : m_bagSlots)
        art = loadArtifact();
}

void HeroArtSet::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    stream << m_hasArts;

    if (!m_hasArts)
        return;

    auto saveArtifact = [&stream, m_features](uint16_t art) {
        m_features->writeArtifact(stream, art);
    };

    for (auto& art : m_mainSlots)
        saveArtifact(art);

    saveArtifact(m_cata);

    saveArtifact(m_book);

    if (m_features->m_artifactMiscFive)
        saveArtifact(m_misc5);

    stream << static_cast<uint16_t>(m_bagSlots.size());
    for (auto& art : m_bagSlots)
        saveArtifact(art);
}

void HeroSpellSet::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);

    prepareArrays(m_features);

    if (m_features->m_heroHasCustomSpells) {
        stream >> m_hasCustomSpells;

        if (m_hasCustomSpells)
            stream.readBits(m_spells);

    } else if (m_features->m_heroHasOneSpell) {
        //we can read one spell
        auto buff = stream.readScalar<uint8_t>();
        if (buff != 254) {
            m_hasCustomSpells = true;
        }
    }
}

void HeroSpellSet::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_heroHasCustomSpells) {
        stream << m_hasCustomSpells;
        if (m_hasCustomSpells)
            stream.writeBits(m_spells);

    } else if (m_features->m_heroHasOneSpell) {
        uint8_t spell = 254;
        if (m_hasCustomSpells) {
            spell = 255; // @todo: look in m_spells
        }
        stream << spell;
    }
}

void HeroPrimSkillSet::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);

    if (m_features->m_heroHasPrimSkills) {
        stream >> m_hasCustomPrimSkills;
        if (m_hasCustomPrimSkills) {
            m_primSkills.resize(m_features->m_primarySkillsCount);
            for (auto& skill : m_primSkills)
                stream >> skill;
        }
    }
}

void HeroPrimSkillSet::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    if (m_features->m_heroHasPrimSkills) {
        stream << m_hasCustomPrimSkills;
        if (m_hasCustomPrimSkills) {
            for (auto& skill : m_primSkills)
                stream << skill;
        }
    }
}

void MapEvent::prepareArrays(const MapFormatFeatures* m_features)
{
    m_players.resize(m_features->m_players);
    m_message.prepareArrays(m_features);
    m_reward.prepareArrays(m_features);
}

void MapEvent::readBinary(ByteOrderDataStreamReader& stream)
{
    auto* m_features = getFeaturesFromStream(stream);
    prepareArrays(m_features);
    stream >> m_message;
    stream >> m_reward;
    stream.zeroPaddingChecked(8, g_enablePaddingCheck);
    stream.readBits(m_players);
    stream >> m_computerActivate >> m_removeAfterVisit;
    stream.zeroPaddingChecked(4, g_enablePaddingCheck);
    if (m_features->m_eventHasHumanActivate)
        stream >> m_humanActivate;
}

void MapEvent::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    auto* m_features = getFeaturesFromStream(stream);
    stream << m_message;
    stream << m_reward;

    stream.zeroPadding(8);
    stream.writeBits(m_players);
    stream << m_computerActivate << m_removeAfterVisit;
    stream.zeroPadding(4);
    if (m_features->m_eventHasHumanActivate)
        stream << m_humanActivate;
}

void MapEvent::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapEvent::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapDwelling::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_owner;
    stream.zeroPaddingChecked(3, g_enablePaddingCheck);

    if (m_hasFaction) {
        stream >> m_factionId;
        if (!m_factionId)
            stream >> m_factionMask;
    }

    if (m_hasLevel)
        stream >> m_minLevel >> m_maxLevel;
}

void MapDwelling::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_owner;
    stream.zeroPadding(3);

    if (m_hasFaction) {
        stream << m_factionId;
        if (!m_factionId)
            stream << m_factionMask;
    }

    if (m_hasLevel)
        stream << m_minLevel << m_maxLevel;
}

void MapDwelling::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapDwelling::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = { m_hasFaction, m_hasLevel };
    reader.jsonToValue(data, *this);
}

void MapQuestGuard::readBinary(ByteOrderDataStreamReader& stream)
{
    m_quest.readBinary(stream);
}

void MapQuestGuard::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    m_quest.writeBinary(stream);
}

void MapQuestGuard::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapQuestGuard::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapGrail::toJson(PropertyTree& data) const
{
    data["radius"] = PropertyTreeScalar(m_radius);
}

void MapGrail::fromJson(const PropertyTree& data)
{
    data["radius"].getScalar().convertTo(m_radius);
}

void MapHeroPlaceholder::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_owner >> m_hero;
    if (m_hero == uint8_t(-1))
        stream >> m_powerRank;
}

void MapHeroPlaceholder::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_owner << m_hero;
    if (m_hero == uint8_t(-1))
        stream << m_powerRank;
}

void MapHeroPlaceholder::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapHeroPlaceholder::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

void MapAbandonedMine::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void MapAbandonedMine::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

}
