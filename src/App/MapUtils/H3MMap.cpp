/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "H3MMap.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"
#include "H3MMapReflection.hpp"
#include "H3MObjectsReflection.hpp"

#include <set>

namespace FreeHeroes {

void H3Map::VictoryCondition::readBinary(ByteOrderDataStreamReader& stream)
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
                m_artID = stream.readScalar<uint8_t>();
            break;
        }
        case VictoryConditionType::GATHERTROOP:
        {
            if (m_features->m_stackId16Bit)
                stream >> m_creatureID;
            else
                m_creatureID = stream.readScalar<uint8_t>();
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
            m_artID = stream.readScalar<uint8_t>();
            stream >> m_pos;
            break;
        }
    }
}

void H3Map::VictoryCondition::writeBinary(ByteOrderDataStreamWriter& stream) const
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

void H3Map::LossCondition::readBinary(ByteOrderDataStreamReader& stream)
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

void H3Map::LossCondition::writeBinary(ByteOrderDataStreamWriter& stream) const
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

void H3Map::readBinary(ByteOrderDataStreamReader& stream)
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
        stream >> playerInfo.m_canHumanPlay >> playerInfo.m_canComputerPlay;

        playerInfo.m_aiTactic = static_cast<AiTactic>(stream.readScalar<uint8_t>());

        if (m_features->m_playerP7) {
            stream >> playerInfo.m_unknown1;
        } else {
            playerInfo.m_unknown1 = -1;
        }

        // Factions this player can choose
        if (m_features->m_factions16Bit)
            stream >> playerInfo.m_allowedFactionsBitmask;
        else
            playerInfo.m_allowedFactionsBitmask = stream.readScalar<uint8_t>();

        stream >> playerInfo.m_isFactionRandom >> playerInfo.m_hasMainTown;
        if (playerInfo.m_hasMainTown) {
            playerInfo.m_generateHeroAtMainTown = true;
            playerInfo.m_generateHero           = false;
            if (m_features->m_playerGenerateHeroInfo)
                stream >> playerInfo.m_generateHeroAtMainTown >> playerInfo.m_generateHero;

            stream >> playerInfo.m_posOfMainTown;
        }

        stream >> playerInfo.m_hasRandomHero >> playerInfo.m_mainCustomHeroId;

        if (playerInfo.m_mainCustomHeroId != 0xff) {
            stream >> playerInfo.m_mainCustomHeroPortrait;
            stream >> playerInfo.m_mainCustomHeroName;
        }

        if (m_features->m_playerPlaceholders)
            stream >> playerInfo.m_powerPlaceholders >> playerInfo.m_heroesNames;
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
        const auto disp = stream.readScalar<uint8_t>();
        m_disposedHeroes.resize(disp);
        for (auto& hero : m_disposedHeroes)
            stream >> hero;
    }

    stream.zeroPadding(31);

    if (m_features->m_mapHotaUnknown1) {
        uint16_t unknown1; // == 16;
        uint32_t unknown2; // == 0;
        stream >> m_hotaVer.m_fixedMap >> unknown1 >> unknown2;
        assert(unknown1 == 16);
        assert(unknown2 == 0);
        if (m_hotaVer.m_ver1 == 3)
            stream >> m_hotaVer.m_roundLimit;
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
            if (m_customHeroData.size() != stream.readSize())
                throw std::runtime_error("heroCount check failed for HOTA header");
        }
        for (auto& hero : m_customHeroData)
            stream >> hero;
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
        for (auto& obj : m_objects) {
            stream >> obj.m_pos >> obj.m_defnum;

            const ObjectTemplate& objTempl = m_objectDefs.at(obj.m_defnum);
            MapObjectType         type     = static_cast<MapObjectType>(objTempl.m_id);
            stream.zeroPadding(5);
            obj.m_impl = IMapObject::Create(type, m_features);
            if (!obj.m_impl)
                throw std::runtime_error("Unsupported map object type:" + std::to_string(objTempl.m_id));

            obj.m_impl->readBinary(stream);
        }
    }

    m_events.resize(stream.readSize());
    for (auto& event : m_events) {
        event.m_features = m_features;
        stream >> event;
    }

    stream.zeroPadding(124);
}

