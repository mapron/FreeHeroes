/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryObjectDef.hpp"
#include "MapScore.hpp"

#include <string>
#include <vector>
#include <cassert>

namespace FreeHeroes::Core {

struct LibraryMapVisitable {
    enum class VisitKind
    {
        Invalid,
        Normal,
        Pick,
        Remove, // lamp/prison
    };

    int         legacyId = -1;
    std::string id;

    ScoreAttr attr      = ScoreAttr::Invalid;
    int       value     = 0;
    int       frequency = 0;
    int       minZone   = -1;
    int       maxZone   = -1;
    VisitKind visitKind = VisitKind::Normal;

    ObjectDefMappings objectDefs;
};

}
