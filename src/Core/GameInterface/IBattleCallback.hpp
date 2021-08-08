/*
 * Copyright (C) 2021 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureStack.hpp"
#include "BattleStack.hpp"

#include <functional>

namespace FreeHeroes::Core {

using BattleCallbackSummon = std::function<AdventureStackConstPtr(BattleStack::Side side, LibraryUnitConstPtr unit, int count)>;

}
