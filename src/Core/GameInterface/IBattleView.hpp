/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattlePlanMove.hpp"
#include "BattlePlanCast.hpp"
#include "BattleStack.hpp"
#include "BattleFieldPosition.hpp"

namespace FreeHeroes::Core {
class IBattleNotify;
class IBattleView
{
public:
    virtual ~IBattleView() = default;

    struct AvailableActions {
        bool baseUnitActions = false; // move/melee/cast
        bool move = false;
        bool rangeAttack = false;
        bool meleeAttack = false;
        bool splashAttack = false;
        bool cast = false;

        bool wait = false;
        bool guard = false;

        bool heroCast = false;

        std::vector<BattlePlanAttackParams::Alteration> alternatives;
    };

    virtual AvailableActions getAvailableActions() const = 0;

    virtual std::vector<BattleStackConstPtr> getAllStacks(bool alive) const = 0;
    virtual std::vector<BattlePosition> getObstacles() const = 0;
    virtual BattleHeroConstPtr getHero(BattleStack::Side side) const = 0;
    virtual BattleStack::Side getCurrentSide() const = 0;
    virtual BattleStackConstPtr getActiveStack() const = 0;

    virtual BattleStackConstPtr    findStack(const BattlePosition pos, bool onlyAlive = false) const= 0;
    virtual BattlePositionSet         findAvailable(BattleStackConstPtr stack) const = 0;
    virtual BattlePositionDistanceMap findDistances(BattleStackConstPtr stack, int limit) const = 0;
    virtual BattlePlanMove            findPlanMove(const BattlePlanMoveParams & moveParams, const BattlePlanAttackParams & attackParams) const = 0;
    virtual BattlePlanCast            findPlanCast(const BattlePlanCastParams & castParams) const = 0;

    virtual DamageResult estimateAvgDamageFromOpponentAfterMove(BattleStackConstPtr stack, BattlePosition newPos) const = 0;


    virtual void addNotify(IBattleNotify * handler) = 0;
    virtual void removeNotify(IBattleNotify * handler) = 0;

    virtual bool isFinished() const = 0;
};

}
