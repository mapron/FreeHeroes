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
    enum class Type {None, Income, Unit, UnitNonStd, UnitUpgrade, Skill, Spell,
                     SpecialBallista, SpecialCannon, SpecialDragons, SpecialSpeed };
    struct Presentation {
        std::string icon;
    };

    std::string id;
    std::string untranslatedName;


    Type type;
    ResourceAmount dayIncome;

    LibraryUnitConstPtr unit = nullptr;

    LibrarySecondarySkillConstPtr skill = nullptr;

    LibrarySpellConstPtr spell = nullptr;

    Presentation presentationParams;
};

}
