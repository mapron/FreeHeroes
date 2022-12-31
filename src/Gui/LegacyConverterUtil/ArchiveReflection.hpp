/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Archive.hpp"

#include "Reflection/EnumTraitsMacro.hpp"
#include "Reflection/MetaInfoMacro.hpp"

namespace FreeHeroes::Core::Reflection {

ENUM_REFLECTION_STRINGIY(
    Archive::BinaryFormat,
    Invalid,
    Invalid,
    LOD,
    HDAT,
    SND,
    VID)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    Archive::Record,
    m_basename,
    m_originalBasename,
    m_extWithDot,
    m_originalExtWithDot,
    m_compressOnDisk,
    m_compressInArchive,
    m_uncompressedSizeCache,
    m_unknown1,
    m_filenameGarbage,
    m_binaryOrder,
    m_isPadding)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    Archive::HdatRecord,
    m_basename,
    m_filenameAlt,
    m_params,
    m_hasBlob)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    Archive,
    m_format,
    m_records,
    m_hdatRecords,
    m_lodFormat,
    m_lodHeader)

}
