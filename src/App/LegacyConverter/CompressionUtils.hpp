/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <iosfwd>

namespace FreeHeroes::Conversion {

int zlibUncompressFromBuffer(const char* sourceData, size_t remainSize, std::ostream & dest);

}
