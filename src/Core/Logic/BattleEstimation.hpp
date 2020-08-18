/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattleStack.hpp"
#include "BattleHero.hpp"
#include "BattleArmy.hpp"

#include "CoreLogicExport.hpp"

namespace sol {
class state;
}

namespace FreeHeroes::Core {

class CORELOGIC_EXPORT BattleEstimation {
public:

    BattleEstimation(LibraryGameRulesConstPtr rules) : m_rules(rules) {}

    void calculateArmyOnBattleStart(BattleArmy & army, const BattleArmy & opponent);
    void calculateArmyOnRoundStart(BattleArmy & army);

    void calculateUnitStats(BattleStack& unit);

    bool checkSpellTarget (const BattleStack& possibleTarget, LibrarySpellConstPtr spell);
    bool checkAttackElementPossibility(const BattleStack& possibleTarget, LibraryUnit::Abilities::AttackWithElement element);

private:
    void bindTypes(sol::state & lua);
    void calculateUnitStatsStartBattle(BattleStack& unit, const BattleSquad& squad, const BattleArmy& opponent);
    void calculateHeroStatsStartBattle(BattleHero& hero, const BattleSquad& squad, const BattleArmy& opponent);

private:
    LibraryGameRulesConstPtr m_rules;
};

}

