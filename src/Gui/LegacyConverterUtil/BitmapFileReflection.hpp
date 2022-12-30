/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BitmapFile.hpp"

#include "Reflection/EnumTraitsMacro.hpp"
#include "Reflection/MetaInfoMacro.hpp"

namespace FreeHeroes::Core::Reflection {

ENUM_REFLECTION_STRINGIY(
    BitmapFile::Compression,
    Invalid,
    Invalid,
    None,
    RLE1,
    RLE2,
    RLE3)

ENUM_REFLECTION_STRINGIY(
    BitmapFile::PixFormat,
    Invalid,
    Invalid,
    Gray,
    RGBA,
    BGRA,
    ARGB,
    ABGR)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::Pixel,
    m_r,
    m_g,
    m_b,
    m_alphaOrGray)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::Palette,
    m_table)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::RLEItem,
    m_segmentType,
    m_length,
    m_isRaw,
    m_raw)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::RLERow,
    m_rle0,
    m_items)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::RLEData,
    m_rle1offsets,
    m_rle2offsets,
    m_rle3offsets,
    m_rleRows,
    m_size)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile,
    m_compression,
    m_pixFormat,
    m_width,
    m_height,
    m_inverseRowOrder,
    m_palette,
    m_rows,
    m_rleData)

}
