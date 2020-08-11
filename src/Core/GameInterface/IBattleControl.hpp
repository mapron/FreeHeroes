/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattlePlanMove.hpp"
#include "BattlePlanCast.hpp"

namespace FreeHeroes::Core {

class IBattleControl
{
public:
    virtual ~IBattleControl() = default;

    virtual bool doGuard() = 0;
    virtual bool doWait() = 0;
    virtual bool doMoveAttack(BattlePlanMoveParams moveParams, BattlePlanAttackParams attackParams) = 0;
    virtual bool doCast(BattlePlanCastParams planParams) = 0;

};

}
