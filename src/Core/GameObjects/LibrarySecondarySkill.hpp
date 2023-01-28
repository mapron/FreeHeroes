/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TranslationMap.hpp"

#include <array>
#include <vector>

namespace FreeHeroes::Core {

struct LibrarySecondarySkill {
    struct Presentation {
        struct LevelParams {
            std::string iconSmall;
            std::string iconMedium;
            std::string iconLarge;
        };
        std::array<LevelParams, 3> levels;
        int                        order = 0;

        TranslationMap name;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    int frequencyWarrior = 0;
    int frequencyMage    = 0;

    enum class HandlerType
    {
        Special,
        Stat,
        School,
        Wisdom
    };
    HandlerType              handler = HandlerType::Special;
    std::vector<std::string> calc;

    bool isWaterContent     = false;
    bool isEnabledByDefault = true;

    Presentation presentationParams;
};

}
