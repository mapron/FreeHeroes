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

enum class UnitType { Living, NonLiving, SiegeMachine, ArrowTower, Wall, Unknown };
enum class UnitNonLivingType { None, Undead, Golem, Gargoyle, Elemental, BattleMachine };

struct LibraryUnit {
    struct Abilities {
        enum class DamagePenalty { Melee, Distance, Obstacle };
        enum class SplashAttack { None, Dragon, Neighbours, Sides, Ranged };
        enum class AttackWithElement { None, Fire, Earth, Air, Ice, Mind, Magic, Undead };

        struct Hate {
            std::set<std::string> enemyIds;
            int bonusPercent = 0;
        };

        UnitType type = UnitType::Living;
        UnitNonLivingType nonLivingType = UnitNonLivingType::None;


        std::vector<DamagePenalty> disabledPenalties;
        [[nodiscard]] bool hasPenalty(DamagePenalty penalty) const noexcept{  return std::find(disabledPenalties.cbegin(), disabledPenalties.cend(), penalty) == disabledPenalties.cend(); }

        SplashAttack splashType = SplashAttack::None;
        std::string splashButtons;
        std::string splashAnimation;

        int maxRetaliations = 1;
        bool chargeAttack  = false;

        int increaseHeroMorale = 0;
        int decreaseEnemyMorale = 0;
        int increaseHeroLuck = 0;
        int decreaseEnemyLuck = 0;

        Hate hate;

        int increaseEnemyManaCost = 0;
        int decreaseHeroManaCost = 0;

        bool bindOnAttack = false;

        int giantLevel = 0;

        int doubleDamageChance = 0;

        int minimalMoraleLevel = -3;
        int minimalLuckLevel = -3;

        int manaStealPercent = 20;

        int ignoreTargetDefense = 0;
        int ignoreAttackerAttack = 0;

        std::vector<SpellCastParams> casts;

        SpellFilter vulnerable;
        BonusRatio vulnerableRatio = {1,1};

        SpellFilter immunes;
        bool immuneBreakable = true;

        MagicReduce magicReduce;
        BonusRatio magicOppSuccessChance = {1,1};
        BonusRatio magicOppSuccessChanceNeighbours = {1,1};

        ResourceAmount weekIncome;

        AttackWithElement attackWithElement = AttackWithElement::None;

    };
    struct Traits {
        bool large = false; // 2hex
        bool rangeAttack = false;
        bool fly = false;
        bool teleport = false;
        bool doubleAttack = false;
        bool freeAttack = false; // no retaliation
        bool canBeCatapult = false;
        bool returnAfterAttack = false;
    };
    struct HeroStackSize {
        int min = 0;
        int max = 0;
        bool isValid() const { return min > 0 && max > 0;}
    };
    struct Presentation {
        std::string spriteAdventure;

        std::string spriteBattle;
        std::string portrait;
        std::string portraitSmall;

        std::string soundId;
        bool soundHasShoot = false;
        bool soundHasMovementStart = false;

        std::string spriteProjectile;
    };

    std::string id;
    std::string untranslatedName;

    UnitPrimaryParams  primary;

    LibraryFactionConstPtr faction = nullptr;

    int level = 0;

    int growth = 0;
    HeroStackSize countWithHeroBase;
    ResourceAmount cost;
    int value = 0;
    std::vector<LibraryUnitConstPtr> upgrades;

    LibraryUnitConstPtr prevUpgrade = nullptr;
    LibraryUnitConstPtr baseUpgrade = nullptr;

    LibraryArtifactConstPtr battleMachineArtifact = nullptr;

    Traits                 traits;
    Abilities              abilities;
    Presentation           presentationParams;
};

}
