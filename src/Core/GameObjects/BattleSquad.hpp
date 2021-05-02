/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureSquad.hpp"

#include "BattleStack.hpp"
#include "BattleFwd.hpp"

namespace FreeHeroes::Core {

struct BattleSquad {
    BattleSquad() = default;
    struct LossItem {
        LibraryUnitConstPtr unit      = nullptr;
        int                 loss      = 0;
        bool                isDead    = false;
        int                 hpLoss    = 0;
        int                 valueLoss = 0;
    };

    struct LossInformation {
        std::vector<LossItem> units;
        int                   totalHpLoss    = 0;
        int                   totalValueLoss = 0;
        LibraryUnitConstPtr   strongestUnit  = nullptr;
    };

    explicit BattleSquad(AdventureSquadConstPtr adventure,
                         BattleHeroConstPtr     battleHero,
                         BattleStack::Side      side)
        : adventure(adventure)
    {
        stacks.reserve(adventure->stacks.size());
        for (const auto& advStack : adventure->stacks) {
            if (advStack.isValid())
                stacks.emplace_back(&advStack, battleHero, side);
        }
    }
    void updateAdventure(AdventureSquad& adventure)
    {
        for (AdventureStack& stack : adventure.stacks) {
            if (!stack.isValid())
                continue;
            int battleIndex = stack.armyParams.indexInArmyValid;
            if (stacks[battleIndex].count > 0)
                stack.count = stacks[battleIndex].count;
            else
                stack = {};
        }
    }
    LossInformation estimateLoss() const
    {
        LossInformation               info;
        std::set<LibraryUnitConstPtr> all;
        for (const AdventureStack& advStack : adventure->stacks) {
            if (!advStack.isValid())
                continue;
            all.insert(advStack.library);
            int   battleIndex = advStack.armyParams.indexInArmyValid;
            auto& battleStack = stacks[battleIndex];
            int   loss        = advStack.count - battleStack.count;
            if (!loss)
                continue;

            LossItem lossItem;
            lossItem.isDead    = !battleStack.isAlive();
            lossItem.unit      = advStack.library;
            lossItem.loss      = loss;
            lossItem.hpLoss    = advStack.estimated.primary.maxHealth * loss;
            lossItem.valueLoss = advStack.estimated.primary.maxHealth * loss;
            info.totalHpLoss += lossItem.hpLoss;
            info.totalValueLoss += lossItem.valueLoss;
            info.units.push_back(lossItem);
        }
        if (!all.empty())
            info.strongestUnit = *std::max_element(all.begin(), all.end(), [](LibraryUnitConstPtr l, LibraryUnitConstPtr r) { return l->value < r->value; });

        return info;
    }
    AdventureSquadConstPtr adventure = nullptr;

    std::vector<BattleStack> stacks;
};

}
