/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "Stat.hpp"

#include <variant>
#include <string>
#include <map>

namespace FreeHeroes::Core {

struct LibraryHeroSpec {
    // clang-format off
    enum class Type {None, Income, Unit, UnitNonStd, UnitUpgrade, Skill, Spell,
                     SpecialBallista, SpecialCannon, SpecialDragons, SpecialSpeed };
    // clang-format on
    struct Presentation {
        std::string icon;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    Type           type;
    ResourceAmount dayIncome;

    LibraryUnitConstPtr unit = nullptr;

    LibrarySecondarySkillConstPtr skill = nullptr;

    LibrarySpellConstPtr spell = nullptr;

    Presentation presentationParams;
};

}
