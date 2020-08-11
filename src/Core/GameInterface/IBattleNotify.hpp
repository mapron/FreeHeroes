/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattleFieldPosition.hpp"
#include "BattleStack.hpp"

namespace FreeHeroes::Core {


struct BattleResult {
    enum class Result { AttackerWon, DefenderWon, Tie };
    Result result;
};

class IBattleNotify {
public:
    virtual ~IBattleNotify() = default;

    virtual void beforeMove(BattleStackConstPtr stack, const BattlePositionPath & path) = 0;
    struct AffectedPhysical {
        struct Target {
            BattleStackConstPtr stack;
            DamageResult damage;
        };
        BattlePositionExtended mainTargetPos;
        Target main;
        std::vector<Target> extra;

        std::vector<Target> getAll() const {
            std::vector<Target> result = extra;
            if (main.stack)
                result.push_back(main);
            return result;
        }
    };

    virtual void beforeAttackMelee(BattleStackConstPtr stack, const AffectedPhysical & affected, bool isRetaliation) = 0;
    virtual void beforeAttackRanged(BattleStackConstPtr stack, const AffectedPhysical & affected) = 0;
    virtual void beforeWait(BattleStackConstPtr stack) = 0;
    virtual void beforeGuard(BattleStackConstPtr stack, int defBonus) = 0;

    enum class Effect { GoodMorale, BadMorale, GoodLuck, BadLuck, Resist };
    virtual void onStackUnderEffect(BattleStackConstPtr stack, Effect effect) = 0;
    struct Caster {
        BattleHeroConstPtr hero = nullptr;
        BattleStackConstPtr unit = nullptr;
        LibraryArtifactConstPtr artifact = nullptr;
    };
    struct AffectedMagic {
        BattlePosition mainPosition;
        std::vector<BattlePosition> area;
        struct Target {
            BattleStackConstPtr unit = nullptr;;
            DamageResult::Loss loss;
        };

        std::vector<Target> targets;
    };

    virtual void onCast(const Caster & caster, const AffectedMagic & affected, LibrarySpellConstPtr spell) = 0;

    virtual void onPositionReset(BattleStackConstPtr stack) = 0;
    virtual void onStartRound(int round) = 0;
    virtual void onBattleFinished(BattleResult result) = 0;
    virtual void onStateChanged() = 0;
    virtual void onControlAvailableChanged(bool controlAvailable) = 0;
};

class BattleNotifyEmpty : public IBattleNotify {
public:
    void beforeMove(BattleStackConstPtr , const BattlePositionPath & ) override {}
    void beforeAttackMelee(BattleStackConstPtr, const AffectedPhysical &, bool) override {}
    void beforeAttackRanged(BattleStackConstPtr, const AffectedPhysical &) override {}
    void beforeWait(BattleStackConstPtr) override {}
    void beforeGuard(BattleStackConstPtr, int) override {}

    void onStackUnderEffect(BattleStackConstPtr, Effect ) override {}
    void onCast(const Caster &, const AffectedMagic &, LibrarySpellConstPtr ) override {}

    void onPositionReset(BattleStackConstPtr) override {}
    void onStartRound(int) override {}
    void onBattleFinished(BattleResult) override {}
    void onStateChanged() override {}
    void onControlAvailableChanged(bool) override {}
};

}
