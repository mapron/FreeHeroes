/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TranslationMap.hpp"

#include <vector>

namespace FreeHeroes::Core {

struct LibraryBuilding {
    struct Presentation {
        int            order = -1;
        TranslationMap name;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    Presentation presentationParams;
};

}
