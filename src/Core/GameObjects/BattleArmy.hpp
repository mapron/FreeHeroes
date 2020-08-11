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
    BattleArmy() = default;

    explicit BattleArmy(AdventureArmyConstPtr adventure, BattleStack::Side side)
        : adventure(adventure) {

        if (adventure->hasHero()) {
            battleHero = BattleHero(&adventure->hero);
        }
        squad = std::make_unique<BattleSquad>(&adventure->squad, battleHero.isValid() ? &battleHero : nullptr, side);
    }
    void updateAdventure(AdventureArmy & adventure, const BattleSquad::LossInformation & opponentLoss, bool isWin) {
        squad->updateAdventure(adventure.squad);

        if (battleHero.isValid()) {
            adventure.hero.mana = battleHero.mana;
            if (isWin)
                adventure.hero.experience += opponentLoss.totalHpLoss;
        }
    }
    bool isEmpty() const noexcept { return squad->stacks.empty();}
    bool hasAlive() const noexcept {
        for (auto & stack : squad->stacks) {
            if (stack.count > 0)
                return true;
        }
        return false;
    }
    AdventureArmyConstPtr adventure = nullptr;

    std::unique_ptr<BattleSquad> squad;
    BattleHero battleHero;
};

}
