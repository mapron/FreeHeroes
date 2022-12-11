/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "H3SVGMap.hpp"

#include "Reflection/EnumTraits.hpp"
#include "Reflection/MetaInfo.hpp"

namespace FreeHeroes::Core::Reflection {

template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3SVGMap>{
    Field("format", &H3SVGMap::m_format),
    Field("anyPlayers", &H3SVGMap::m_anyPlayers),
    Field("verMajor", &H3SVGMap::m_versionMajor),
    Field("verMinor", &H3SVGMap::m_versionMinor),
    Field("mapName", &H3SVGMap::m_mapName),
    Field("mapDescr", &H3SVGMap::m_mapDescr),
    Field("tiles", &H3SVGMap::m_tiles),
};

}
