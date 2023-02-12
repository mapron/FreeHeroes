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
    enum class Type
    {
        Invalid,
        Upgrade,
        Exp,
        ExpGold,
        Support,
        Generator,
    };

    int         legacyId = -1;
    std::string id;

    Type type      = Type::Invalid;
    int  value     = 0;
    int  frequency = 0;

    ObjectDefMappings objectDefs;
};

}
