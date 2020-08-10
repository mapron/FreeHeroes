/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureHero.hpp"

#include "BattleFwd.hpp"
#include "BattleStack.hpp"

namespace FreeHeroes::Core {

struct BattleHero {
    BattleHero() = default;
    explicit BattleHero(AdventureHeroConstPtr adventure)
        : library(adventure->library), adventure(adventure)
    { mana = adventure->mana; }

    bool isValid() const noexcept { return !!adventure;}

    LibraryHeroConstPtr library = nullptr; // for convenience and shorter code.
    AdventureHeroConstPtr adventure = nullptr;
    BattleStack::Side side = BattleStack::Side::Attacker;

    struct EstimatedParams {
        HeroPrimaryParams   primary;
        PrimaryRngParams    squadRngParams;
        AdventureHero::SpellList availableSpells; // sorted by order.
    };
    EstimatedParams estimated;

    int manaCost(LibrarySpellConstPtr spell) const {
        auto it = std::find_if(estimated.availableSpells.cbegin(), estimated.availableSpells.cend(),
                     [spell](const AdventureHero::SpellDetails & det) { return det.spell == spell; });
        if (it == estimated.availableSpells.cend())
            return -1;
        return it->manaCost;
    }

    int mana = 0;
    bool castedInRound = false;
};


}
