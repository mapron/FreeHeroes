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
        RngChanceParams  rngChances;
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

        BonusRatio retaliationPower = {1, 1};

        BonusRatio luckChance;
        BonusRatio moraleChance;

        MagicReduce  magicReduce;
        BonusRatio   magicOppSuccessChance = {1,1}; // 1 - resist chance.
        SpellFilter  immunes; // common or without breakable
        LibraryUnit::Abilities::CastsOnHit castsOnHit;
        LibraryUnit::Abilities::FixedCast fixedCast;


        bool canDoAnything = false;
        bool canCast =  false;
        bool canMove = false;
        bool canAttackMelee = false;
        bool canAttackRanged = false;
        bool canAttackFreeSplash = false;
        bool rangeAttackIsBlocked = false;
    };
    EstimatedParams estimatedOnStart;  // adventure->estimated + land bonus/castle bonus + hero.rngParamsOppBonus + environment

    // variable params

    int count = 0;
    int health = 0;
    int remainingShoots = 0;
    int castsDone = 0;

    struct RoundState {
        int baseRoll = -1;
        int baseRollCount = 0;
        bool finishedTurn = false;
        bool waited = false;
        bool hadHighMorale = false;
        bool hadLowMorale = false;
       // bool guarded = false;
        int retaliationsDone = 0;
        int guardBonus = 0;
    };
    RoundState roundState;

    int speedOrder = 0;
    int sameSpeedOrder = 0;

    BattlePositionExtended pos;

    struct Effect {
        explicit Effect(SpellCastParams sc, int remainRounds = 0) : power(sc), roundsRemain(remainRounds) {}

        SpellCastParams power; // spell
        int roundsRemain = 0; // spell
    };
    using EffectList = std::vector<Effect>;
    EffectList appliedEffects;

    EstimatedParams current;           // estimatedOnStart + temp spells


    constexpr auto turnOrder() const noexcept {
        return std::tie(speedOrder, sameSpeedOrder, side);
    }
    constexpr bool isAlive() const noexcept {
        return count > 0;
    }
};


}
