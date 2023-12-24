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
    Invalid,
    Water,
    Ice,
    Mud,
    Lava)

ENUM_REFLECTION_STRINGIFY(
    FHRoadType,
    Invalid,
    Invalid,
    Dirt,
    Gravel,
    Cobblestone)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPos,
    m_x,
    m_y,
    m_z)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHZone,
    m_id,
    m_terrainId,
    m_tiles,
    m_tilesVariants,
    m_tilesFlippedHor,
    m_tilesFlippedVert,
    m_rect)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRiver,
    m_type,
    m_tiles,
    m_tilesVariants)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRoad,
    m_type,
    m_tiles,
    m_tilesVariants)
STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTileMap::ChangedTile,
    m_pos,
    m_orig)
STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTileMap,
    m_width,
    m_height,
    m_depth,
    m_changedViews,
    m_inverseFlipHor,
    m_inverseFlipVert)

}
