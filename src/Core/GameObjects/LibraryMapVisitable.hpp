/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryObjectDef.hpp"

#include <string>
#include <vector>
#include <cassert>

namespace FreeHeroes::Core {

struct LibraryMapVisitable {
    int         legacyId = -1;
    std::string id;

    ObjectDefMappings objectDefs;
};

}