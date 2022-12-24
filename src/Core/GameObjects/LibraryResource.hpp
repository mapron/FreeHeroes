/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryObjectDef.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

struct LibraryResource {
    struct Presentation {
        int         orderKingdom = 0;
        int         orderCommon  = 0;
        std::string icon;
        std::string iconLarge;
        std::string iconTrade;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    ObjectDefMappings objectDefs;
    ObjectDefMappings minesDefs;
    Presentation      presentationParams;
};

}
