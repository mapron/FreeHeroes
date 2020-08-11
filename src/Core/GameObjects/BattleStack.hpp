/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureStack.hpp"

#include "BattleFieldPosition.hpp"

#include "BattleFwd.hpp"
#include "BonusRatio.hpp"
#include "LibrarySpell.hpp"

namespace FreeHeroes::Core {

struct BattleStack {
    enum class Side { Attacker, Defender };

    BattleStack() = default;
    BattleStack(AdventureStackConstPtr adventure, BattleHeroConstPtr hero, Side side)
        : library(adventure->library), adventure(adventure), hero(hero), side(side)
        {   count = adventure->count; }

    // main params

    LibraryUnitConstPtr library = nullptr;
    AdventureStackConstPtr adventure = nullptr;
    BattleHeroConstPtr hero = nullptr;


    const Side side = Side::Attacker;

    // estimated params
    struct EstimatedParams {
        UnitPrimaryParams primary;
        PrimaryAttackParams adMelee ;
        PrimaryAttackParams adRanged;
        PrimaryRngParams rngParams;
        PrimaryRngParams rngMax;
        bool hasMorale = false;
        int maxRetaliations = 0;
        int maxAttacksMelee = 0;
        int maxAttacksRanged = 0;
        bool hasBuff = false;
        bool hasDebuff = false;

        BonusRatio meleeAttack    = {0, 1};          // spells
        BonusRatio rangedAttack   = {0, 1};          // spells
        BonusRatio meleeDefense   = {0, 1};          // spells
        BonusRatio rangedDefense  = {0, 1};          // spells

        MagicReduce  magicReduce;
        BonusRatio   magicOppSuccessChance = {1,1}; // 1 - resist chance.
        SpellFilter  immunes; // common or without breakable

        //bool isAlive = false;
        bool canDoAnything = false;
        bool canCast =  false;
        bool canMove = false;
        bool canAttackMelee = false;
        bool canAttackRanged = false;
        bool canAttackFreeSplash = false;
    };
    EstimatedParams estimatedOnStart;  // adventure->estimated + land bonus/castle bonus + hero.rngParamsOppBonus + environment

    // variable params

    int count = 0;
    int health = 0;
    int remainingShoots = 0;

    void applyLoss(DamageResult::Loss loss) {
        count  = loss.remainCount;
        health = loss.remainCount > 0 ? loss.remainTopStackHealth : 0;
    }

    struct RoundState {
        int baseRoll = -1;
        int baseRollCount = 0;
        bool finishedTurn = false;
        bool waited = false;
        bool hadHighMorale = false;
        bool hadLowMorale = false;
       // bool guarded = false;
        int retaliationsDone = 0;
    };
    RoundState roundState;

    int speedOrder = 0;
    int sameSpeedOrder = 0;

    BattlePositionExtended pos;

    std::vector<BattleStackConstPtr> aliveNeighbours;

    struct Effect {
        enum class Type { Spell, Guard };
        Type type = Type::Spell;
        Effect() = default;
        explicit Effect(SpellCastParams sc, int remainRounds = 0) : type(Type::Spell), power(sc), roundsRemain(remainRounds) {}
        explicit Effect(Type type, int v) : type(type), value(v) {}

        SpellCastParams power; // spell
        int roundsRemain = 0; // spell
        int value = 0; // guard
    };
    using EffectList = std::vector<Effect>;
    EffectList appliedEffects;

    EstimatedParams current;           // estimatedOnStart + temp spells


    constexpr auto turnOrder() const noexcept {
        return std::tie(roundState.waited, speedOrder, sameSpeedOrder, side);
    }
    constexpr bool isAlive() const noexcept {
        return count > 0;
    }
};


}
