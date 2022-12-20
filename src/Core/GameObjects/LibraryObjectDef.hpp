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

struct LibraryObjectDef {
    int                  legacyId = -1;
    std::string          id;
    std::string          defFile;
    std::vector<uint8_t> blockMap;
    std::vector<uint8_t> visitMap;
    std::vector<uint8_t> terrainsHard;
    std::vector<uint8_t> terrainsSoft;

    int objId    = -1;
    int subId    = -1;
    int type     = -1;
    int priority = -1;

    std::vector<LibraryObjectDefConstPtr> alternatives;
};

}
