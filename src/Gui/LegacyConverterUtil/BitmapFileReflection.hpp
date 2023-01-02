/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BitmapFile.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

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

STRUCT_REFLECTION_PAIRED(
    BitmapFile::RLEItemRaw,
    "data",
    m_bytes)

STRUCT_REFLECTION_PAIRED(
    BitmapFile::RLEItemNorm,
    "l",
    m_length,
    "v",
    m_value)

STRUCT_REFLECTION_PAIRED(
    BitmapFile::RLEItem,
    "isRaw",
    m_isRaw,
    "c",
    m_isCompressedLength,
    "r",
    m_raw,
    "n",
    m_norm)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::RLERow,
    m_items,
    m_width)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile::RLEData,
    m_rle1offsets,
    m_rle2offsets,
    m_rle3offsets,
    m_rleRows,
    m_originalSize)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    BitmapFile,
    m_compression,
    m_compressionOriginal,
    m_pixFormat,
    m_width,
    m_height,
    m_inverseRowOrder,
    m_palette,
    m_rows,
    m_rleData)

}
