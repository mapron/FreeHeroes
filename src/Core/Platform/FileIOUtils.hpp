/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CorePlatformExport.hpp"
#include "FsUtils.hpp"

#include <filesystem>

namespace FreeHeroes::Core {

COREPLATFORM_EXPORT bool readFileIntoBuffer(const std_path& filename, std::string& buffer);
COREPLATFORM_EXPORT bool writeFileFromBuffer(const std_path& filename, const std::string& buffer);

}
