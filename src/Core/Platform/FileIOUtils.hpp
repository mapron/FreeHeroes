/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CorePlatformExport.hpp"
#include "FsUtils.hpp"
#include "ByteBuffer.hpp"

#include <filesystem>

namespace FreeHeroes::Core {

COREPLATFORM_EXPORT bool readFileIntoBuffer(const std_path& filename, std::string& buffer) noexcept(true);
COREPLATFORM_EXPORT bool writeFileFromBuffer(const std_path& filename, const std::string& buffer) noexcept(true);

COREPLATFORM_EXPORT bool readFileIntoHolder(const std_path& filename, ByteArrayHolder& holder) noexcept(true);
COREPLATFORM_EXPORT bool writeFileFromHolder(const std_path& filename, const ByteArrayHolder& holder) noexcept(true);

COREPLATFORM_EXPORT std::string readFileIntoBufferThrow(const std_path& filename) noexcept(false);
COREPLATFORM_EXPORT void        writeFileFromBufferThrow(const std_path& filename, const std::string& buffer) noexcept(false);

COREPLATFORM_EXPORT ByteArrayHolder readFileIntoHolderThrow(const std_path& filename) noexcept(false);
COREPLATFORM_EXPORT void            writeFileFromHolderThrow(const std_path& filename, const ByteArrayHolder& holder) noexcept(false);
}
