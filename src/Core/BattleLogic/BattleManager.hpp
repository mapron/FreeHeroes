/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattleLogicExport.hpp"

#include "IBattleView.hpp"
#include "IBattleControl.hpp"
#include "IAIFactory.hpp"

#include "BattleField.hpp"
#include "BattleArmy.hpp"
#include "BattleEnvironment.hpp"
#include "GeneralEstimation.hpp"

namespace FreeHeroes::Core {

class BattleFieldPathFinder;
class IBattleNotify;
class IRandomGenerator;

class BATTLELOGIC_EXPORT BattleManager : public IBattleView
    , public IBattleControl
    , public IAIFactory {
public:
    BattleManager() = delete;
    ~BattleManager();

    BattleManager(BattleArmy&                              attArmy,
                  BattleArmy&                              defArmy,
                  const BattleFieldPreset&                 fieldPreset,
                  const std::shared_ptr<IRandomGenerator>& randomGenerator,
                  LibraryGameRulesConstPtr                 rules);

    void start();

    // View
protected:
    AvailableActions                 getAvailableActions() const override;
    std::vector<BattleStackConstPtr> getAllStacks(bool m_alive) const override;
    std::vector<BattlePosition>      getObstacles() const override;

    BattleStackConstPtr       findStack(const BattlePosition pos, bool onlyAlive = false) const override;
    BattlePositionSet         findAvailable(BattleStackConstPtr stack) const override;
    BattlePositionDistanceMap findDistances(BattleStackConstPtr stack, int limit) const override;
    BattlePlanMove            findPlanMove(const BattlePlanMoveParams& moveParams, const BattlePlanAttackParams& attackParams) const override;
    BattlePlanCast            findPlanCast(const BattlePlanCastParams& castParams) const override;

    DamageResult estimateAvgDamageFromOpponentAfterMove(BattleStackConstPtr stack, BattlePosition newPos) const override;

    BattleHeroConstPtr  getHero(BattleStack::Side side) const override;
    BattleStack::Side   getCurrentSide() const override;
    BattleStackConstPtr getActiveStack() const override;

    void addNotify(IBattleNotify* handler) override;
    void removeNotify(IBattleNotify* handler) override;

    bool isFinished() const override;

    // Control
protected:
    bool doMoveAttack(BattlePlanMoveParams moveParams, BattlePlanAttackParams attackParams) override;
    bool doWait() override;
    bool doGuard() override;
    bool doCast(BattlePlanCastParams planParams) override;

    // IAIFactory
public:
    std::unique_ptr<IAI> makeAI(const IAI::AIParams& params, IBattleControl& battleControl) override;

    // Internal
private:
    BattleHeroConstPtr   currentHero() const;
    BattleHeroMutablePtr currentHero();
    BattleHeroMutablePtr getHeroMutable(BattleStack::Side side);

    void makeMelee(BattleStackMutablePtr attacker, BattleStackMutablePtr defender, const std::vector<BattleStackConstPtr>& sideTargets, bool isRetaliation);
    void makeRanged(BattleStackMutablePtr attacker, BattlePosition target, BattleStackMutablePtr defender, const std::vector<BattleStackConstPtr>& sideTargets, int64_t rangeDenom);

    BattleStackMutablePtr              findStackNonConst(const BattlePosition pos, bool onlyAlive = false);
    BattleStackMutablePtr              findStackNonConst(BattleStackConstPtr stack);
    std::vector<BattleStackConstPtr>   findNeighboursOf(BattleStackConstPtr stack) const;
    std::vector<BattleStackMutablePtr> findMutableNeighboursOf(BattleStackConstPtr stack);

    DamageEstimate estimateDamage(BattleStackConstPtr    attacker,
                                  BattleStackConstPtr    defender,
                                  BattlePlanMove::Attack mode,
                                  int64_t                rangedDenom) const;
    DamageEstimate estimateRetaliationDamage(BattleStackConstPtr    attacker,
                                             BattleStackConstPtr    defender,
                                             BattlePlanMove::Attack mode,
                                             const DamageEstimate&  mainEstimate) const;
    bool           canRetaliate(BattleStackConstPtr attacker, BattleStackConstPtr defender) const;

    int64_t    rangedDenom(BattleStackConstPtr attacker, BattleStackConstPtr defender) const;
    int64_t    rangedDenom(BattleStackConstPtr attacker, BattlePosition target) const;
    int64_t    rangedDenom(BattleStackConstPtr attacker, int distance, bool wallOnTheWay) const;
    BonusRatio meleeAttackFactor(BattleStackConstPtr attacker, BattleStackConstPtr defender) const;

    enum class LuckRoll
    {
        None,
        Luck,
        Unluck
    };
    DamageResult       fullDamageEstimate(const BonusRatio    baseRoll,
                                          BattleStackConstPtr attacker,
                                          BattleStackConstPtr defender,
                                          bool                melee,
                                          BonusRatio          extraReduce,
                                          LuckRoll            luckFactor = LuckRoll::None) const;
    DamageResult::Loss damageLoss(BattleStackConstPtr defender, int damage) const;
    DamageResult::Loss risingLoss(BattleStackConstPtr target, int health) const;

    BattleFieldPathFinder setupFinder(BattleStackConstPtr stack) const;
    BattlePositionSet     getSpellArea(BattlePosition pos, LibrarySpell::Range range) const;
    BattlePositionSet     getSplashExtraTargets(LibraryUnit::Abilities::SplashAttack splash,
                                                BattlePositionExtended               from,
                                                BattleAttackDirection                direction) const;

    // Setup
private:
    void makePositions(const BattleFieldPreset& fieldPreset);
    void initialParams();
    void updateState();

    void startNewRound();

    void beforeCurrentActive();

    void checkSidesFirstTurn();
    void beforeFirstTurn(BattleStack::Side side);

private:
    bool canMoveOrAttack(BattleStackConstPtr stack) const;

    bool     checkRngRoll(BonusRatio rollChance);
    bool     checkResist(BonusRatio successChance);
    LuckRoll makeLuckRoll(BattleStackConstPtr attacker);

    void recalcStack(BattleStackMutablePtr stack);
    void applyLoss(BattleStackMutablePtr stack, const DamageResult::Loss& loss, bool isRising = false);

private:
    BattleArmy&                        m_att;
    BattleArmy&                        m_def;
    BattleEnvironment                  m_env;
    std::vector<BattlePosition>        m_obstacles;
    const BattleFieldGeometry          m_field;
    std::vector<BattleStackMutablePtr> m_all;
    std::vector<BattleStackMutablePtr> m_alive;
    std::vector<BattleStackMutablePtr> m_roundQueue;
    BattleStackMutablePtr              m_current = nullptr;

    int  m_roundIndex           = 0;
    bool m_battleFinished       = false;
    bool m_attackerHadFirstTurn = false;
    bool m_defenderHadFirstTurn = false;

    class BattleNotifyEach;
    std::unique_ptr<BattleNotifyEach> m_notifiers;
    std::shared_ptr<IRandomGenerator> m_randomGenerator;
    LibraryGameRulesConstPtr          m_rules = nullptr;

    struct ControlGuard {
        ControlGuard(BattleManager* parent);
        ~ControlGuard();
        BattleManager* parent;
    };
};

}
