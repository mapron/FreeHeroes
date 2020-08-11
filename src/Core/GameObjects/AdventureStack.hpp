/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryUnit.hpp"

#include "AdventureFwd.hpp"

namespace FreeHeroes::Core {


struct AdventureStack {
    AdventureStack() = default;
    AdventureStack(LibraryUnitConstPtr library, int count)
        : library(library), count(count) { if (!library) this->count = 0;  }

    bool isValid() const { return count > 0 && library; }

    // main params
    LibraryUnitConstPtr library = nullptr;
    int count = 0;

    bool isEqualTo(const AdventureStack & another) const noexcept { return library == another.library && count == another.count;}

    // estimation input:
    struct ArmyParams {
        int                      indexInArmy      = 0; // that's error-prone, probably, need to sure indeces are unique. Whatever.
        int                      indexInArmyValid = 0; // same.
        int                      indexInArmySparse = 0; // same.
    };
    ArmyParams armyParams;

    // estimated params:
    struct EstimatedSquadBonus {
        int undeadBonus = 0;
        PrimaryRngParams   rngParamsBonus;
        PrimaryRngParams   rngParamsOpponent;
    };
    EstimatedSquadBonus estimatedSquadBonus;

    struct EstimatedParams {
        UnitPrimaryParams    primary;    // library->primary + hero->primary.
        PrimaryRngParams         rngParams;  // (if hero) hero(skills+artifacts) + squad(abilities,factions,undead)
        bool                     hasMorale = false;
        MoraleDetails            moraleDetails;
        LuckDetails              luckDetails;
        MagicReduce              magicReduce;
        BonusRatio               magicOppSuccessChance = {1,1};
        SpellFilter              immunes;
        SpellFilter              immunesWithoutBreakable;
    };
    EstimatedParams estimated;

};


}
