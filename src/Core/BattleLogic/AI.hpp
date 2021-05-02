/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IBattleControl.hpp"
#include "IBattleView.hpp"
#include "IAI.hpp"
#include "BattleField.hpp"

#include "Profiler.hpp"

namespace FreeHeroes::Core {

class AI : public IAI {
public:
    AI(const AIParams& params, IBattleControl& battleControl, IBattleView& battleView, BattleFieldGeometry geometry);

    int         run(int stepLimit) override;
    void        runStep() override;
    std::string getProfiling() const override;
    void        clearProfiling() override;

private:
    void    prepareReachable();
    int64_t calculateValueForRanged(BattleStackConstPtr stack);
    int64_t calculateValueForAttackPlan(const BattlePlanMove& planResult);
    bool    findWaitReachable();

    void makeRangedAttack();
    void makeMeleeAttack();
    bool makeMoveToClosestTarget();

private:
    const AIParams m_params;

    IBattleControl& m_battleControl;
    IBattleView&    m_battleView;

    struct Try {
        BattleStackConstPtr stack;

        BattlePlanMoveParams   moveParams;
        BattlePlanAttackParams attackParams;
        int                    distanceTurns    = 0;
        int                    distanceCells    = 0;
        int64_t                meleeAttackValue = 0;
    };

    struct OpponentStack {
        BattleStackConstPtr m_stack;
        bool                m_isWide = false;

        int m_turnsToReach  = -1; // -1 => unreachable
        int m_distanceCells = -1;

        int64_t m_rangedAttackValue = 0;
    };

    struct StepData {
        BattleStackConstPtr m_current             = nullptr;
        bool                m_rangeAttackAvaiable = false;
        bool                isWide                = false;
        int                 currentSpeed          = 0;

        std::vector<OpponentStack> m_opponent;

        std::vector<Try>          m_possibleAttacks;
        std::vector<Try>          m_possibleAttacksNotNow;
        BattlePositionDistanceMap reachNow;
        BattlePositionDistanceMap reachAtAll;

        void clear()
        { // no preserve allocated space
            m_opponent.clear();

            m_possibleAttacks.clear();
            m_possibleAttacksNotNow.clear();
            reachNow.clear();
            reachAtAll.clear();
        }
    };
    StepData                  m_stepData;
    ProfilerContext           m_profileContext;
    const BattleFieldGeometry m_field;
};

}
