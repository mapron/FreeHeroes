/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Stat.hpp"

#include <vector>

namespace FreeHeroes::Core {

struct LibraryGameRules {
    struct RngRules {
        std::vector<BonusRatio> positiveChances;
        std::vector<BonusRatio> negativeChances;
        int maxEffectiveValue = +3;
        int minEffectiveValue = -3;
    };
    RngRules luck;
    RngRules morale;

    struct PhysicalConst {
        int maxEffectiveAttack  = 60;
        int maxEffectiveDefense = 28;
        BonusRatio attackValue  { 1, 20};
        BonusRatio defenseValue { 1, 40};
    };
    PhysicalConst physicalConst;

    struct Limits {
        int stacks = 7;
        int maxHeroLevel = 75;
        PrimaryAttackParams maxHeroAd;
        PrimaryMagicParams  maxHeroMagic;
        PrimaryAttackParams maxUnitAd;
    };

    Limits limits;
};

}
