/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Stat.hpp"
#include "LibraryFwd.hpp"
#include "LibraryResource.hpp"
#include "LibrarySpell.hpp"

#include <string>
#include <vector>
#include <set>

namespace FreeHeroes::Core {

// clang-format off
enum class UnitType { Living, NonLiving, SiegeMachine, ArrowTower, Wall, Unknown };
enum class UnitNonLivingType { None, Undead, Golem, Gargoyle, Elemental, BattleMachine };
// clang-format on

struct LibraryUnit {
    struct Abilities {
        // clang-format off
        enum class SplashAttack { None, Dragon, Neighbours, Sides, Ranged };
        enum class AttackWithElement { None, Fire, Earth, Air, Ice, Mind, Magic, Undead };
        // clang-format on

        struct ExtraDamage {
            std::vector<LibraryUnitConstPtr> enemies;
            BonusRatio                       damageBonus{ 0, 1 };
            bool                             isEmpty() const noexcept { return enemies.empty() || damageBonus == BonusRatio{ 0, 1 }; }
        };
        struct StatBonus {
            int                 morale   = 0;
            int                 luck     = 0;
            int                 manaCost = 0;
            RngChanceMultiplier rngMult;
        };

        struct CastOnHit {
            SpellCastParams params;
            bool            melee  = true;
            bool            ranged = true;
            BonusRatio      chance{ 1, 1 };
        };
        using CastsOnHit = std::vector<CastOnHit>;
        struct FixedCast {
            SpellCastParams params;
            int             count = 0;
        };

        UnitType          type          = UnitType::Living;
        UnitNonLivingType nonLivingType = UnitNonLivingType::None;

        std::vector<RangeAttackPenalty> disabledPenalties;

        SplashAttack         splashType         = SplashAttack::None;
        bool                 splashFriendlyFire = false;
        std::string          splashButtons;
        LibrarySpellConstPtr splashSpell   = nullptr;
        AttackWithElement    splashElement = AttackWithElement::None;
        [[nodiscard]] bool   hasMeleeSplash() const noexcept { return splashType == SplashAttack::Dragon || splashType == SplashAttack::Sides || splashType == SplashAttack::Neighbours; }

        int  maxRetaliations = 1;
        bool chargeAttack    = false;

        StatBonus squadBonus;
        StatBonus opponentBonus;

        ExtraDamage       extraDamage;
        AttackWithElement attackWithElement        = AttackWithElement::None;
        AttackWithElement vulnerableAgainstElement = AttackWithElement::None;

        //bool bindOnAttack = false;

        //int giantLevel = 0; // used for Slayer spell.

        //int doubleDamageChance = 0;

        int minimalMoraleLevel = -99;
        int minimalLuckLevel   = -99;

        //int manaStealPercent = 0;

        BonusRatio reduceTargetDefense{ 1, 1 };
        BonusRatio reduceAttackerAttack{ 1, 1 };

        SpellFilter vulnerable;
        BonusRatio  vulnerableBonus = { 1, 1 }; // 1/1 => +100% of base damage.

        SpellFilter immunes;
        bool        immuneBreakable = true;

        MagicReduce magicReduce;
        BonusRatio  magicOppSuccessChance           = { 1, 1 };
        BonusRatio  magicOppSuccessChanceNeighbours = { 1, 1 };

        bool regenerate = false;

        CastsOnHit castsOnHit;
        FixedCast  fixedCast;

        ResourceAmount weekIncome;
    };
    struct Traits {
        bool large             = false; // 2hex
        bool rangeAttack       = false;
        bool fly               = false;
        bool teleport          = false;
        bool doubleAttack      = false;
        bool freeAttack        = false; // no retaliation
        bool canBeCatapult     = false;
        bool returnAfterAttack = false;
    };
    struct HeroStackSize {
        int  min = 0;
        int  max = 0;
        bool isValid() const { return min > 0 && max > 0; }
    };
    struct Presentation {
        std::string spriteBattle;
        std::string portrait;
        std::string portraitSmall;

        std::string soundId;
        bool        soundHasShoot         = false;
        bool        soundHasMovementStart = false;

        std::string spriteProjectile;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    UnitPrimaryParams primary;

    LibraryFactionConstPtr faction = nullptr;

    int level = 0;

    int                              growth = 0;
    HeroStackSize                    countWithHeroBase;
    ResourceAmount                   cost;
    int                              value = 0;
    std::vector<LibraryUnitConstPtr> upgrades;

    LibraryUnitConstPtr prevUpgrade = nullptr;
    LibraryUnitConstPtr baseUpgrade = nullptr;

    LibraryArtifactConstPtr battleMachineArtifact = nullptr;

    Traits                   traits;
    Abilities                abilities;
    Presentation             presentationParams;
    LibraryObjectDefConstPtr mapObjectDef = nullptr;
};

}
