/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AI.hpp"

#include "BattleStack.hpp"

#include "Profiler.hpp"
#include "Logger.hpp"

#include <sstream>

namespace FreeHeroes::Core {

static std::ostream& operator<<(std::ostream& os, const BattlePosition& pos)
{
    return os << "(" << pos.x << ", " << pos.y << ")"; // whatever needed to print bar to os
}

namespace {
struct AttackVariant {
    BattleDirection innerDirection;
    bool            rightCell;
};
using AttackVariantList = std::vector<AttackVariant>;

}

AI::AI(const AIParams& params, IBattleControl& battleControl, IBattleView& battleView, BattleFieldGeometry geometry)
    : m_params(params)
    , m_battleControl(battleControl)
    , m_battleView(battleView)
    , m_field(geometry)
{
}

int AI::run(int stepLimit)
{
    ProfilerDefaultContextSwitcher switcher(m_profileContext);
    ProfilerScope                  mainScope("AI");
    while (!m_battleView.isFinished() && stepLimit-- > 0) {
        runStep();
    }
    return stepLimit;
}

void AI::runStep()
{
    ProfilerDefaultContextSwitcher switcher(m_profileContext);
    m_stepData.clear();
    ProfilerScope mainScope("step");
    m_stepData.m_current    = m_battleView.getActiveStack();
    m_stepData.isWide       = m_stepData.m_current->library->traits.large;
    m_stepData.currentSpeed = m_stepData.m_current->current.primary.battleSpeed;

    const auto availableActions      = m_battleView.getAvailableActions();
    m_stepData.m_rangeAttackAvaiable = availableActions.rangeAttack;

    std::vector<BattleStackConstPtr> all = m_battleView.getAllStacks(true);
    m_stepData.m_opponent.reserve(all.size());
    for (auto stack : all) {
        if (stack->side != m_stepData.m_current->side) {
            OpponentStack os;
            os.m_stack = stack;
            m_stepData.m_opponent.push_back(os);
        }
    }
    if (m_stepData.m_opponent.empty()) { // WTF?
        m_battleControl.doGuard();
        return;
    }
    if (m_stepData.m_rangeAttackAvaiable) {
        makeRangedAttack();
        return;
    }

    prepareReachable();

    if (!m_stepData.m_possibleAttacks.empty()) {
        makeMeleeAttack();
        return;
    }

    if (findWaitReachable()) {
        m_battleControl.doWait();
        return;
    }

    if (makeMoveToClosestTarget()) {
        return;
    }

    m_battleControl.doGuard();
}

void AI::clearProfiling()
{
    m_profileContext.clearAll();
}

std::string AI::getProfiling() const
{
    std::ostringstream os;
    os << m_profileContext.printToStr();
    return os.str();
}

void AI::prepareReachable()
{
    ProfilerScope scope("prep Reachable");
    //    m_stepData.m_reachableNow.reserve(m_stepData.m_opponent.size());
    //    m_stepData.m_reachableNotNow.reserve(m_stepData.m_opponent.size());

    m_stepData.reachNow   = m_battleView.findDistances(m_stepData.m_current, m_stepData.currentSpeed);
    m_stepData.reachAtAll = m_battleView.findDistances(m_stepData.m_current, -1);
    //const auto & field =

    for (OpponentStack& opp : m_stepData.m_opponent) {
        opp.m_isWide                  = opp.m_stack->library->traits.large;
        const auto& attackVariantList = BattlePositionExtended::getAttackSuggestions(m_stepData.isWide, opp.m_isWide);
        for (const auto& attackVariant : attackVariantList) {
            BattlePlanMoveParams   moveParams;
            BattlePlanAttackParams attackParams;
            attackParams.m_attackTarget    = opp.m_stack->pos.specificPos(attackVariant.second);
            attackParams.m_attackDirection = attackVariant.first;

            auto currentExtPos    = m_stepData.m_current->pos;
            auto attackFromPos    = m_field.suggestPositionForAttack(currentExtPos, opp.m_stack->pos, opp.m_stack->pos.getPosSub(attackVariant.second), attackParams.m_attackDirection);
            moveParams.m_movePos  = attackFromPos;
            moveParams.m_moveFrom = m_stepData.m_current->pos;

            auto itNow         = m_stepData.reachNow.find(attackFromPos.mainPos());
            int  distanceNow   = itNow == m_stepData.reachNow.cend() ? -1 : itNow->second;
            auto itMaybe       = m_stepData.reachAtAll.find(attackFromPos.mainPos());
            int  distanceMaybe = itMaybe == m_stepData.reachAtAll.cend() ? -1 : itMaybe->second;

            if (attackFromPos == m_stepData.m_current->pos) {
                distanceNow   = 0;
                distanceMaybe = 0;
            }

            if (distanceMaybe < 0)
                continue;

            int64_t value = 0;
            {
                BattlePlanMoveParams moveParamsTmp = moveParams;
                moveParamsTmp.m_noMoveCalculation  = true;
                auto plan                          = m_battleView.findPlanMove(moveParamsTmp, attackParams);
                value                              = calculateValueForAttackPlan(plan);
            }

            Try t;
            t.stack            = opp.m_stack;
            t.moveParams       = moveParams;
            t.attackParams     = attackParams;
            t.distanceCells    = distanceMaybe;
            t.distanceTurns    = (t.distanceCells - 1) / m_stepData.currentSpeed;
            t.meleeAttackValue = value;

            m_stepData.m_possibleAttacksNotNow.push_back(t);

            // opp.m_possibleAttacksNoLimit.push_back({moveParams, attackParams, distanceMaybe, value});

            if (distanceNow < 0)
                continue;

            t.distanceCells = distanceNow;
            t.distanceTurns = (t.distanceCells - 1) / m_stepData.currentSpeed;
            assert(t.distanceTurns == 0);

            opp.m_turnsToReach = 0;
            m_stepData.m_possibleAttacks.push_back(t);
        }
    }
}

int64_t AI::calculateValueForRanged(BattleStackConstPtr stack)
{
    BattlePlanMoveParams   moveParams{ m_stepData.m_current->pos, m_stepData.m_current->pos };
    BattlePlanAttackParams attackParams{ stack->pos.mainPos() };

    BattlePlanMove plan = m_battleView.findPlanMove(moveParams, attackParams);
    return calculateValueForAttackPlan(plan);
}

int64_t AI::calculateValueForAttackPlan(const BattlePlanMove& planResult)
{
    const auto    rollMain               = planResult.m_mainDamage;
    const auto    retaliate              = planResult.m_retaliationDamage;
    const int64_t attackerValue          = planResult.m_attacker->library->value;
    const int64_t defenderValue          = planResult.m_defender->library->value;
    int64_t       damageEstimateByKills  = 0;
    int64_t       damageEstimateByDamage = 0;
    auto          deathWeight            = m_params.mainKillsWeight;
    if (rollMain.avgRoll.loss.remainCount == 0)
        deathWeight *= m_params.fullKillsMultiply;
    damageEstimateByKills += defenderValue * rollMain.avgRoll.loss.deaths * deathWeight;
    damageEstimateByDamage += defenderValue * rollMain.avgRoll.loss.damageTotal * m_params.mainDamageWeight / planResult.m_defender->current.primary.maxHealth;

    if (retaliate.isValid) {
        damageEstimateByKills += attackerValue * retaliate.avgRoll.loss.deaths * m_params.retaliationKillsWeight;
        damageEstimateByDamage += attackerValue * retaliate.avgRoll.loss.damageTotal * m_params.retaliationDamageWeight / planResult.m_attacker->current.primary.maxHealth;
    }
    for (auto& extra : planResult.m_extraAffectedTargets) {
        const int64_t extraValue  = extra.stack->library->value;
        auto          deathWeight = m_params.mainKillsWeight * m_params.extraKillsMultiply;
        auto          dmgWeight   = m_params.mainDamageWeight * m_params.extraKillsMultiply;
        if (extra.damage.avgRoll.loss.remainCount == 0)
            deathWeight *= m_params.fullKillsMultiply;
        damageEstimateByKills += extraValue * extra.damage.avgRoll.loss.deaths * deathWeight;
        damageEstimateByDamage += extraValue * extra.damage.avgRoll.loss.damageTotal * dmgWeight / extra.stack->current.primary.maxHealth;
    }
    for (auto& extraRet : planResult.m_extraRetaliationAffectedTargets) {
        const int64_t extraValue = extraRet.stack->library->value;
        damageEstimateByKills += extraValue * retaliate.avgRoll.loss.deaths * m_params.retaliationKillsWeight;
        damageEstimateByDamage += extraValue * retaliate.avgRoll.loss.damageTotal * m_params.retaliationDamageWeight / extraRet.stack->current.primary.maxHealth;
    }
    const DamageResult potentialDamageBeforeMove = m_battleView.estimateAvgDamageFromOpponentAfterMove(m_stepData.m_current, m_stepData.m_current->pos.mainPos());
    const DamageResult potentialDamageAfterMove  = m_battleView.estimateAvgDamageFromOpponentAfterMove(m_stepData.m_current, planResult.m_moveTo.mainPos());
    if (potentialDamageBeforeMove.loss.damageTotal != potentialDamageAfterMove.loss.damageTotal) {
        // @todo : apply to value with some coeff.
    }

    int64_t damageEstimateFinal = damageEstimateByKills ? damageEstimateByKills : damageEstimateByDamage;

    //    Logger() << "main: d " << rollMain.avgRoll.loss.damageTotal << " k " << rollMain.avgRoll.loss.deaths
    //             <<" ret: d " << retaliate.avgRoll.loss.damageTotal << " k " << retaliate.avgRoll.loss.deaths
    //            << " => " << damageEstimateFinal;
    return damageEstimateFinal;
}

bool AI::findWaitReachable()
{
    if (m_stepData.m_current->roundState.waited)
        return false;

    ProfilerScope scope("prep WaitReachable");

    for (auto& t : m_stepData.m_possibleAttacksNotNow) {
        if (t.distanceTurns == 1 && m_stepData.currentSpeed > t.stack->current.primary.battleSpeed)
            return true;
    }
    return false;
}

void AI::makeRangedAttack()
{
    ProfilerScope scope("make Ranged");
    Logger() << "makeRangedAttack, possibilities:";
    for (auto& opp : m_stepData.m_opponent) {
        opp.m_rangedAttackValue = calculateValueForRanged(opp.m_stack);
        Logger() << opp.m_stack->library->id << " at " << opp.m_stack->pos.mainPos() << ", value=" << opp.m_rangedAttackValue;
    }

    auto it = std::max_element(m_stepData.m_opponent.cbegin(), m_stepData.m_opponent.cend(), [](const OpponentStack& l, const OpponentStack& r) {
        return l.m_rangedAttackValue < r.m_rangedAttackValue;
    });

    BattlePlanMoveParams   planParams{ m_stepData.m_current->pos, m_stepData.m_current->pos };
    BattlePlanAttackParams attackParams{ it->m_stack->pos.mainPos() };

    Logger() << " => " << it->m_stack->library->id << " at " << it->m_stack->pos.mainPos() << ", value=" << it->m_rangedAttackValue;

    m_battleControl.doMoveAttack(planParams, attackParams);
}

void AI::makeMeleeAttack()
{
    ProfilerScope scope("make Melee");
    Logger() << "makeMeleeAttack, possibilities:";
    for (auto& t : m_stepData.m_possibleAttacks) {
        Logger() << t.stack->library->id << " attack from " << t.moveParams.m_movePos.mainPos() << ", value=" << t.meleeAttackValue;
    }
    auto       it = std::max_element(m_stepData.m_possibleAttacks.cbegin(), m_stepData.m_possibleAttacks.cend(), [this](const Try& l, const Try& r) {
        const int lHasCurrentPos = l.moveParams.m_movePos == m_stepData.m_current->pos;
        const int rHasCurrentPos = r.moveParams.m_movePos == m_stepData.m_current->pos;

        if (l.meleeAttackValue != r.meleeAttackValue)
            return l.meleeAttackValue < r.meleeAttackValue;

        if (lHasCurrentPos != rHasCurrentPos) {
            return lHasCurrentPos < rHasCurrentPos;
        }
        return l.distanceCells > r.distanceCells;
    });
    const Try& t  = *it;
    Logger() << " => " << t.stack->library->id << " attack from " << t.moveParams.m_movePos.mainPos() << ", value=" << t.meleeAttackValue;
    ProfilerScope scope1("doMoveAttack");
    m_battleControl.doMoveAttack(t.moveParams, t.attackParams);
}

bool AI::makeMoveToClosestTarget()
{
    ProfilerScope scope("make MoveToClosest");
    if (m_stepData.m_possibleAttacksNotNow.empty()) {
        return false;
    }
    Logger() << "makeMoveToClosestTarget, possibilities:";
    for (auto& t : m_stepData.m_possibleAttacks) {
        Logger() << t.stack->library->id << " attack from " << t.moveParams.m_movePos.mainPos() << ", value=" << t.meleeAttackValue << ", distance=" << t.distanceCells;
    }

    auto       it = std::max_element(m_stepData.m_possibleAttacksNotNow.cbegin(), m_stepData.m_possibleAttacksNotNow.cend(), [](const Try& l, const Try& r) {
        if (l.distanceTurns != r.distanceTurns)
            return l.distanceTurns > r.distanceTurns;

        if (l.meleeAttackValue != r.meleeAttackValue)
            return l.meleeAttackValue < r.meleeAttackValue;
        return l.distanceCells > r.distanceCells;
    });
    const Try& t  = *it;
    Logger() << " => " << t.stack->library->id << " attack from " << t.moveParams.m_movePos.mainPos() << ", value=" << t.meleeAttackValue << ", distance=" << t.distanceCells;

    auto failedPathParams                     = t.moveParams;
    failedPathParams.m_calculateUnlimitedPath = true;
    auto               plan                   = m_battleView.findPlanMove(failedPathParams, {});
    BattlePositionPath planFailedPath         = plan.m_walkPath;
    if (planFailedPath.empty())
        return false;

    for (int i = m_stepData.currentSpeed + 1; i >= 0; --i) {
        if (i >= (int) planFailedPath.size())
            continue;
        BattlePosition       closestPossiblePos = planFailedPath[i];
        BattlePlanMoveParams planParams{ this->m_stepData.m_current->pos.moveMainTo(closestPossiblePos), m_stepData.m_current->pos };
        BattlePlanMove       plan = m_battleView.findPlanMove(planParams, {});
        if (!plan.isValid())
            continue;
        m_battleControl.doMoveAttack(planParams, {});
        return true;
    }
    return false;
}

}
