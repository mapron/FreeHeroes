/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ViewSettings.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteRenderSettings,
    m_reserved)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpritePaintSettings,
    m_viewScalePercent,
    m_doubleScale,
    m_animateTerrain,
    m_animateObjects,
    m_grid,
    m_gridOnTop,
    m_blockMask,
    m_overlay,
    m_gridOpacity)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    ViewSettings,
    m_renderSettings,
    m_paintSettings,
    m_inspectByHover)

}
