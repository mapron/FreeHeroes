/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleManager.hpp"

#include "AI.hpp"

#include "IBattleNotify.hpp"
#include "IRandomGenerator.hpp"

#include "BattleFieldPathFinder.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibrarySpell.hpp"
#include "LibraryGameRules.hpp"
#include "BattleEstimation.hpp"

#include "Logger.hpp"
#include "Profiler.hpp"

#include <iostream>
#include <algorithm>
#include <cassert>

namespace FreeHeroes::Core {

static std::ostream& operator<<(std::ostream& os, const FreeHeroes::Core::BattlePosition& pos) {
    return os << "(" << pos.x << ", " << pos.y << ")";
}

namespace {
const int rangedLimit = 10;

}

class BattleManager::BattleNotifyEach : public IBattleNotify {
    std::vector<IBattleNotify*> m_children;
public:
    BattleNotifyEach() = default;
    BattleNotifyEach(std::vector<IBattleNotify*> children) : m_children(std::move(children)) {}

    void addChild(IBattleNotify* child) { m_children.push_back(child); }
    void removeChild(IBattleNotify* child) {
        m_children.erase(std::remove(m_children.begin(), m_children.end(), child), m_children.end());
    }

    void beforeMove(BattleStackConstPtr stack, const BattlePositionPath & path) override { for (auto * child : m_children) child->beforeMove(stack, path); }
    void beforeAttackMelee(BattleStackConstPtr stack , const AffectedPhysical & affected, bool isRetaliation) override { for (auto * child : m_children) child->beforeAttackMelee(stack, affected, isRetaliation); }
    void beforeAttackRanged(BattleStackConstPtr stack, const AffectedPhysical & affected) override { for (auto * child : m_children) child->beforeAttackRanged(stack, affected); }
    void beforeWait(BattleStackConstPtr stack) override { for (auto * child : m_children) child->beforeWait(stack); }
    void beforeGuard(BattleStackConstPtr stack, int defBonus) override { for (auto * child : m_children) child->beforeGuard(stack, defBonus); }

    void onStackUnderEffect(BattleStackConstPtr stack, Effect effect) override  { for (auto * child : m_children) child->onStackUnderEffect(stack, effect); }
    void onCast(const Caster & caster, const AffectedMagic & affected, LibrarySpellConstPtr spell) override  { for (auto * child : m_children) child->onCast(caster, affected, spell); }


