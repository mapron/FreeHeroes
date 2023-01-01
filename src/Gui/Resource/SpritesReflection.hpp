/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Sprites.hpp"

#include "Reflection/EnumTraitsMacro.hpp"
#include "Reflection/MetaInfoMacro.hpp"

namespace FreeHeroes::Core::Reflection {
using namespace ::FreeHeroes::Gui;

// clang-format off
template<>
inline constexpr const std::tuple MetaInfo::s_fields<QPoint>{
    SetGetLambda<QPoint, int>("x", [](QPoint& obj, int val) { obj.setX(val); }, [](const QPoint& obj) { return obj.x(); }),
    SetGetLambda<QPoint, int>("y", [](QPoint& obj, int val) { obj.setY(val); }, [](const QPoint& obj) { return obj.y(); }),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<QSize>{
    SetGetLambda<QSize, int>("w", [](QSize& obj, int val) { obj.setWidth(val);  }, [](const QSize& obj) { return obj.width(); }),
    SetGetLambda<QSize, int>("h", [](QSize& obj, int val) { obj.setHeight(val); }, [](const QSize& obj) { return obj.height(); }),
};
// clang-format on

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteNew,
    m_boundarySize,
    m_groups)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteNew::Frame,
    m_padding,
    m_hasBitmap,
    m_bitmapSize,
    m_bitmapOffset,
    m_boundarySize)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteNew::Params,
    m_scaleFactorPercent,
    m_animationCycleDuration,
    m_specialFrameIndex,
    m_actionPoint)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteNew::Group,
    m_params,
    m_groupId,
    m_frames)

}
