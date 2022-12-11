/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include "CorePlatformExport.hpp"

namespace FreeHeroes::Core {

COREPLATFORM_EXPORT bool showFolderInFileManager(const std_path& path);

COREPLATFORM_EXPORT bool createShortCut(const std_path&    fromDir,
                                        const std::string& fromName,
                                        const std_path&    toDir,
                                        const std::string& toFilename,
                                        const std::string& extraArgs);

}
