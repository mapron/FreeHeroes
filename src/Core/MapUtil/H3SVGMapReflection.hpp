/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "H3SVGMap.hpp"

#include "MernelReflection/EnumTraits.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

// clang-format off
STRUCT_REFLECTION_PAIRED(
    H3SVGMap,
    "format",              m_format,
    "anyPlayers",          m_anyPlayers,
    "verMajor",            m_versionMajor,
    "verMinor",            m_versionMinor,
    "mapName",             m_mapName,
    "mapDescr",            m_mapDescr,
    "tiles",               m_tiles
)

// clang-format on
}
