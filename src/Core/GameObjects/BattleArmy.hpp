/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureArmy.hpp"

#include "BattleHero.hpp"
#include "BattleSquad.hpp"

#include <memory>

namespace FreeHeroes::Core {

struct BattleArmy {
    explicit BattleArmy(AdventureArmyConstPtr adventure, BattleStack::Side side)
        : adventure(adventure)
        , side(side)
    {
        if (adventure->hasHero()) {
            battleHero = BattleHero(&adventure->hero);
        }
        squad = std::make_unique<BattleSquad>(&adventure->squad, battleHero.isValid() ? &battleHero : nullptr, side);
    }
    void updateAdventure(AdventureArmy& adventureArmy, const BattleSquad::LossInformation& opponentLoss, bool isWin)
    {
        squad->updateAdventure(adventureArmy.squad);

        if (battleHero.isValid()) {
            adventureArmy.hero.mana = battleHero.mana;
            if (isWin)
                adventureArmy.hero.experience += opponentLoss.totalHpLoss;
        }
    }
    bool isEmpty() const noexcept { return squad->stacks.empty(); }
    bool hasAlive() const noexcept
    {
        for (auto& stack : squad->stacks) {
            if (stack.count > 0)
                return true;
        }
        return false;
    }
    BattleStackMutablePtr summon(AdventureStackConstPtr advStack)
    {
        stacksSummon.emplace_back(advStack, battleHero.isValid() ? &battleHero : nullptr, side);
        return &stacksSummon.back();
    }
    BattleStackMutablePtr createMachineShoot(AdventureStackConstPtr stack)
    {
        machineShoot = std::make_unique<BattleStack>(stack, battleHero.isValid() ? &battleHero : nullptr, side);
        return machineShoot.get();
    }

    AdventureArmyConstPtr   adventure = nullptr;
    const BattleStack::Side side;

    std::unique_ptr<BattleSquad> squad;
    BattleHero                   battleHero;

    std::deque<BattleStack>      stacksSummon;
    std::unique_ptr<BattleStack> machineShoot;
};

}
