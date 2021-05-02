/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>
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
    };

    std::string id;
    std::string untranslatedName;

    int frequencyFighter = 0;
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

    Presentation presentationParams;
};

}
