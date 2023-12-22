/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "H3CCampaign.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

#include "H3CCampaignReflection.hpp"

#include <array>
#include <set>

namespace FreeHeroes {

namespace {

enum CampaignVersion
{
    NONE = 0,
    RoE  = 4,
    AB   = 5,
    SoD  = 6,
    WoG  = 6,
    //		Chr = 7, // Heroes Chronicles, likely identical to SoD, untested

    VCMI     = 1,
    VCMI_MIN = 1,
    VCMI_MAX = 1,
};
}

H3CCampaign::H3CCampaign()
{
}

void H3CCampaign::readBinary(ByteOrderDataStreamReader& stream)
{
    stream >> m_version >> m_campId >> m_name >> m_description;
    if (m_version > CampaignVersion::RoE)
        stream >> m_difficultyChoosenByPlayer;
    stream >> m_musicId;

    {
        std::string             buffer = "Test 1";
        Mernel::ByteArrayHolder holder;
        holder.resize(buffer.size());
        memcpy(holder.data(), buffer.data(), holder.size());
        m_scenarios.push_back(Scenario{ .m_filename = "test1.h3m", .m_data = holder });
    }
    {
        std::string             buffer = "Test 2";
        Mernel::ByteArrayHolder holder;
        holder.resize(buffer.size());
        memcpy(holder.data(), buffer.data(), holder.size());
        m_scenarios.push_back(Scenario{ .m_filename = "test2.h3m", .m_data = holder });
    }
}

void H3CCampaign::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream << m_version << m_campId << m_name << m_description;
    if (m_version > CampaignVersion::RoE)
        stream << m_difficultyChoosenByPlayer;
    stream << m_musicId;
}

void H3CCampaign::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void H3CCampaign::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

}
