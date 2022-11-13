/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#pragma once
#include "ByteBuffer.hpp"
#include "CorePlatformExport.hpp"

#include <vector>
#include <string>

namespace FreeHeroes::Core {

enum class CompressionType
{
    None,
    Gzip,
    ZStd, // unsupported yet.
};

struct CompressionInfo {
    CompressionType m_type  = CompressionType::None;
    int             m_level = 5;
};

COREPLATFORM_EXPORT void uncompressDataBuffer(const ByteArrayHolder& input, ByteArrayHolder& output, CompressionInfo compressionInfo);
COREPLATFORM_EXPORT void compressDataBuffer(const ByteArrayHolder& input, ByteArrayHolder& output, CompressionInfo compressionInfo);

}
