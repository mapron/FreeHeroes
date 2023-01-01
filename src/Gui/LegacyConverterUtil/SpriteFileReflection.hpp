/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BitmapFileReflection.hpp"

#include "SpriteFile.hpp"

namespace FreeHeroes::Core::Reflection {

ENUM_REFLECTION_STRINGIY(
    SpriteFile::BinaryFormat,
    Invalid,
    Invalid,
    DEF,
    DEF32,
    BMP,
    PCX)
ENUM_REFLECTION_STRINGIY(
    SpriteFile::DefType,
    Invalid,
    Invalid,
    BattleSpells,
    Sprite,
    BattleSprite,
    AdventureItem,
    AdventureHero,
    AdventureTerrain,
    Cursor,
    Interface,
    SpriteFrame,
    BattleHero)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteFile::Frame,
    m_paddingLeft,
    m_paddingTop,

    m_hasBitmap,
    m_bitmapIndex,
    m_bitmapWidth,
    m_bitmapHeight,
    m_bitmapOffsetX,
    m_bitmapOffsetY,

    m_bitmapFilename,
    m_bitmapFilenamePad,

    m_boundaryWidth,
    m_boundaryHeight,

    m_binaryOrder,
    m_isDuplicate,
    m_headerIndex,
    m_dupHeaderIndex,

    m_shortHeaderFormat)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteFile::Group,
    m_groupId,
    m_unk1,
    m_unk2,
    m_frames)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SpriteFile,
    m_format,
    m_defType,
    m_boundaryWidth,
    m_boundaryHeight,
    /*bitmaps*/
    m_groups,
    m_palette,
    m_tralilingData,
    m_embeddedBitmapData)

}
