/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "H3CCampaign.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"
#include "MernelPlatform/Logger.hpp"

#include "H3CCampaignReflection.hpp"

#include <array>
#include <set>

namespace FreeHeroes {

namespace {
constexpr const bool g_enableOffsetTrace = false;

[[maybe_unused]] void streamTrace(ByteOrderDataStreamReader& stream, const char* description)
{
    if (g_enableOffsetTrace)
        Mernel::Logger(Mernel::Logger::Warning) << description << " offset=" << stream.getBuffer().getOffsetRead();
}

[[maybe_unused]] void streamTrace(ByteOrderDataStreamWriter& stream, const char* description)
{
    if (g_enableOffsetTrace)
        Mernel::Logger(Mernel::Logger::Warning) << description << " offset=" << stream.getBuffer().getOffsetWrite();
}

enum class CampaignVersion
{
    NONE   = 0,
    RoE    = 4,
    AB     = 5,
    SoD    = 6,
    WoG    = 6,
    HotA16 = 6,
    HotA17 = 10,
    //		Chr = 7, // Heroes Chronicles, likely identical to SoD, untested

    VCMI     = 1,
    VCMI_MIN = 1,
    VCMI_MAX = 1,
};
enum class CampaignStartOptions
{
    NONE = 0,
    START_BONUS,
    HERO_CROSSOVER,
    HERO_OPTIONS
};

enum class CampaignBonusType
{
    NONE = -1,
    SPELL,
    MONSTER,
    BUILDING,
    ARTIFACT,
    SPELL_SCROLL,
    PRIMARY_SKILL,
    SECONDARY_SKILL,
    RESOURCE,
    HEROES_FROM_PREVIOUS_SCENARIO,
    HERO
};

// copy paste FROM VCMI, just to get required buffer size. Rework later.
void readScenarioTravel(ByteOrderDataStreamReader& stream, CampaignVersion version, size_t index)
{
    auto readContainer = [](ByteOrderDataStreamReader& stream, int sizeBytes) {
        stream.zeroPadding(sizeBytes);
    };

    uint8_t whatHeroKeeps;
    stream >> whatHeroKeeps;
    // whatHeroKeeps.experience = whatHeroKeeps & 1;
    // whatHeroKeeps.primarySkills = whatHeroKeeps & 2;
    // whatHeroKeeps.secondarySkills = whatHeroKeeps & 4;
    // whatHeroKeeps.spells = whatHeroKeeps & 8;
    // whatHeroKeeps.artifacts = whatHeroKeeps & 16;

    if (version == CampaignVersion::HotA17) {
        // 0F
        // 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
        // 00 00 00 00 00 00 00 00 00 00 00 00
        // 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 03 03 06 B3 00 01 01 03 B3 00 07 00 03 B3 00 9C 00
        stream.zeroPadding(19);
        stream.zeroPadding(13);
        std::vector<size_t> hotaTrailSizes{
            31, 34, 34, 37, 21, 33, 35, 31
        };
        stream.zeroPadding(hotaTrailSizes.at(index));
        // terrible hardcode of campaign scenarios variable data at the end, need to decode this later:
        /*
 1                   8                       16          20          24          28          32
00 00 00 00 00 00 00 00 00 00 00 00 00 01 03 03 06 B3 00 01 01 03 B3 00 07 00 03 B3 00 9C 00
00 36 00 00 00 00 00 00 00 00 00 00 00 01 06 03 07 FD 08 00 00 00 03 FD FF 54 00 01 B4 00 AC 00 0A 00
00 36 00 00 00 00 00 00 00 00 00 00 00 01 06 03 07 02 0F 00 00 00 03 B3 00 31 00 01 C3 00 B4 00 02 00
00 7E 00 00 00 00 00 00 00 0C 00 00 00 01 06 03 01 FD FF B1 00 08 00 01 FD FF AD 00 14 00 05 C3 00 00 02 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 03 02 01 BC 00 04 BE 00
00 00 00 00 00 00 00 00 00 00 00 00 00 01 03 03 03 FD FF 46 00 03 FD FF 72 00 05 B3 00 01 01 01 01
80 00 00 00 00 00 00 08 00 01 00 00 00 01 03 03 03 FD FF 52 00 01 B8 00 B6 00 04 00 05 B3 00 03 00 00 00
80 00 00 00 00 00 00 08 00 01 00 00 00 01 03 03 03 B8 00 73 00 03 B8 00 50 00 03 B8 00 96 00 
*/
        return;
    }

    readContainer(stream, 19);
    readContainer(stream, version < CampaignVersion::SoD ? 17 : 18);

    auto startOptions = static_cast<CampaignStartOptions>(stream.readScalar<uint8_t>());

    switch (startOptions) {
        case CampaignStartOptions::NONE:
            //no bonuses. Seems to be OK
            break;
        case CampaignStartOptions::START_BONUS: //reading of bonuses player can choose
        {
            stream.readScalar<uint8_t>(); // color
            uint8_t numOfBonuses = stream.readScalar<uint8_t>();
            for (int g = 0; g < numOfBonuses; ++g) {
                auto bonustype = static_cast<CampaignBonusType>(stream.readScalar<uint8_t>());
                //hero: FFFD means 'most powerful' and FFFE means 'generated'
                switch (bonustype) {
                    case CampaignBonusType::SPELL:
                    {
                        stream.readScalar<uint16_t>(); //hero
                        stream.readScalar<uint8_t>();  //spell ID
                        break;
                    }
                    case CampaignBonusType::MONSTER:
                    {
                        stream.readScalar<uint16_t>(); //hero
                        stream.readScalar<uint16_t>(); //monster type
                        stream.readScalar<uint16_t>(); //monster count
                        break;
                    }
                    case CampaignBonusType::BUILDING:
                    {
                        stream.readScalar<uint8_t>(); //building ID (0 - town hall, 1 - city hall, 2 - capitol, etc)
                        break;
                    }
                    case CampaignBonusType::ARTIFACT:
                    {
                        stream.readScalar<uint16_t>(); //hero
                        stream.readScalar<uint16_t>(); //artifact ID
                        break;
                    }
                    case CampaignBonusType::SPELL_SCROLL:
                    {
                        stream.readScalar<uint16_t>(); //hero
                        stream.readScalar<uint8_t>();  //spell ID
                        break;
                    }
                    case CampaignBonusType::PRIMARY_SKILL:
                    {
                        stream.readScalar<uint16_t>(); //hero
                        stream.readScalar<uint32_t>(); //bonuses (4 bytes for 4 skills)
                        break;
                    }
                    case CampaignBonusType::SECONDARY_SKILL:
                    {
                        stream.readScalar<uint16_t>(); //hero
                        stream.readScalar<uint8_t>();  //skill ID
                        stream.readScalar<uint8_t>();  //skill level
                        break;
                    }
                    case CampaignBonusType::RESOURCE:
                    {
                        stream.readScalar<uint8_t>(); //type
                        //FD - wood+ore
                        //FE - mercury+sulfur+crystal+gem
                        stream.readScalar<uint32_t>(); //count
                        break;
                    }
                    default:
                        assert("Corrupted h3c file");
                        break;
                }
            }
            break;
        }
        case CampaignStartOptions::HERO_CROSSOVER: //reading of players (colors / scenarios ?) player can choose
        {
            uint8_t numOfBonuses = stream.readScalar<uint8_t>();
            for (int g = 0; g < numOfBonuses; ++g) {
                stream.readScalar<uint8_t>(); //player color
                stream.readScalar<uint8_t>(); //from what scenario
            }
            break;
        }
        case CampaignStartOptions::HERO_OPTIONS: //heroes player can choose between
        {
            uint8_t numOfBonuses = stream.readScalar<uint8_t>();
            for (int g = 0; g < numOfBonuses; ++g) {
                stream.readScalar<uint8_t>();  //player color
                stream.readScalar<uint16_t>(); //hero, FF FF - random
            }
            break;
        }
        default:
        {
            assert("Corrupted h3c file");
            break;
        }
    }
};

}

H3CCampaign::H3CCampaign()
{
}

void H3CCampaign::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_version;
    if (m_version >= static_cast<uint32_t>(CampaignVersion::HotA17))
        stream >> m_somethingHota1 >> m_somethingHota2 >> m_somethingHota2a >> m_somethingHota3;
    stream >> m_campId >> m_name;
    stream >> m_description;
    if (m_version > static_cast<uint32_t>(CampaignVersion::RoE))
        stream >> m_difficultyChoosenByPlayer;
    stream >> m_musicId;

