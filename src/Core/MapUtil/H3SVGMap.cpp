/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "H3SVGMap.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

#include "H3MMapReflection.hpp"
#include "H3SVGMapReflection.hpp"

#include <array>
#include <set>

namespace FreeHeroes {

namespace {
constexpr const std::string_view g_signature{ "H3SVG" };
}

H3SVGMap::H3SVGMap()
{
}

void H3SVGMap::readBinary(ByteOrderDataStreamReader& stream)
{
    std::string str;
    str.resize(g_signature.size());
    stream.readBlock(str.data(), g_signature.size());

    if (str != g_signature)
        throw std::runtime_error("Invalid H3 savegame provided, must start from " + str + " magic sequence.");

    stream.zeroPadding(3);
    stream >> m_versionMajor >> m_versionMinor;
    stream.zeroPadding(32);

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

    stream >> m_anyPlayers >> m_tiles.m_size >> m_tiles.m_hasUnderground;
    {
        auto lock = stream.setContainerSizeBytesGuarded(2);
        stream >> m_mapName;
        stream >> m_mapDescr;
    }
}

void H3SVGMap::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    stream.writeBlock(g_signature.data(), g_signature.size());
    stream.zeroPadding(3);
    stream << m_versionMajor << m_versionMinor;
    stream.zeroPadding(32);

    stream << static_cast<int32_t>(m_format);
    stream << m_anyPlayers << m_tiles.m_size << m_tiles.m_hasUnderground;
    {
        auto lock = stream.setContainerSizeBytesGuarded(2);
        stream << m_mapName;
        stream << m_mapDescr;
    }
}

void H3SVGMap::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void H3SVGMap::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

}