void H3Map::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << static_cast<int32_t>(m_format);

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
        stream << playerInfo.m_canHumanPlay << playerInfo.m_canComputerPlay;

        stream << static_cast<uint8_t>(playerInfo.m_aiTactic);

        if (m_features->m_playerP7)
            stream << playerInfo.m_unknown1;

        if (m_features->m_factions16Bit)
            stream << playerInfo.m_allowedFactionsBitmask;
        else
            stream << uint8_t(playerInfo.m_allowedFactionsBitmask);

        stream << playerInfo.m_isFactionRandom << playerInfo.m_hasMainTown;
        if (playerInfo.m_hasMainTown) {
            if (m_features->m_playerGenerateHeroInfo)
                stream << playerInfo.m_generateHeroAtMainTown << playerInfo.m_generateHero;

            stream << playerInfo.m_posOfMainTown;
        }

        stream << playerInfo.m_hasRandomHero << playerInfo.m_mainCustomHeroId;

        if (playerInfo.m_mainCustomHeroId != 0xff) {
            stream << playerInfo.m_mainCustomHeroPortrait;
            stream << playerInfo.m_mainCustomHeroName;
        }

        if (m_features->m_playerPlaceholders)
            stream << playerInfo.m_powerPlaceholders << playerInfo.m_heroesNames;
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
        uint16_t unknown1 = 16;
        uint32_t unknown2 = 0;
        stream << m_hotaVer.m_fixedMap << unknown1 << unknown2;
        if (m_hotaVer.m_ver1 == 3)
            stream << m_hotaVer.m_roundLimit;
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

            obj.m_impl->writeBinary(stream);
        }
    }

    stream << m_events;

    stream.zeroPadding(124);
}

void H3Map::toJson(PropertyTree& data) const
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
        obj.m_impl->toJson(objJson["impl"]);
        objList.append(std::move(objJson));
    }
}

void H3Map::fromJson(const PropertyTree& data)
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

        obj.m_impl->fromJson(objJson["impl"]);

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

void H3Pos::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_x >> m_y >> m_z;
}

void H3Pos::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_x << m_y << m_z;
}

void SHeroName::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_heroId;
    stream >> m_heroName;
}

void SHeroName::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_heroId;
    stream << m_heroName;
}

void DisposedHero::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_heroId >> m_portrait;
    stream >> m_name;
    stream >> m_players;
}

void DisposedHero::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_heroId << m_portrait;
    stream << m_name;
    stream << m_players;
}

void MapTile::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_terType >> m_terView >> m_riverType >> m_riverDir >> m_roadType >> m_roadDir >> m_extTileFlags;
}

void MapTile::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_terType << m_terView << m_riverType << m_riverDir << m_roadType << m_roadDir << m_extTileFlags;
}

void ObjectTemplate::prepareArrays()
{
    m_visitMask.resize(6);
    m_blockMask.resize(6);
}

void ObjectTemplate::readBinary(ByteOrderDataStreamReader& stream)
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
    m_type = static_cast<Type>(stream.readScalar<uint8_t>());
    stream >> m_drawPriority;

    stream.zeroPadding(16);
}

void ObjectTemplate::writeBinary(ByteOrderDataStreamWriter& stream) const
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

void GlobalMapEvent::readBinary(ByteOrderDataStreamReader& stream)
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

void GlobalMapEvent::writeBinary(ByteOrderDataStreamWriter& stream) const
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

void CustomHeroData::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_enabled;
    if (!m_enabled)
        return;

    m_spellSet.prepareArrays();
    m_primSkillSet.prepareArrays();

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

    stream >> m_spellSet >> m_primSkillSet;
}

void CustomHeroData::writeBinary(ByteOrderDataStreamWriter& stream) const
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