    streamTrace(stream, "Before scenario list");
    const size_t totalScenarios = m_scenarios.size();
    for (size_t i = 0; i < totalScenarios; i++) {
        Scenario& sc        = m_scenarios[i];
        sc.m_index          = i;
        sc.m_totalScenarios = totalScenarios;
        sc.m_version        = m_version;
        stream >> sc;
        if (sc.m_filename.empty()) {
            sc.m_filename = "file" + std::to_string(i) + ".h3m";
        }
    }
}

void H3CCampaign::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_version;
    if (m_version >= static_cast<uint32_t>(CampaignVersion::HotA17))
        stream << m_somethingHota1 << m_somethingHota2 << m_somethingHota2a << m_somethingHota2;
    stream << m_campId << m_name;
    stream << m_description;
    if (m_version > static_cast<uint32_t>(CampaignVersion::RoE))
        stream << m_difficultyChoosenByPlayer;
    stream << m_musicId;

    for (size_t i = 0; i < m_scenarios.size(); i++) {
        const Scenario& sc = m_scenarios[i];
        assert(sc.m_version == m_version);
        stream << sc;
    }
}

void H3CCampaign::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
    PropertyTree& objList = data["scenarios"];
    objList.convertToList();
    for (auto& obj : m_scenarios) {
        PropertyTree objJson;
        writer.valueToJson(obj, objJson);
        objList.append(std::move(objJson));
    }
}

