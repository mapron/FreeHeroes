/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiUtilsExport.hpp"

#include "MernelPlatform/FsUtils.hpp"

namespace FreeHeroes {

GUIUTILS_EXPORT Mernel::std_path findHeroes3Installation(bool hotaAllowed = true) noexcept;

}
