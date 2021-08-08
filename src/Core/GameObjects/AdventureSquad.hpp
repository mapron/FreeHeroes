/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureFwd.hpp"
#include "AdventureStack.hpp"
#include "LibraryResource.hpp"

#include <vector>
#include <deque>

namespace FreeHeroes::Core {

struct AdventureSquad {
    std::vector<AdventureStack> stacks;
    std::deque<AdventureStack>  stacksSummoned;
    bool                        useCompactFormation = false;

    bool isEqualTo(const AdventureSquad& another) const noexcept
    {
        if (stacks.size() != another.stacks.size())
            return false;
        for (size_t i = 0; i < stacks.size(); ++i) {
            if (!stacks[i].isEqualTo(another.stacks[i]))
                return false;
        }
        return true;
    }

    bool isEmpty() const noexcept { return stacks.empty(); }

    bool addWithMerge(LibraryUnitConstPtr unit, int count)
    {
        for (size_t i = 0; i < stacks.size(); ++i) {
            if (stacks[i].isValid() && stacks[i].library == unit) {
                stacks[i].count += count;
                return true;
            }
        }
        for (size_t i = 0; i < stacks.size(); ++i) {
            if (!stacks[i].isValid()) {
                stacks[i] = AdventureStack(unit, count);
                return true;
            }
        }
        return false;
    }

    AdventureStackMutablePtr summon(LibraryUnitConstPtr unit, int count)
    {
        stacksSummoned.push_back(AdventureStack(unit, count));
        return &stacksSummoned.back();
    }

    struct EstimatedParams {
        struct Bonus {
            PrimaryRngParams    rngParams;
            int                 manaCost = 0;
            RngChanceMultiplier rngMult;
        };

        Bonus          squadBonus;             // undead + factions + abilities  + (if)hero(skills+artifacts) + terrain
        Bonus          oppBonus;               // abilities                      + (if)hero(skills+artifacts)
        int            armySpeed          = 0; // min(unit.armySpeed)
        int            fastestBattleSpeed = 0; // max(unit.battleSpeed)
        MoraleDetails  moraleDetails;
        LuckDetails    luckDetails;
        ResourceAmount weekIncomeMax;
    };
    EstimatedParams estimated;
};

}
