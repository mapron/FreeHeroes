/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryObjectDef.hpp"

#include "Reward.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

// clang-format off
enum class FieldLayout { Standard, Object, Churchyard1, Churchyard2, Ruins, Spit };
enum class FieldLayoutPos { None = -1, TR = 0, R, BR, BL, L, TL, T, B };
// clang-format on

struct LibraryMapBank {
    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    FieldLayout fieldLayout = FieldLayout::Object;

    Rewards rewards;

    using Guards = std::vector<UnitWithCount>;

    struct Variant {
        std::string name;
        int         rewardIndex = 0;
        Guards      guards;
    };
    using Variants = std::vector<Variant>;
    Variants variants;

    struct Presentation {
        int order = 0;
    };
    Presentation      presentationParams;
    ObjectDefMappings objectDefs;
};

}
