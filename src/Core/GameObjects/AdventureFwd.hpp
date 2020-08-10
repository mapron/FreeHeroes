/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes::Core {

struct AdventureHero;
using AdventureHeroConstPtr = const AdventureHero*;
using AdventureHeroMutablePtr = AdventureHero*;

struct AdventureStack;
using AdventureStackConstPtr = const AdventureStack*;
using AdventureStackMutablePtr = AdventureStack*;

struct AdventureSquad;
using AdventureSquadConstPtr = const AdventureSquad*;
using AdventureSquadMutablePtr = AdventureSquad*;

struct AdventureArmy;
using AdventureArmyConstPtr = const AdventureArmy*;
using AdventureArmyMutablePtr = AdventureArmy*;

}
