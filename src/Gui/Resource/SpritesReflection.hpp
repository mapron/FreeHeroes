/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Sprites.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

namespace Mernel::Reflection {
using namespace ::FreeHeroes::Gui;

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    ::FreeHeroes::PixmapPoint,
    m_x,
    m_y)

STRUCT_REFLECTION_PAIRED(
    ::FreeHeroes::PixmapSize,
    "w",
    m_width,
    "h",
    m_height)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    Sprite,
    m_boundarySize,
    m_mask,
    m_groups)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    Sprite::FrameImpl,
    m_padding,
    m_hasBitmap,
    m_bitmapSize,
    m_bitmapOffset,
    m_boundarySize)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    ISprite::SpriteSequenceParams,
    m_scaleFactorPercent,
    m_animationCycleDuration,
    m_specialFrameIndex,
    m_actionPoint)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    ISprite::SpriteSequenceMask,
    m_width,
    m_height,
    m_draw1,
    m_draw2)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    Sprite::Group,
    m_params,
    m_groupId,
    m_frames)

}
