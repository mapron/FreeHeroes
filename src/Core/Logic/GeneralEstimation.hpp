/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CoreLogicExport.hpp"

#include "Stat.hpp"
#include "LibraryFwd.hpp"

#include <numeric>

namespace sol {
class state;
}

namespace FreeHeroes::Core {
class IRandomGenerator;
struct BattleStack;
struct SpellCastParams ;
class BonusRatio;


struct CORELOGIC_EXPORT GeneralEstimation {

    void bindTypes(sol::state & lua);


    int spellBaseDamage(int targetUnitLevel, const SpellCastParams & castParams, int targetIndex);


    int64_t getExperienceForLevel(int level);
    int getLevelByExperience(int64_t experience);

    enum class DamageRollMode { Random, Min, Max, Avg };

    BonusRatio calculatePhysicalBase(DamageDesc dmg, int count, DamageRollMode rollMode, IRandomGenerator &randomGenerator);
    std::pair<BonusRatio, BonusRatio> calculateAttackPower(int attackerAttack, int targetDefense);
};

}