    void onPositionReset(BattleStackConstPtr stack) override { for (auto * child : m_children) child->onPositionReset(stack); }
    void onStartRound(int round) override { for (auto * child : m_children) child->onStartRound(round); }
    void onBattleFinished(BattleResult result) override { for (auto * child : m_children) child->onBattleFinished(result); }
    void onStateChanged() override { for (auto * child : m_children) child->onStateChanged(); }
    void onControlAvailableChanged(bool controlAvailable) override { for (auto * child : m_children) child->onControlAvailableChanged(controlAvailable); }
};

BattleManager::~BattleManager()
{
}

BattleManager::BattleManager(BattleArmy & attArmy, BattleArmy & defArmy,
                             const BattleFieldPreset & fieldPreset,
                             const std::shared_ptr<IRandomGenerator> & randomGenerator,
                             LibraryGameRulesConstPtr rules)
    : m_att(attArmy)
    , m_def(defArmy)
    , m_obstacles(fieldPreset.obstacles)
    , m_field(fieldPreset.field)
    , m_roundIndex(0)
    , m_notifiers(new BattleNotifyEach)
    , m_randomGenerator(randomGenerator)
    , m_rules(rules)
{
    makePositions(fieldPreset);

    initialParams();
}

void BattleManager::start()
{
    startNewRound();

    m_notifiers->onControlAvailableChanged(!m_battleFinished);
}


// =================================== View ===================================

IBattleView::AvailableActions BattleManager::getAvailableActions() const
{
    AvailableActions result;
    if (m_roundQueue.empty() || m_battleFinished)
        return result;
    auto currentStack  = m_current;
    auto hero = currentHero();
    result.heroCast = hero && !hero->castedInRound && hero->adventure->hasSpellBook && !hero->estimated.availableSpells.empty();
    result.move         = currentStack->current.canMove;        // @todo: battle machine, arrow tower
    result.meleeAttack  = currentStack->current.canAttackMelee; // @todo: battle machine
    result.rangeAttack  = currentStack->current.canAttackRanged     && !currentStack->current.rangeAttackIsBlocked;
    result.splashAttack = currentStack->current.canAttackFreeSplash && !currentStack->current.rangeAttackIsBlocked;
    result.cast         = currentStack->current.canCast;
    if (result.cast)
        result.possibleUnitCast = currentStack->current.fixedCast.params.spell;

    result.wait  = !currentStack->roundState.waited;
    result.guard = true;       // @todo:
    result.baseUnitActions = result.move || result.meleeAttack || result.rangeAttack || result.cast;
    if (result.rangeAttack && result.meleeAttack)
        result.alternatives.push_back(BattlePlanAttackParams::Alteration::ForceMelee);
    if (result.splashAttack)
        result.alternatives.push_back(BattlePlanAttackParams::Alteration::FreeAttack);
    return result;
}

std::vector<BattleStackConstPtr> BattleManager::getAllStacks(bool alive) const
{
    std::vector<BattleStackConstPtr> result(alive ? this->m_alive.size() : m_all.size());
    if (alive)
        std::copy(this->m_alive.cbegin(), this->m_alive.cend(), result.begin());
    else
        std::copy(this->m_all.cbegin(), this->m_all.cend(), result.begin());
    return result;
}

std::vector<BattlePosition> BattleManager::getObstacles() const
{
    return m_obstacles;
}

//const BattleField& BattleManager::getField() const { return m_field; }


BattleStackConstPtr BattleManager::findStack(const BattlePosition pos, bool onlyAlive) const
{
    const std::vector<BattleStackMutablePtr> & pointers = onlyAlive ? m_alive : m_all;

    auto it = std::find_if(pointers.begin(), pointers.end(), [&pos](auto * stack){ return stack->pos.contains(pos);});
    return it == pointers.end() ? nullptr : *it;
}

BattlePositionSet BattleManager::findAvailable(BattleStackConstPtr stack) const
{
    if (!stack->current.canMove)
        return BattlePositionSet();

    auto finder = this->setupFinder(stack);

    BattlePositionSet result = finder.findAvailable(stack->current.primary.battleSpeed);
    return result;
}

BattlePositionDistanceMap BattleManager::findDistances(BattleStackConstPtr stack, int limit) const
{
    auto finder = this->setupFinder(stack);

    BattlePositionDistanceMap result = finder.findDistances(limit);
    return result;
}

BattlePlanMove BattleManager::findPlanMove(const BattlePlanMoveParams & moveParams, const BattlePlanAttackParams & attackParams) const
{
    //ProfilerScope scope("BM::findPlanMove");
    auto stack = getActiveStack();

    BattlePlanMove result;
    result.m_moveFrom = stack->pos;
    result.m_moveTo   = moveParams.m_movePos;


    BattleStackConstPtr targetStack = attackParams.isActive() ? findStack(attackParams.m_attackTarget, true) : nullptr;
    result.m_freeAttack = attackParams.m_alteration == BattlePlanAttackParams::Alteration::FreeAttack;

    if (targetStack || result.m_freeAttack) {
        if (targetStack && stack->side == targetStack->side)
            return result;

        result.m_attackMode = BattlePlanMove::Attack::Melee;
        result.m_attackTarget = attackParams.m_attackTarget;
        result.m_attackDirection  = attackParams.m_attackDirection;

        if (attackParams.m_alteration != BattlePlanAttackParams::Alteration::ForceMelee && stack->current.canAttackRanged && !stack->current.rangeAttackIsBlocked) {
            result.m_moveTo = result.m_moveFrom;

            result.m_attackMode = BattlePlanMove::Attack::Ranged;
            if (targetStack)
                result.m_rangedAttackDenominator = rangedDenom(stack, targetStack);
            else
                result.m_rangedAttackDenominator = rangedDenom(stack, attackParams.m_attackTarget);
        } else {
            if (attackParams.m_alteration == BattlePlanAttackParams::Alteration::FreeAttack)
                return result;

            auto checkMove = m_field.suggestPositionForAttack(stack->pos,
                                                              targetStack->pos,
                                                              targetStack->pos.mainPos() == attackParams.m_attackTarget ? BattlePositionExtended::Sub::Main :  BattlePositionExtended::Sub::Secondary,
                                                              attackParams.m_attackDirection);
            if (checkMove != result.m_moveTo) {
                assert(!"Should not really happen from ui");
                return result;
            }
        }
        if (result.m_attackMode == BattlePlanMove::Attack::Melee && result.m_attackDirection == BattleAttackDirection::None)
            return result;

        result.m_defender = targetStack;
        result.m_attacker = stack;

        if (targetStack) {
            result.m_mainDamage        = estimateDamage(stack, targetStack, result.m_attackMode, result.m_rangedAttackDenominator);
            result.m_retaliationDamage = estimateRetaliationDamage(stack, targetStack, result.m_attackMode, result.m_mainDamage);
        }

        if (result.m_attackMode == BattlePlanMove::Attack::Melee
                && stack->library->abilities.hasMeleeSplash()) {
            result.m_splashPositions = getSplashExtraTargets(stack->library->abilities.splashType, result.m_moveTo, result.m_attackDirection);
            std::set<BattleStackConstPtr> splashAffected;
            for (auto pos : result.m_splashPositions) {
                BattleStackConstPtr targetCandidate = findStack(pos, true);
                if (!targetCandidate || targetCandidate == stack || targetCandidate == targetStack || splashAffected.contains(targetCandidate))
                    continue;
                if (!stack->library->abilities.splashFriendlyFire && targetCandidate->side == stack->side)
                    continue;

                splashAffected.insert(targetCandidate);
                BattlePlanMove::Target extraTarget;
                extraTarget.stack = targetCandidate;
                extraTarget.damage = estimateDamage(stack, targetCandidate, BattlePlanMove::Attack::Melee, 1);
                result.m_extraAffectedTargets.push_back(extraTarget);
            }
        }
        if (result.m_retaliationDamage.isValid
                && result.m_attackMode == BattlePlanMove::Attack::Melee
                && targetStack->library->abilities.hasMeleeSplash()) {
            result.m_splashRetaliationPositions = getSplashExtraTargets(targetStack->library->abilities.splashType, targetStack->pos, attackDirectionInverse(result.m_attackDirection));
            std::set<BattleStackConstPtr> splashAffected;
            for (auto pos : result.m_splashRetaliationPositions) {
                BattleStackConstPtr targetCandidate = findStack(pos, true);
                if (!targetCandidate || targetCandidate == stack || targetCandidate == targetStack || splashAffected.contains(targetCandidate))
                    continue;
                if (!targetStack->library->abilities.splashFriendlyFire && targetCandidate->side == targetStack->side)
                    continue;

                splashAffected.insert(targetCandidate);
                BattlePlanMove::Target extraTarget;
                extraTarget.stack = targetCandidate;
                extraTarget.damage = estimateDamage(targetStack, targetCandidate, BattlePlanMove::Attack::Melee, 1);
                result.m_extraRetaliationAffectedTargets.push_back(extraTarget);
            }
        }
        if (result.m_attackMode == BattlePlanMove::Attack::Ranged && stack->library->abilities.splashType == LibraryUnit::Abilities::SplashAttack::Ranged) {
            result.m_splashPositions = m_field.getAdjacentSet( attackParams.m_attackTarget );
            std::set<BattleStackConstPtr> splashAffected;
            for (auto pos : result.m_splashPositions) {
                BattleStackConstPtr targetCandidate = findStack(pos, true);
                if (!targetCandidate || targetCandidate == stack || targetCandidate == targetStack || splashAffected.contains(targetCandidate))
                    continue;
                if (!stack->library->abilities.splashFriendlyFire && targetCandidate->side == stack->side)
                    continue;

                if (!BattleEstimation(m_rules).checkAttackElementPossibility(*targetCandidate, stack->library->abilities.splashElement))
                    continue;

                splashAffected.insert(targetCandidate);
                BattlePlanMove::Target extraTarget;
                extraTarget.stack = targetCandidate;
                extraTarget.damage = estimateDamage(stack, targetCandidate, BattlePlanMove::Attack::Ranged, result.m_rangedAttackDenominator);
                result.m_extraAffectedTargets.push_back(extraTarget);
            }
        }
    }

    if (!m_field.isValid(result.m_moveTo.leftPos()) || !m_field.isValid(result.m_moveTo.rightPos()))
        return result;

    if (moveParams.m_noMoveCalculation)
        return result;

    if (stack->pos != result.m_moveTo) {
        if (!stack->current.canMove)
            return result;

        BattleFieldPathFinder finder = this->setupFinder(stack);
        result.m_walkPath = finder.fromStartTo(result.m_moveTo.mainPos(),
                            moveParams.m_calculateUnlimitedPath ? -1 : stack->current.primary.battleSpeed);
        if (result.m_walkPath.empty())
            return result;
    }
    result.m_isValid = true;
    assert(result.m_moveFrom == moveParams.m_moveFrom);
    return result;
}

BattlePlanCast BattleManager::findPlanCast(const BattlePlanCastParams & castParams) const
{
    BattlePlanCast result;

    result.m_castPosition = castParams.m_target;
    result.m_isValid = false;


    if (!m_field.isValid(castParams.m_target))
        return result;

    BonusRatio spellHeroIncreaseFactor {0,1};
    LibrarySpell::Range range = LibrarySpell::Range::Single;
    if (castParams.m_isHeroCast) {
        auto hero = currentHero();
        if (!hero)
            return result;

        assert(castParams.m_spell);
        result.m_spell       = castParams.m_spell;
        result.m_power.spell = castParams.m_spell;

        if (hero->manaCost(result.m_spell) > hero->mana) {
            assert(!"That should not happen.");
            return result;
        }

        int schoolLevel = hero->adventure->estimated.schoolLevels.getLevelForSpell(castParams.m_spell->school);
        spellHeroIncreaseFactor = hero->adventure->estimated.magicIncrease.getIncreaseForSpell(castParams.m_spell->school);

        if (schoolLevel < (int)result.m_spell->rangeByLevel.size())
            range = result.m_spell->rangeByLevel[schoolLevel];

        result.m_power.spellPower = hero->estimated.primary.magic.spellPower;
        result.m_power.durationBonus = hero->adventure->estimated.extraRounds;
        result.m_power.skillLevel = schoolLevel;
        result.m_power.heroSpecLevel = -1;
        if (hero->library->spec->spell == result.m_spell)
            result.m_power.heroSpecLevel = hero->adventure->level;
    } else if (castParams.m_isUnitCast){
        auto stack = getActiveStack();
        if (stack->current.fixedCast.count <=0) {
            assert(!"That should not happen.");
            return {};
        }

        result.m_power = stack->current.fixedCast.params;
        result.m_spell = result.m_power.spell;
    } else {
        return {};
    }
    const auto currentSide = getCurrentSide();
    std::vector<BattleStackConstPtr> affectedStackCandidates;
    if (range == LibrarySpell::Range::Chain4 || range == LibrarySpell::Range::Chain5) {
        std::set<BattleStackConstPtr> alive(m_alive.cbegin(), m_alive.cend());
        BattlePositionSet alivePositions;
        for (auto stack : alive)
            alivePositions.insert(stack->pos.mainPos());
        auto takeNextAlive = [&alive, &alivePositions](const BattlePosition pos) -> BattleStackConstPtr {
            auto it = std::find_if(alive.begin(), alive.end(), [&pos](auto * stack){ return stack->pos.contains(pos);});
            if (it !=  alive.end()) {
                auto stack = *it;
                assert(alivePositions.contains(stack->pos.mainPos()));
                alivePositions.erase(stack->pos.mainPos());
                alive.erase(stack);
                return stack;
            }
            return nullptr;
        };


        BattleStackConstPtr targetStack = takeNextAlive(castParams.m_target);
        if (!targetStack)
            return result;

        if (!BattleEstimation(m_rules).checkSpellTarget(*targetStack, result.m_spell))
            return result;

        size_t limit = range == LibrarySpell::Range::Chain4 ? 4 : 5;

        affectedStackCandidates.push_back(targetStack);
        BattlePosition currentPos = targetStack->pos.mainPos();
        while (!alive.empty()) {
            BattlePositionSet nextPossible = m_field.closestTo(currentPos, alivePositions);
            assert(!nextPossible.empty());
            if (nextPossible.empty())
                break;

            currentPos = *nextPossible.begin(); // @todo: check weird things like "goto opposite side first";
            auto stack = takeNextAlive(currentPos);
            if (!BattleEstimation(m_rules).checkSpellTarget(*stack, result.m_spell))
                continue;

            affectedStackCandidates.push_back(stack);
            assert(stack);
            if (!stack)
                break;
            if (affectedStackCandidates.size() >= limit)
                break;
        }
        for (auto stack: affectedStackCandidates)  {
            result.m_affectedArea.insert(stack->pos.mainPos());
            result.m_affectedArea.insert(stack->pos.secondaryPos());
        }
    } else {
        result.m_affectedArea = getSpellArea(castParams.m_target, range);
        std::set<BattleStackConstPtr> alreadyCovered;
        for (auto pos : result.m_affectedArea) {
            BattleStackConstPtr targetStack = findStack(pos, true);
            if (!targetStack || alreadyCovered.contains(targetStack))
                continue;

            alreadyCovered.insert(targetStack);
            if (result.m_spell->qualify == LibrarySpell::Qualify::Good) {
                if (currentSide != targetStack->side)
                    continue;
            }
            if (result.m_spell->qualify == LibrarySpell::Qualify::Bad
                    || (result.m_spell->type == LibrarySpell::Type::Offensive && !result.m_spell->indistinctive)) {
                if (currentSide == targetStack->side)
                    continue;
            }
            if (!BattleEstimation(m_rules).checkSpellTarget(*targetStack, result.m_spell))
                continue;

            affectedStackCandidates.push_back(targetStack);
        }
    }

    size_t affectedIndex = 0;
    for (BattleStackConstPtr targetStack : affectedStackCandidates) {

        BattlePlanCast::Target target;
        target.stack = targetStack;
        target.magicSuccessChance *= targetStack->current.magicOppSuccessChance;
        if (result.m_spell->type == LibrarySpell::Type::Offensive) {
            const int level = targetStack->library->level / 10;
            const int baseSpellDamage = std::max(1, GeneralEstimation(m_rules).spellBaseDamage(level, result.m_power, static_cast<int>(affectedIndex)));
            BonusRatio spellDamage(baseSpellDamage, 1);
            const BonusRatio spellDamageInit = spellDamage;
            spellDamage += spellDamageInit * spellHeroIncreaseFactor;
            if (   !targetStack->library->abilities.vulnerable.isDefault()
                 && targetStack->library->abilities.vulnerable.contains(result.m_spell))
                spellDamage += spellDamageInit * targetStack->library->abilities.vulnerableBonus;

            auto reduceFactor = targetStack->current.magicReduce.getReduceForSpell(result.m_spell->school);
            spellDamage *= reduceFactor;
            const int totalDamage = std::max(1, spellDamage.roundDownInt() );
            DamageResult::Loss loss = damageLoss(targetStack, totalDamage);

            target.loss = loss;
            target.totalFactor = spellDamage / spellDamageInit;
            result.lossTotal.damageTotal += loss.damageTotal;
            result.lossTotal.deaths      += loss.deaths;
        }
        result.m_targeted.push_back(target);
        affectedIndex++;
    }

    if (result.m_targeted.empty())
        return result;

    result.m_isValid = true;
    return result;
}

DamageResult BattleManager::estimateAvgDamageFromOpponentAfterMove(BattleStackConstPtr stack, BattlePosition newPos) const
{
    DamageResult result;
    (void)stack;
    (void)newPos;
    // @todo:
    return result;
}


BattleHeroConstPtr BattleManager::getHero(BattleStack::Side side) const
{
    auto * hero = (side == BattleStack::Side::Attacker ? &m_att.battleHero : &m_def.battleHero);
    return hero->isValid() ? hero : nullptr;
}

BattleStack::Side BattleManager::getCurrentSide() const
{
    assert(!m_roundQueue.empty());
    return m_current->side;
}

BattleStackConstPtr BattleManager::getActiveStack() const
{
    return m_current;
}

void BattleManager::addNotify(IBattleNotify* handler)
{
    m_notifiers->addChild(handler);
}

void BattleManager::removeNotify(IBattleNotify* handler)
{
    m_notifiers->removeChild(handler);
}


bool BattleManager::isFinished() const
{
    return m_battleFinished;
}

// ===================================  Control ===================================


bool BattleManager::doMoveAttack(BattlePlanMoveParams moveParams, BattlePlanAttackParams attackParams)
{
    assert(m_current);
    if (moveParams.m_calculateUnlimitedPath || moveParams.m_noMoveCalculation) {
        assert(!"Can't execute such a thing");
        return false;
    }

    BattleStackMutablePtr current = m_current;
    Logger(Logger::Info) << "doMoveAttack " << current->library->id << " from " << current->pos.mainPos()
                         << " to " << moveParams.m_movePos.mainPos() << ", hit direction " << int(attackParams.m_attackDirection);
    assert(current->pos == moveParams.m_moveFrom);

    const auto plan = findPlanMove(moveParams, attackParams);
    if (!plan.isValid()) {
        //(void)findPlanMove(moveParams, attackParams);
        assert(!"Invalid task for battle, that should be discarded earlier");
        return false;
    }
    ControlGuard guard(this);

    // @todo: check for sands/mines and probably throw different notify
    if (!plan.m_walkPath.empty())
        m_notifiers->beforeMove(current, plan.m_walkPath);



    current->pos.setMainPos(plan.m_moveTo.mainPos());
    current->roundState.finishedTurn = true;


    if (plan.m_attackMode == BattlePlanMove::Attack::Melee || plan.m_attackMode == BattlePlanMove::Attack::Ranged ) {
        assert(m_field.isValid(plan.m_attackTarget));
        auto mainTarget = findStackNonConst(plan.m_attackTarget, true);
        if (!plan.m_freeAttack) {
            assert(mainTarget);
            assert(mainTarget->side != current->side); // @todo: what about Hypnosis???
            assert(mainTarget->isAlive());
        }
        std::vector<BattleStackConstPtr> extraMain, extraRetaliate;
        for (auto target : plan.m_extraAffectedTargets)
            extraMain.push_back(target.stack);
        for (auto target : plan.m_extraRetaliationAffectedTargets)
            extraRetaliate.push_back(target.stack);

        if (plan.m_attackMode == BattlePlanMove::Attack::Ranged) {
            assert(current->current.canAttackRanged);
            makeRanged(current, plan.m_attackTarget, mainTarget, extraMain, plan.m_rangedAttackDenominator);
            if (current->current.maxAttacksRanged > 1 && current->current.canAttackRanged && (!mainTarget || mainTarget->isAlive()))
                makeRanged(current, plan.m_attackTarget, mainTarget, extraMain, plan.m_rangedAttackDenominator);
        } else {
            assert(current->current.canAttackMelee);
            makeMelee(current, mainTarget, extraMain, false);
            if (canRetaliate(current, mainTarget)) {
                mainTarget->roundState.retaliationsDone++;
                makeMelee(mainTarget, current, extraRetaliate, true);
            }
            if (current->current.maxAttacksMelee > 1 && current->current.canAttackMelee && (!mainTarget || mainTarget->isAlive())) {
                makeMelee(current, mainTarget, extraMain, false);
                if (canRetaliate(current, mainTarget)) {
                    mainTarget->roundState.retaliationsDone++;
                    makeMelee(mainTarget, current, extraRetaliate, true);
                }
            }
        }
        if (mainTarget)
            m_notifiers->onPositionReset(mainTarget);
    }
    m_notifiers->onPositionReset(current);

    if (current->count > 0 && !current->roundState.hadHighMorale) {
        const int morale = current->current.rngParams.morale;
        if (morale > 0 && checkRngRoll(m_current->current.moraleChance)) {
            current->roundState.hadHighMorale = true;
            current->roundState.finishedTurn = false;
            m_notifiers->onStackUnderEffect(current, IBattleNotify::Effect::GoodMorale);
        }
    }
    m_current = nullptr;
    updateState();
    return true;
}

bool BattleManager::doWait()
{
    assert(m_current);
    BattleStackMutablePtr current = m_current;
    if (current->roundState.waited)
        return false;

    Logger(Logger::Info) << "doWait " << current->library->id;


    ControlGuard guard(this);

    m_notifiers->beforeWait(current);

    current->roundState.waited = true;

    m_current = nullptr;
    updateState();
    return true;
}

bool BattleManager::doGuard()
{
    ControlGuard guard(this);
    assert(m_current);
    BattleStackMutablePtr current = m_current;

    Logger(Logger::Info) << "doGuard " << current->library->id;

    const int bonus = current->current.primary.ad.defense >= 5 ? current->current.primary.ad.defense / 5 : 1;
    m_notifiers->beforeGuard(current, bonus);


    current->roundState.finishedTurn = true;
    current->roundState.guardBonus = bonus;
    recalcStack(current);

    m_current = nullptr;
    updateState();
    return true;
}

bool BattleManager::doCast(BattlePlanCastParams planParams)
{
    assert(m_current);
    assert(m_field.isValid(planParams.m_target));

    const auto plan = findPlanCast(planParams);
    if (!plan.isValid()) {
        assert(!"Invalid task for battle, that should be discarded earlier");
        return false;
    }
    Logger(Logger::Info) << "doCast " << plan.m_spell->id;

    ControlGuard guard(this);
    IBattleNotify::Caster caster;
    if (planParams.m_isHeroCast) {

        auto hero = currentHero();
        assert(hero);

        const int manaCost = hero->manaCost(plan.m_spell);
        assert(hero->mana >= manaCost);
        hero->mana -= manaCost;
        hero->castedInRound = true;
        caster.hero = hero;
    } else {
        caster.unit = m_current;
        m_current->roundState.finishedTurn = true;
        m_current->castsDone++;
    }



    IBattleNotify::AffectedMagic affected;
    if (plan.m_spell->type == LibrarySpell::Type::Temp) {
        // @todo: special duration caluations!
        const int rounds = plan.m_power.spellPower + plan.m_power.durationBonus;
        const auto effect = BattleStack::Effect(plan.m_power, rounds);

        for (auto & target : plan.m_targeted) {
            auto targetStack = findStackNonConst(target.stack);

            if (target.magicSuccessChance != BonusRatio{1,1}) {
                if (checkResist(target.magicSuccessChance)) {
                    m_notifiers->onStackUnderEffect(target.stack, IBattleNotify::Effect::Resist);
                    continue;
                }
            }

            targetStack->appliedEffects.push_back(effect);
            affected.targets.push_back({target.stack, {}});

            recalcStack(targetStack);
        }
    } else if (plan.m_spell->type == LibrarySpell::Type::Offensive){
        for (auto & target : plan.m_targeted) {
            auto targetStack = findStackNonConst(target.stack);
            auto loss = target.loss;
            if (target.magicSuccessChance != BonusRatio{1,1}) {
                if (checkResist(target.magicSuccessChance)) {
                    m_notifiers->onStackUnderEffect(target.stack, IBattleNotify::Effect::Resist);
                    continue;
                }
            }
            affected.targets.push_back({target.stack, loss});

            applyLoss(targetStack, loss);
        }
    } else {
        assert(!"Unsupported");
    }
    affected.mainPosition = planParams.m_target;
    affected.area = std::vector<BattlePosition>(plan.m_affectedArea.cbegin(), plan.m_affectedArea.cend());

    if (!affected.targets.empty())
        m_notifiers->onCast(caster, affected, plan.m_spell);

    if (!planParams.m_isHeroCast) {
        m_current = nullptr;
    } else {
        recalcStack(m_current);
        if (!m_current->isAlive())
            m_current = nullptr;
    }
    updateState();
    return true;
}

std::unique_ptr<IAI> BattleManager::makeAI(const IAI::AIParams& params, IBattleControl & battleControl)
{
    return std::make_unique<AI>(params, battleControl, *this,  m_field);
}


// =================================== Internal ===================================
BattleHeroConstPtr BattleManager::currentHero() const
{
    return getHero(getCurrentSide());
}

BattleHeroMutablePtr BattleManager::currentHero()
{
    return getHeroMutable(getCurrentSide());
}

BattleHeroMutablePtr BattleManager::getHeroMutable(BattleStack::Side side)
{
    auto * hero = (side == BattleStack::Side::Attacker ? &m_att.battleHero : &m_def.battleHero);
    return hero->isValid() ? hero : nullptr;
}

void BattleManager::makeMelee(BattleStackMutablePtr attacker, BattleStackMutablePtr defender, const std::vector<BattleStackConstPtr> & sideTargets, bool isRetaliation)
{
    const LuckRoll luckRoll = makeLuckRoll(attacker);
    const BonusRatio baseRoll = attacker->roundState.baseRoll == -1
                              ? GeneralEstimation(m_rules).calculatePhysicalBase(attacker->current.primary.dmg, attacker->count, GeneralEstimation::DamageRollMode::Random, *m_randomGenerator)
                              : BonusRatio(attacker->roundState.baseRoll, 1) * BonusRatio(attacker->count, attacker->roundState.baseRollCount); // if count changed after last roll, scale it.

    const BonusRatio retaliationPenalty = isRetaliation ? attacker->current.retaliationPower : BonusRatio{1,1};
    const DamageResult damage = fullDamageEstimate(baseRoll, attacker, defender, true, retaliationPenalty, luckRoll);
    attacker->roundState.baseRoll = damage.damageBaseRoll;
    attacker->roundState.baseRollCount = attacker->count;

    IBattleNotify::AffectedPhysical affected;
    affected.main = {defender, damage};
    for (auto sideTarget : sideTargets) {
        if (!sideTarget->isAlive())
            continue;

        const DamageResult damage = fullDamageEstimate(baseRoll, attacker, sideTarget, true, {1,1}, LuckRoll::None);
        affected.extra.push_back({sideTarget, damage});
    }
    m_notifiers->beforeAttackMelee(attacker, affected, isRetaliation);

    applyLoss(defender, damage.loss);
    for (const auto & extra : affected.extra) {
        auto stack = findStackNonConst(extra.stack);
        applyLoss(stack, extra.damage.loss);
    }
    if (isRetaliation) {
        attacker->current.retaliationPower = {1,1};
    }

    recalcStack(attacker);

    if (defender->isAlive()) {
        for (const auto & cast : attacker->current.castsOnHit) {
            if (!cast.melee)
                continue;
            if (!BattleEstimation(m_rules).checkSpellTarget(*defender, cast.params.spell))
                continue;
            if (!checkRngRoll(cast.chance))
                continue;

            defender->appliedEffects.emplace_back(cast.params, cast.params.spellPower);
            recalcStack(defender);
            m_notifiers->onCast({.unit = attacker}, { .targets = {{defender, {}}} }, cast.params.spell);
        }
    }
}

void BattleManager::makeRanged(BattleStackMutablePtr attacker, BattlePosition target, BattleStackMutablePtr defender, const std::vector<BattleStackConstPtr> & sideTargets, int64_t rangeDenom)
{
    const LuckRoll luckRoll = makeLuckRoll(attacker);
    const BonusRatio baseRoll = attacker->roundState.baseRoll == -1
                              ? GeneralEstimation(m_rules).calculatePhysicalBase(attacker->current.primary.dmg, attacker->count, GeneralEstimation::DamageRollMode::Random, *m_randomGenerator)
                              : BonusRatio(attacker->roundState.baseRoll, 1) * BonusRatio(attacker->count, attacker->roundState.baseRollCount); // if count changed after last roll, scale it.

    const DamageResult damage = defender ? fullDamageEstimate(baseRoll, attacker, defender, false, {1, rangeDenom}, luckRoll) : DamageResult{};

    attacker->roundState.baseRoll = std::max(1, baseRoll.roundDownInt());
    attacker->roundState.baseRollCount = attacker->count;


    IBattleNotify::AffectedPhysical affected;
    if (defender)
        affected.mainTargetPos = defender->pos;
    else
        affected.mainTargetPos.setMainPos(target);

    affected.main = {defender, damage};
    for (auto sideTarget : sideTargets) {
        if (!sideTarget->isAlive())
            continue;

        const DamageResult damageSide = fullDamageEstimate(baseRoll, attacker, sideTarget, false, {1, rangeDenom}, LuckRoll::None);
        affected.extra.push_back({sideTarget, damageSide});
    }
    m_notifiers->beforeAttackRanged(attacker, affected);
    if (defender)
        applyLoss(defender, damage.loss);

    for (const auto & extra : affected.extra) {
        auto stack = findStackNonConst(extra.stack);
        applyLoss(stack, extra.damage.loss);
    }

    attacker->remainingShoots--; // @todo: ammo cart?
    recalcStack(attacker);
}

BattleStackMutablePtr BattleManager::findStackNonConst(const BattlePosition pos, bool onlyAlive)
{
    std::vector<BattleStackMutablePtr> & pointers = onlyAlive ? m_alive : m_all;

    auto it = std::find_if(pointers.begin(), pointers.end(), [&pos](auto * stack){ return stack->pos.contains(pos);});
    return it == pointers.end() ? nullptr : *it;
}

BattleStackMutablePtr BattleManager::findStackNonConst(BattleStackConstPtr stack)
{
    auto it = std::find(m_all.begin(), m_all.end(), stack);
    return it == m_all.end() ? nullptr : *it;
}

std::vector<BattleStackConstPtr> BattleManager::findNeighboursOf(BattleStackConstPtr stack) const
{
    std::vector<BattleStackConstPtr> result;
    result.reserve(8);
    for (const auto nighbourPos : m_field.getAdjacentSet(stack->pos)) {
        auto neighbour = findStack(nighbourPos, true);
        if (neighbour)
            result.push_back(neighbour);
    }
    return result;
}

std::vector<BattleStackMutablePtr> BattleManager::findMutableNeighboursOf(BattleStackConstPtr stack)
{
    std::vector<BattleStackMutablePtr> result;
    result.reserve(8);
    for (const auto nighbourPos : m_field.getAdjacentSet(stack->pos)) {
        auto neighbour = findStackNonConst(nighbourPos, true);
        if (neighbour)
            result.push_back(neighbour);
    }
    return result;
}



DamageEstimate BattleManager::estimateDamage(BattleStackConstPtr attacker,
                                             BattleStackConstPtr defender,
                                             BattlePlanMove::Attack mode,
                                             int64_t rangeDenom) const
{
    if (!attacker || !defender)
        return {};

    const bool isMelee = mode == BattlePlanMove::Attack::Melee;

    const BonusRatio baseRollMin =  GeneralEstimation(m_rules).calculatePhysicalBase(attacker->current.primary.dmg, attacker->count, GeneralEstimation::DamageRollMode::Min, *m_randomGenerator);
    const BonusRatio baseRollAvg =  GeneralEstimation(m_rules).calculatePhysicalBase(attacker->current.primary.dmg, attacker->count, GeneralEstimation::DamageRollMode::Avg, *m_randomGenerator);
    const BonusRatio baseRollMax =  GeneralEstimation(m_rules).calculatePhysicalBase(attacker->current.primary.dmg, attacker->count, GeneralEstimation::DamageRollMode::Max, *m_randomGenerator);

    return {
        true,
        fullDamageEstimate(baseRollMin, attacker, defender, isMelee, {1, rangeDenom}),
        fullDamageEstimate(baseRollAvg, attacker, defender, isMelee, {1, rangeDenom}),
        fullDamageEstimate(baseRollMax, attacker, defender, isMelee, {1, rangeDenom})
    };
}

DamageEstimate BattleManager::estimateRetaliationDamage(BattleStackConstPtr attacker,
                                                        BattleStackConstPtr defender,
                                                        BattlePlanMove::Attack mode,
                                                        const DamageEstimate & mainEstimate) const
{
    if (!attacker || !defender)
        return {};

    if (mode != BattlePlanMove::Attack::Melee)
        return {};

    if (mainEstimate.lowRoll.isKilled())
        return {};


    if (!canRetaliate(attacker, defender))
        return {};

    const BonusRatio baseRollMin =  GeneralEstimation(m_rules).calculatePhysicalBase(defender->current.primary.dmg, defender->count - mainEstimate.lowRoll.loss.deaths, GeneralEstimation::DamageRollMode::Min, *m_randomGenerator);
    const BonusRatio baseRollAvg =  GeneralEstimation(m_rules).calculatePhysicalBase(defender->current.primary.dmg, defender->count - mainEstimate.avgRoll.loss.deaths, GeneralEstimation::DamageRollMode::Avg, *m_randomGenerator);
    const BonusRatio baseRollMax =  GeneralEstimation(m_rules).calculatePhysicalBase(defender->current.primary.dmg, defender->count - mainEstimate.maxRoll.loss.deaths, GeneralEstimation::DamageRollMode::Max, *m_randomGenerator);


    return {
        true,
        fullDamageEstimate(baseRollMin, defender, attacker, true, {1,1} ),
        fullDamageEstimate(baseRollAvg, defender, attacker, true, {1,1} ),
        fullDamageEstimate(baseRollMax, defender, attacker, true, {1,1} )
    };
}

bool BattleManager::canRetaliate(BattleStackConstPtr attacker, BattleStackConstPtr defender) const
{
    if (!defender || !attacker)
        return false;
    if (!defender->isAlive() || !attacker->isAlive() || !defender->current.canAttackMelee)
        return false;
    if (attacker->library->traits.freeAttack)
        return false;
    if (defender->current.retaliationPower.num() == 0)
        return false;
    if (defender->current.maxRetaliations == -1)
        return true;

    if (defender->roundState.retaliationsDone >= defender->current.maxRetaliations)
        return false;

    return true;
}

int64_t BattleManager::rangedDenom(BattleStackConstPtr attacker, BattleStackConstPtr defender) const
{
    const int distance = attacker->pos.shortestHexDistance(defender->pos).first;
    const bool wallOnTheWay = false;//@todo: walls
    return rangedDenom(attacker, distance, wallOnTheWay);
}

int64_t BattleManager::rangedDenom(BattleStackConstPtr attacker, BattlePosition target) const
{
    const int distance = attacker->pos.shortestHexDistance(target).first;
    const bool wallOnTheWay = false;//@todo: walls
    return rangedDenom(attacker, distance, wallOnTheWay);
}

int64_t BattleManager::rangedDenom(BattleStackConstPtr attacker, int distance, bool wallOnTheWay) const
{
    const bool hasDistancePenalty = !attacker->adventure->estimated.disabledPenalties.contains(RangeAttackPenalty::Distance);
    const bool hasWallPenalty     = !attacker->adventure->estimated.disabledPenalties.contains(RangeAttackPenalty::Obstacle);
    const int distanceDen = (hasDistancePenalty && distance > rangedLimit) ? 2 : 1;
    const int wallsDen = hasWallPenalty && wallOnTheWay ? 2 : 1;
    return distanceDen * wallsDen;
}

BonusRatio BattleManager::meleeAttackFactor(BattleStackConstPtr attacker, BattleStackConstPtr defender) const
{
    (void)defender;
    if (!attacker->library->traits.rangeAttack)
        return BonusRatio(1, 1);

    const bool hasMeleePenalty = !attacker->adventure->estimated.disabledPenalties.contains(RangeAttackPenalty::Melee);

    return BonusRatio(1, hasMeleePenalty ? 2 : 1);
}

DamageResult BattleManager::fullDamageEstimate(const BonusRatio baseRoll,
                                               BattleStackConstPtr attacker,
                                               BattleStackConstPtr defender,
                                               bool melee,
                                               BonusRatio extraReduce,
                                               LuckRoll luckFactor) const
{
    BonusRatio totalBaseFactor      {0,1}; // use to base = base + base * factor
    BonusRatio totalReduceFactor    {1,1}; // use to baseIncreased = baseIncreased * factor
    int attack = attacker->current.primary.ad.attack;
    int defense = defender->current.primary.ad.defense;
    if (melee) {
        attack  += attacker->current.adMelee.attack;
        defense += defender->current.adMelee.defense;
    } else {
        attack  += attacker->current.adRanged.attack;
        defense += defender->current.adRanged.defense;
    }
    attack  = (defender->library->abilities.reduceAttackerAttack * attack ).roundDownInt();
    defense = (attacker->library->abilities.reduceTargetDefense  * defense).roundDownInt();

    BonusRatio apCalcBase   {0,1};
    BonusRatio apCalcReduce {0,1};
    const int attackPower = attack - defense;
    if (attackPower > 0) {
        const int effectiveAttackPower = std::clamp(attackPower, 0, m_rules->physicalConst.maxEffectiveAttack);
        apCalcBase = m_rules->physicalConst.attackValue * effectiveAttackPower;
    } else if (attackPower < 0) {
        const int effectiveDefensePower = std::clamp(-attackPower, 0, m_rules->physicalConst.maxEffectiveDefense);
        apCalcReduce = m_rules->physicalConst.defenseValue * effectiveDefensePower;
    }

    totalBaseFactor   += apCalcBase;
    totalReduceFactor *= (BonusRatio{1,1} - apCalcReduce);

    if (luckFactor == LuckRoll::Luck)
        totalBaseFactor += BonusRatio{1,1};
    else if (luckFactor == LuckRoll::Unluck)
        totalReduceFactor *= BonusRatio{1,2};

    totalReduceFactor *= extraReduce;

    if (melee) {
        totalReduceFactor *= meleeAttackFactor(attacker, defender);
        if (attacker->hero)
            totalBaseFactor += attacker->hero->adventure->estimated.meleeAttack;

        totalBaseFactor   += attacker->current.meleeAttack;
        totalReduceFactor *= (BonusRatio{1,1} - defender->current.meleeDefense);
    } else {
        if (attacker->hero)
            totalBaseFactor += attacker->hero->adventure->estimated.rangedAttack;

        totalBaseFactor   += attacker->current.rangedAttack;
        totalReduceFactor *= (BonusRatio{1,1} - defender->current.rangedDefense);
    }
    if (defender->hero)
        totalReduceFactor *= (BonusRatio{1,1} - defender->hero->adventure->estimated.defense);

    auto & enemies = attacker->library->abilities.extraDamage.enemies;
    if (std::find(enemies.cbegin(), enemies.cend(), defender->library) != enemies.cend())
        totalBaseFactor   += attacker->library->abilities.extraDamage.damageBonus;

    const auto & awe = attacker->library->abilities.attackWithElement;
    if (awe != LibraryUnit::Abilities::AttackWithElement::None
        && awe == defender->library->abilities.vulnerableAgainstElement)
        totalBaseFactor += defender->library->abilities.vulnerableBonus;

    static const std::set<UnitNonLivingType> s_mindImmunes {UnitNonLivingType::Golem, UnitNonLivingType::Elemental, UnitNonLivingType::Undead};
    if (awe == LibraryUnit::Abilities::AttackWithElement::Magic && defender->current.immunes.containsAll())
        totalReduceFactor *= BonusRatio{1,2};
    else if (awe == LibraryUnit::Abilities::AttackWithElement::Mind
             && (defender->current.immunes.containsMind()
                 || s_mindImmunes.contains(defender->library->abilities.nonLivingType)))
        totalReduceFactor *= BonusRatio{1,2};

    DamageResult damageResult;
    damageResult.damageBaseRoll = std::max(1, baseRoll.roundDownInt());

    BonusRatio finalRoll      = baseRoll * (BonusRatio(1,1) + totalBaseFactor) * totalReduceFactor;
    auto damageTotal   = std::max(finalRoll.roundDownInt(), 1);
    if (baseRoll > BonusRatio(0,1))
        damageResult.damagePercent = (finalRoll * 100 / baseRoll).roundDownInt();

    damageResult.loss = damageLoss(defender, damageTotal);
    return damageResult;
}

DamageResult::Loss BattleManager::damageLoss(BattleStackConstPtr defender, int damage) const
{
    DamageResult::Loss loss;
    const int defenderMaxHealth = defender->current.primary.maxHealth;
    const int effectiveHealth   = defender->health + (defender->count - 1) *  defenderMaxHealth;
    const int remainEffHealth   = effectiveHealth - damage;
    loss.remainCount     = remainEffHealth <= 0 ? 0 : ((remainEffHealth - 1) / defenderMaxHealth) + 1;
    loss.remainTopStackHealth = remainEffHealth - (loss.remainCount - 1) * defenderMaxHealth;
    loss.deaths = defender->count - loss.remainCount;
    loss.damageTotal = damage;
    return loss;
}


BattleFieldPathFinder BattleManager::setupFinder(BattleStackConstPtr stack) const
{
    BattleFieldPathFinder finder(m_field);
    finder.setGoThroughObstacles(stack->library->traits.fly || stack->library->traits.teleport);
    const bool mirrored = stack->side == BattleStack::Side::Defender;
    const bool large = stack->library->traits.large;
    BattlePositionSet finderObstacles;
    std::vector<BattlePosition> battleObstacles = this->m_obstacles;
    for (auto * stackAlive : m_alive) {
        if (stackAlive != stack) {
            battleObstacles.push_back(stackAlive->pos.leftPos());
            battleObstacles.push_back(stackAlive->pos.rightPos());
        }
    }
    for (auto pos : battleObstacles) {
        finderObstacles.insert(pos);
        if (!large)
            continue;
        finderObstacles.insert({mirrored ? pos.x + 1 : pos.x - 1, pos.y});
    }
    if (large) {
        for (int h = 0; h < m_field.height; ++h)
             finderObstacles.insert({mirrored ? 0 : m_field.width - 1, h});
    }
    finder.setObstacles(finderObstacles);
    finder.floodFill(stack->pos.mainPos());
    return finder;
}

BattlePositionSet BattleManager::getSpellArea(BattlePosition pos, LibrarySpell::Range range) const
{
    BattlePositionSet result;
    result.insert(pos);
    int floodIterations = 0;
    if (range == LibrarySpell::Range::R1 || range == LibrarySpell::Range::R1NoCenter) {
        floodIterations = 1;
    }
    else if (range == LibrarySpell::Range::R2) {
        floodIterations = 2;
    }
    else if (range == LibrarySpell::Range::R3) {
        floodIterations = 3;
    }
    if (floodIterations > 0)
        result = m_field.getFloodFillFrom(pos, floodIterations);

    if (range == LibrarySpell::Range::All) {
        for (int x = 0; x < m_field.width; ++x) {
            for (int y = 0; y < m_field.height; ++y) {
                result.insert({x, y});
            }
        }
    }
    for (auto ob : m_obstacles)
        result.erase(ob);
    if (range == LibrarySpell::Range::R1NoCenter) {
        result.erase(pos);
    }
    return result;
}

BattlePositionSet BattleManager::getSplashExtraTargets(LibraryUnit::Abilities::SplashAttack splash,
                                                       BattlePositionExtended from,
                                                       BattleAttackDirection direction) const
{
    BattlePositionSet result;
    if (splash == LibraryUnit::Abilities::SplashAttack::Sides) {
        if (direction == BattleAttackDirection::R)  result = m_field.validNeighbours(from.rightPos(), {BattleDirection::BR, BattleDirection::TR});
        if (direction == BattleAttackDirection::TR) result = m_field.validNeighbours(from.rightPos(), {BattleDirection::TL, BattleDirection::R});
        if (direction == BattleAttackDirection::BR) result = m_field.validNeighbours(from.rightPos(), {BattleDirection::BL, BattleDirection::R});

        if (direction == BattleAttackDirection::L) result = m_field.validNeighbours(from.leftPos(), {BattleDirection::BL, BattleDirection::TL});
        if (direction == BattleAttackDirection::TL)result = m_field.validNeighbours(from.leftPos(), {BattleDirection::TR, BattleDirection::L});
        if (direction == BattleAttackDirection::BL)result = m_field.validNeighbours(from.leftPos(), {BattleDirection::BR, BattleDirection::L});

        if (from.sightDirection() == BattlePositionExtended::Sight::ToRight) {
            if (direction == BattleAttackDirection::T) result = m_field.validNeighbours(from.rightPos(), {BattleDirection::TR});
            if (direction == BattleAttackDirection::B) result = m_field.validNeighbours(from.rightPos(), {BattleDirection::BR});
        }
        if (from.sightDirection() == BattlePositionExtended::Sight::ToLeft) {
            if (direction == BattleAttackDirection::T) result = m_field.validNeighbours(from.leftPos(), {BattleDirection::TL});
            if (direction == BattleAttackDirection::B) result = m_field.validNeighbours(from.leftPos(), {BattleDirection::BL});
        }
    }
    if (splash == LibraryUnit::Abilities::SplashAttack::Neighbours) {
        result = m_field.getAdjacentSet(from);
    }
    if (splash == LibraryUnit::Abilities::SplashAttack::Dragon) {
        if (direction == BattleAttackDirection::R)  result = m_field.validNeighbours(m_field.neighbour(from.rightPos(), BattleDirection::R ), {BattleDirection::R});
        if (direction == BattleAttackDirection::TR) result = m_field.validNeighbours(m_field.neighbour(from.rightPos(), BattleDirection::TR), {BattleDirection::TR});
        if (direction == BattleAttackDirection::BR) result = m_field.validNeighbours(m_field.neighbour(from.rightPos(), BattleDirection::BR), {BattleDirection::BR});

        if (direction == BattleAttackDirection::L)  result = m_field.validNeighbours(m_field.neighbour(from.leftPos(), BattleDirection::L ), {BattleDirection::L});
        if (direction == BattleAttackDirection::TL) result = m_field.validNeighbours(m_field.neighbour(from.leftPos(), BattleDirection::TL), {BattleDirection::TL});
        if (direction == BattleAttackDirection::BL) result = m_field.validNeighbours(m_field.neighbour(from.leftPos(), BattleDirection::BL), {BattleDirection::BL});

        if (from.sightDirection() == BattlePositionExtended::Sight::ToRight) {
            if (direction == BattleAttackDirection::T) result = m_field.validNeighbours(m_field.neighbour(from.rightPos(), BattleDirection::TL), {BattleDirection::TL});
            if (direction == BattleAttackDirection::B) result = m_field.validNeighbours(m_field.neighbour(from.rightPos(), BattleDirection::BL), {BattleDirection::BL});
        }
        if (from.sightDirection() == BattlePositionExtended::Sight::ToLeft) {
            if (direction == BattleAttackDirection::T) result = m_field.validNeighbours(m_field.neighbour(from.leftPos(), BattleDirection::TR), {BattleDirection::TR});
            if (direction == BattleAttackDirection::B) result = m_field.validNeighbours(m_field.neighbour(from.leftPos(), BattleDirection::BR), {BattleDirection::BR});
        }
    }

    return result;
}

// =================================== Setup ===================================


void BattleManager::makePositions(const BattleFieldPreset & fieldPreset)
{
    for (auto & stack : m_att.squad->stacks) {
        m_all.push_back(&stack);
        BattlePosition pos = fieldPreset.calcPosition(true,
                                                      stack.adventure->armyParams.indexInArmyValid,
                                                      m_att.squad->stacks.size(),
                                                      m_att.squad->adventure->useCompactFormation);
        stack.pos.setMainPos(pos);
    }

    for (auto & stack : m_def.squad->stacks) {
        m_all.push_back(&stack);
        BattlePosition pos = fieldPreset.calcPosition(false,
                                                      stack.adventure->armyParams.indexInArmyValid,
                                                      m_def.squad->stacks.size(),
                                                      m_def.squad->adventure->useCompactFormation);
        if (pos.isEmpty()) { // that meant that provided stack do not fits into positions layout. Make it dead for now.
            stack.count = 0;
            Logger(Logger::Warning) << "Removing non-fitting stack" << stack.library->id << " from defender army";
        }
        stack.pos.setMainPos(pos);
    }
}

void BattleManager::initialParams()
{
    BattleEstimation(m_rules).calculateEnvironmentOnBattleStart(m_env, m_att, m_def);
    BattleEstimation(m_rules).calculateArmyOnBattleStart(m_def, m_att, m_env);
    BattleEstimation(m_rules).calculateArmyOnBattleStart(m_att, m_def, m_env);

    for (auto * stack : m_all) {
        stack->pos.setLarge(stack->adventure->library->traits.large);
        stack->pos.setSight(stack->side == BattleStack::Side::Attacker ? BattlePositionExtended::Sight::ToRight : BattlePositionExtended::Sight::ToLeft);
    }
}

void BattleManager::updateState()
{
    m_alive.clear();
    std::copy_if(m_all.begin(), m_all.end(), std::back_inserter(m_alive), [](auto * stack){ return stack->count > 0;});

    {
        int attackerAlive = 0;
        int defenderAlive = 0;
        for (auto * stack : m_alive) {
            if (stack->side == BattleStack::Side::Attacker)
                attackerAlive++;
            else
                defenderAlive++;
        }
        auto endGame = [this](BattleResult::Result result) {
            m_battleFinished = true;
            m_notifiers->onBattleFinished({result});
        };
        if (attackerAlive == 0 && defenderAlive == 0)
            return endGame(BattleResult::Result::Tie);
        else if (attackerAlive == 0)
            return endGame(BattleResult::Result::DefenderWon);
        else if (defenderAlive == 0)
            return endGame(BattleResult::Result::AttackerWon);
    }

    {
        std::map<BattleStack::Side, std::map<int, int>> bySpeed;
        for (auto * stack : m_alive) {
            recalcStack(stack);
            int & order = bySpeed[stack->side][stack->current.primary.battleSpeed];
            stack->sameSpeedOrder = order++;
            stack->speedOrder = -stack->current.primary.battleSpeed;
        }
    }

    m_roundQueue.clear();
    std::copy_if(m_alive.begin(), m_alive.end(), std::back_inserter(m_roundQueue), [](auto * stack){
        return !stack->roundState.finishedTurn && stack->current.canDoAnything;
    });

    std::sort(m_roundQueue.begin(), m_roundQueue.end(), [](BattleStackMutablePtr left, BattleStackMutablePtr right){
        return left->turnOrder() < right->turnOrder();
    });

    if (m_roundQueue.empty()) {
        // @note: Long recursion in calls when a lot of units skip turns every round is a small concern, we have not so much data on the stack.
        // braces in blocks above just to reduce stack data.
        startNewRound();
        return;
    }

    if (!m_current)
        m_current = m_roundQueue[0];

    beforeCurrentActive();

    m_notifiers->onStateChanged();
}

void BattleManager::startNewRound()
{
    BattleEstimation(m_rules).calculateArmyOnRoundStart(m_def);
    BattleEstimation(m_rules).calculateArmyOnRoundStart(m_att);

    m_roundIndex++;

    m_notifiers->onStartRound(m_roundIndex);

    updateState();
}

void BattleManager::beforeCurrentActive()
{
    if (!m_current)
        return;

    if (m_current->count > 0 && m_current->current.canDoAnything) {
        const int morale = m_current->current.rngParams.morale;
        if (morale < 0 && !m_current->roundState.hadLowMorale && checkRngRoll(m_current->current.moraleChance)) {
            m_current->roundState.finishedTurn = true;
            m_current->roundState.hadLowMorale = true;
            m_notifiers->onStackUnderEffect(m_current, IBattleNotify::Effect::BadMorale);
            m_current = nullptr;
            updateState();
            return;
        }
        if (m_current->adventure->estimated.regenerate && m_current->health < m_current->current.primary.maxHealth) {
            m_current->health = m_current->current.primary.maxHealth;
            m_notifiers->onStackUnderEffect(m_current, IBattleNotify::Effect::Regenerate);
        }
    }
    checkSidesFirstTurn();

    m_notifiers->onStateChanged();
}

void BattleManager::checkSidesFirstTurn()
{
    if (!m_attackerHadFirstTurn && m_current->side == BattleStack::Side::Attacker) {
        m_attackerHadFirstTurn = true;
        beforeFirstTurn(BattleStack::Side::Attacker);
    }
    if (!m_defenderHadFirstTurn && m_current->side == BattleStack::Side::Defender) {
        m_defenderHadFirstTurn = true;
        beforeFirstTurn(BattleStack::Side::Defender);
    }
}

void BattleManager::beforeFirstTurn(BattleStack::Side side)
{
    BattleArmy & army = side == BattleStack::Side::Attacker ? m_att : m_def;
    if (!army.battleHero.isValid())
        return;
    for (auto & castBeforeStart : army.battleHero.adventure->estimated.castsBeforeStart) {
        int rounds = castBeforeStart.spellPower;
        const auto effect = BattleStack::Effect(castBeforeStart, rounds);
        IBattleNotify::AffectedMagic affected;
        for (auto targetStack : m_alive) {
            if (castBeforeStart.spell->qualify == LibrarySpell::Qualify::Good && targetStack->side != side)
                continue;
            if (castBeforeStart.spell->qualify == LibrarySpell::Qualify::Bad && targetStack->side == side)
                continue;
            if (!BattleEstimation(m_rules).checkSpellTarget(*targetStack, castBeforeStart.spell))
                continue;

            targetStack->appliedEffects.push_back(effect);
            affected.targets.push_back({targetStack, {}});

            recalcStack(targetStack);
        }
        if (!affected.targets.empty()) {
            IBattleNotify::Caster caster;
            caster.artifact =  castBeforeStart.art;
            m_notifiers->onCast(caster, affected, castBeforeStart.spell);
        }

    }
    m_notifiers->onStateChanged();
}

bool BattleManager::canMoveOrAttack(BattleStackConstPtr stack) const
{
    // @todo: stone / blind
    return stack->isAlive();
}



bool BattleManager::checkRngRoll(BonusRatio rollChance)
{
    if (rollChance.num() == 0)
        return false;

    if (rollChance >= BonusRatio{1,1})
        return true;

    assert(rollChance > BonusRatio(0,1));
    if (rollChance.denom() < 128) {
        auto roll = m_randomGenerator->genSmall(static_cast<uint8_t>(rollChance.denom()) - 1);
        return (roll < rollChance.num());
    }
    auto roll = static_cast<int64_t>(m_randomGenerator->gen(rollChance.denom() - 1));
    return (roll < rollChance.num());
}

bool BattleManager::checkResist(BonusRatio successChance)
{
    if (successChance < BonusRatio{1, 100})
        successChance = {1, 100}; // @todo: move to  gameRules.

    const bool isSuccess = checkRngRoll(successChance);
    return !isSuccess;
}

BattleManager::LuckRoll BattleManager::makeLuckRoll(BattleStackConstPtr attacker)
{
    int luckLevel = attacker->current.rngParams.luck;
    if (!luckLevel)
        return LuckRoll::None;

    if (!checkRngRoll(m_current->current.luckChance))
        return LuckRoll::None;

    if (luckLevel > 0) {
        m_notifiers->onStackUnderEffect(attacker, IBattleNotify::Effect::GoodLuck);
        return LuckRoll::Luck;
    }
    m_notifiers->onStackUnderEffect(attacker, IBattleNotify::Effect::BadLuck);
    return LuckRoll::Unluck;
}

void BattleManager::recalcStack(BattleStackMutablePtr stack)
{
    BattleEstimation(m_rules).calculateUnitStats(*stack);

    const bool canBeBlocked = !stack->adventure->estimated.disabledPenalties.contains(RangeAttackPenalty::Blocked);
    if (canBeBlocked) {
        for (BattleStackConstPtr neighbour : findNeighboursOf(stack)) {
            if (neighbour->side != stack->side) {
                stack->current.rangeAttackIsBlocked = true;
                break;
            }
        }
    }
}

void BattleManager::applyLoss(BattleStackMutablePtr stack, const DamageResult::Loss & loss)
{
    stack->count  = loss.remainCount;
    stack->health = loss.remainCount > 0 ? loss.remainTopStackHealth : 0;
    BonusRatio retaliationPower {1,1};
    {
        BattleStack::EffectList effectsTmp;
        for (auto & effect : stack->appliedEffects) {
            if (effect.power.spell->hasEndCondition(LibrarySpell::EndCondition::GetHit)) {
                if (!effect.power.spell->retaliationWhenCancel.empty())
                    retaliationPower = effect.power.spell->retaliationWhenCancel[effect.power.skillLevel];

                continue;
            }
            effectsTmp.push_back(effect);
        }
        stack->appliedEffects = effectsTmp;
    }

    recalcStack(stack);

    stack->current.retaliationPower = retaliationPower;
}

BattleManager::ControlGuard::ControlGuard(BattleManager* parent) : parent(parent)
{
    parent->m_notifiers->onControlAvailableChanged(false);
}

BattleManager::ControlGuard::~ControlGuard()
{
    parent->m_notifiers->onControlAvailableChanged(!parent->m_battleFinished);
}

}
