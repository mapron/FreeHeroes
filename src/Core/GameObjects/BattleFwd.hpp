/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes::Core {

struct BattleStack;
using BattleStackConstPtr = const BattleStack*;
using BattleStackMutablePtr = BattleStack*;

struct BattleHero;
using BattleHeroMutablePtr = BattleHero*;
using BattleHeroConstPtr = const BattleHero*;

struct BattleArmy;
using BattleArmyConstPtr = const BattleArmy*;
using BattleArmyMutablePtr = BattleArmy*;

struct BattleSquad;
using BattleSquadConstPtr = const BattleSquad*;
using BattleSquadMutablePtr = BattleSquad*;

}
