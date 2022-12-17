/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

struct LibraryMapObstacle {
    int         legacyId = -1;
    std::string id;

    LibraryObjectDefConstPtr mapObjectDef = nullptr;
};

}