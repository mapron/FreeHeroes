/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTileMap.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

ENUM_REFLECTION_STRINGIFY(
    FHRiverType,
    Invalid,
    None,
    Water,
    Ice,
    Mud,
    Lava)

ENUM_REFLECTION_STRINGIFY(
    FHRoadType,
    Invalid,
    None,
    Dirt,
    Gravel,
    Cobblestone)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPos,
    m_x,
    m_y,
    m_z)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPackedTileMap::River,
    m_type,
    m_tiles,
    m_views,
    m_flipHor,
    m_flipVert)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPackedTileMap::Road,
    m_type,
    m_tiles,
    m_views,
    m_flipHor,
    m_flipVert)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPackedTileMap,
    m_terrains,
    m_tileTerrianIndexes,
    m_tileViews,
    m_terrainFlipHor,
    m_terrainFlipVert,
    m_coastal,
    m_rivers,
    m_roads)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTileMap,
    m_width,
    m_height,
    m_depth)

}