void H3CCampaign::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);

    for (const PropertyTree& objJson : data["scenarios"].getList()) {
        Scenario obj;
        reader.jsonToValue(objJson, obj);
        m_scenarios.push_back(std::move(obj));
    }
}

void H3CCampaign::PrologEpilog::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_enabled;
    if (!m_enabled)
        return;
    stream >> m_video >> m_music >> m_text;
}

void H3CCampaign::PrologEpilog::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_enabled;
    if (!m_enabled)
        return;
    stream << m_video << m_music << m_text;
}

void H3CCampaign::Scenario::readBinary(ByteOrderDataStreamReader& stream)
{
    streamTrace(stream, "Scenario start");
    stream >> m_filename >> m_packedMapSize;

    // todo: check somehow gzip segment before? m_packedMapSize
    m_preconditions.resize(m_totalScenarios);
    stream.readBits(m_preconditions, false);
    stream >> m_regionColor >> m_difficulty >> m_regionText;

    streamTrace(stream, "After regionText");
    stream >> m_prolog;

    PrologEpilog fakeProlog;
    if (m_version >= static_cast<uint32_t>(CampaignVersion::HotA17)) {
        stream >> fakeProlog; // hota has up to 3 prolog/epilog
        stream >> fakeProlog;
    }

    streamTrace(stream, "After prolog");
    stream >> m_epilog;
    if (m_version >= static_cast<uint32_t>(CampaignVersion::HotA17)) {
        stream >> fakeProlog;
        stream >> fakeProlog;
    }

    streamTrace(stream, "After epilog");
    readScenarioTravel(stream, static_cast<CampaignVersion>(m_version), m_index);
    streamTrace(stream, "Scenario end");
}

void H3CCampaign::Scenario::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_filename << m_packedMapSize;
    stream.writeBits(m_preconditions, false);
    stream << m_regionColor << m_difficulty << m_regionText;
    stream << m_prolog << m_epilog;
    assert("make readScenarioTravel write version");
}

}
