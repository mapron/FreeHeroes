/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureSquad.hpp"
#include "AdventureHero.hpp"

namespace FreeHeroes::Core {

struct AdventureArmy {
    AdventureSquad squad;
    AdventureHero hero;

    bool isEqualTo(const AdventureArmy & another) const noexcept {
        if (!hero.isEqualTo(another.hero))
            return false;

        if (!squad.isEqualTo(another.squad))
            return false;

        return true;
    }

    bool isEmpty() const noexcept { return squad.isEmpty();}

    bool hasHero() const noexcept { return hero.isValid(); }

    struct EstimatedParams {
        ResourceAmount weekIncomeMax;
        ResourceAmount dayIncome;
    };
    EstimatedParams estimated;
};


}
