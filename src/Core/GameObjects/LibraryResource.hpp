/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <string>

namespace FreeHeroes::Core {

struct LibraryResource {

    struct Presentation {
        int orderKingdom = 0;
        int orderCommon  = 0;
        std::string icon;
        std::string iconLarge;
        std::string iconTrade;
    };

    std::string id;
    std::string untranslatedName;
    Presentation presentationParams;
};


}
