/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleEstimation.hpp"
#include "GeneralEstimation.hpp"

#include "LibraryGameRules.hpp"

#include <sol/sol.hpp>

namespace FreeHeroes::Core {

void BattleEstimation::bindTypes(sol::state& lua)
{
    GeneralEstimation(m_rules).bindTypes(lua);

    lua.new_usertype<BattleStack::EstimatedParams>( "BattleUnitEstimatedParams",
       "primary"      , &BattleStack::EstimatedParams::primary,
       "rng"          , &BattleStack::EstimatedParams::rngParams,

       "adMelee"      , &BattleStack::EstimatedParams::adMelee   ,
       "adRanged"     , &BattleStack::EstimatedParams::adRanged  ,

       "meleeAttack"  , &BattleStack::EstimatedParams::meleeAttack   ,
       "rangedAttack" , &BattleStack::EstimatedParams::rangedAttack  ,
       "meleeDefense" , &BattleStack::EstimatedParams::meleeDefense  ,
       "rangedDefense", &BattleStack::EstimatedParams::rangedDefense ,

       "magicReduce"  , &BattleStack::EstimatedParams::magicReduce ,

       "maxRetaliations"  , &BattleStack::EstimatedParams::maxRetaliations ,

       "canCast"          , &BattleStack::EstimatedParams::canCast,
       "canMove"          , &BattleStack::EstimatedParams::canMove,
       "canAttackMelee"   , &BattleStack::EstimatedParams::canAttackMelee,
       "canAttackRanged"  , &BattleStack::EstimatedParams::canAttackRanged

   );
}


void BattleEstimation::calculateUnitStats(BattleStack& unit)
{
    if (unit.count <= 0)
        return;

    unit.current = unit.estimatedOnStart;
    auto & cur = unit.current;
    if (cur.rngMax.morale < 3)
        cur.rngParams.morale = std::min(cur.rngParams.morale, cur.rngMax.morale);
    if (cur.rngMax.luck < 3)
        cur.rngParams.luck = std::min(cur.rngParams.luck, cur.rngMax.luck);

//    Logger(Logger::Info) << "start stack:" << unit.library->id
//            << ", maxRetaliations=" << cur.maxRetaliations
//                            ;

    sol::state lua;

    bool hasBuff = false;
    bool hasDebuff = false;

    cur.canMove = true;
    cur.canCast = false;
    cur.canAttackMelee =  true;
    cur.canAttackRanged =  unit.library->traits.rangeAttack && unit.remainingShoots > 0;

    // bindings
    bindTypes(lua);

    std::vector<BattleStack::Effect> effectsTmp;

    // remove outdated;
    {
        effectsTmp.clear();
        for (auto & effect : unit.appliedEffects) {
            if (effect.roundsRemain <= 0)
                continue;
            effectsTmp.push_back(effect);
        }
        unit.appliedEffects = effectsTmp;
    }

    // remove duplicates - leave only the last applied;
    // also , if after spell was applied counterspell - remove it too.
    {
        effectsTmp.clear();
        auto tmp = unit.appliedEffects;
        std::set<LibrarySpellConstPtr> used;
        std::reverse(tmp.begin(), tmp.end());
        for (auto & effect : tmp) {
            if (effect.type != BattleStack::Effect::Type::Spell) {
                effectsTmp.push_back(effect);
                continue;
            }
            if (used.count(effect.power.spell))
                continue;
            const bool counterSpellIsUsed = [&used, &effect]() -> bool{
                for (auto spell : effect.power.spell->counterSpells) {
                    if (used.count(spell))
                        return true;
                }
                return false;
            }();
            if (counterSpellIsUsed)
                continue;

            used.insert(effect.power.spell);
            effectsTmp.push_back(effect);
        }
        std::reverse(effectsTmp.begin(), effectsTmp.end());
        unit.appliedEffects = effectsTmp;
    }
    lua["u"] = cur;

    for (auto & effect : unit.appliedEffects) {
        if (effect.type == BattleStack::Effect::Type::Guard) {
            cur.primary.ad.defense += effect.value;
            lua["u"] = cur;
            continue;
        }
        const bool isBuff   = effect.power.spell->qualify == LibrarySpell::Qualify::Good;
        const bool isDebuff = effect.power.spell->qualify == LibrarySpell::Qualify::Bad;
        const bool isSomething = isBuff || isDebuff;
        assert(isSomething);
        if (!isSomething)
            continue;
        hasBuff   = hasBuff   || isBuff;
        hasDebuff = hasDebuff || isDebuff;
        lua["level"] = effect.power.skillLevel;
        lua["isSpec"] = effect.power.heroSpecLevel != -1;
        lua["unitLevel"] = unit.library->level / 10;
        lua["heroLevel"] = effect.power.heroSpecLevel;

        for (const auto & calc : effect.power.spell->calcScript)
            lua.script(calc);
    }
    cur = lua["u"];

    cur.hasBuff = hasBuff;
    cur.hasDebuff = hasDebuff;

    cur.canAttackFreeSplash = cur.canAttackRanged && unit.library->abilities.splashType == LibraryUnit::Abilities::SplashAttack::Ranged;
    cur.canDoAnything   = cur.canMove || cur.canAttackMelee || cur.canAttackRanged || cur.canCast;

    cur.primary.ad.attack   = std::clamp(cur.primary.ad.attack         , 0, m_rules->limits.maxUnitAd.attack );
    cur.primary.ad.defense  = std::clamp(cur.primary.ad.defense        , 0, m_rules->limits.maxUnitAd.defense);

    cur.moraleChance = GeneralEstimation(m_rules).estimateMoraleRoll(cur.rngParams.morale, cur.rngChances);
    cur.luckChance   = GeneralEstimation(m_rules).estimateLuckRoll  (cur.rngParams.luck  , cur.rngChances);

}

bool BattleEstimation::checkSpellTarget(const BattleStack& possibleTarget, LibrarySpellConstPtr spell)
{
    if (!possibleTarget.current.immunes.isDefault() && possibleTarget.current.immunes.contains(spell))
        return false;

    sol::state lua;

    bindTypes(lua);

    lua["result"] = true;
    lua["type"] = possibleTarget.library->abilities.type;
    lua["nonLivingType"] = possibleTarget.library->abilities.nonLivingType;

    for (const auto & calc : spell->filterScript)
        lua.script(calc);

    bool result = lua["result"];
    return result;
}

bool BattleEstimation::checkAttackElementPossibility(const BattleStack& possibleTarget, LibraryUnit::Abilities::AttackWithElement element)
{
    if (element == LibraryUnit::Abilities::AttackWithElement::Fire) {
        if (!possibleTarget.current.immunes.isDefault() && possibleTarget.current.immunes.contains(MagicSchool::Fire))
            return false;
    }
    if (element == LibraryUnit::Abilities::AttackWithElement::Undead) {
        if (possibleTarget.library->abilities.type != UnitType::Living)
            return false;
    }

    return true;
}

void BattleEstimation::calculateHeroStatsStartBattle(BattleHero& hero, const BattleSquad & squad, const BattleArmy & opponent)
{
    if (!hero.isValid())
        return;

    hero.mana = hero.adventure->mana;
    hero.estimated.primary   = hero.adventure->estimated.primary;
    if (opponent.battleHero.isValid()) {
        auto & spellPower = hero.estimated.primary.magic.spellPower;
        spellPower = BonusRatio::calcSubDecrease(spellPower, opponent.battleHero.adventure->estimated.spReduceOpp, 1);
    }
    // @todo: animag garrisons, antimag sphere, etc.
    hero.estimated.availableSpells = hero.adventure->estimated.availableSpells;
    hero.estimated.squadRngParams  = squad.adventure->estimated.squadBonus.rngParams;
    for (auto & spell : hero.estimated.availableSpells) {
        if (!spell.manaCost)
            continue;
        spell.manaCost += squad.adventure->estimated.squadBonus.manaCost;
        spell.manaCost += opponent.squad->adventure->estimated.oppBonus.manaCost;
        spell.manaCost = std::max(0, spell.manaCost);
    }
}



void BattleEstimation::calculateUnitStatsStartBattle(BattleStack& unit, const BattleSquad& squad, const BattleArmy& opponent)
{
    unit.estimatedOnStart.maxRetaliations = unit.library->abilities.maxRetaliations;

    unit.estimatedOnStart.primary   = unit.adventure->estimated.primary;   // library + hero ad/speed/life bonus
    unit.estimatedOnStart.rngParams = unit.adventure->estimated.rngParams; // hero + own squad
    unit.estimatedOnStart.hasMorale = unit.adventure->estimated.hasMorale;
    unit.estimatedOnStart.rngMax    = squad.adventure->estimated.rngMax;
    unit.estimatedOnStart.rngChances    = squad.adventure->estimated.squadBonus.rngChance;
    unit.estimatedOnStart.magicOppSuccessChance = unit.adventure->estimated.magicOppSuccessChance;
    unit.estimatedOnStart.magicReduce           = unit.adventure->estimated.magicReduce;
    unit.estimatedOnStart.immunes               = unit.adventure->estimated.immunes;
    if (0) { // @todo: black sphere
        unit.estimatedOnStart.immunes           = unit.adventure->estimated.immunesWithoutBreakable;
    }

    const auto & oppEstim = opponent.squad->adventure->estimated;
    unit.estimatedOnStart.rngParams += oppEstim.oppBonus.rngParams;
    unit.estimatedOnStart.rngMax.luck   = std::min(unit.estimatedOnStart.rngMax.luck  , oppEstim.rngMax.luck);
    unit.estimatedOnStart.rngMax.morale = std::min(unit.estimatedOnStart.rngMax.morale, oppEstim.rngMax.morale);
    if (!unit.estimatedOnStart.hasMorale)
        unit.estimatedOnStart.rngParams.morale = 0;

    unit.estimatedOnStart.rngChances.luck      *= oppEstim.oppBonus.rngChance.luck;
    unit.estimatedOnStart.rngChances.morale    *= oppEstim.oppBonus.rngChance.morale;
    unit.estimatedOnStart.rngChances.unluck    *= oppEstim.oppBonus.rngChance.unluck;
    unit.estimatedOnStart.rngChances.dismorale *= oppEstim.oppBonus.rngChance.dismorale;

    unit.estimatedOnStart.maxAttacksMelee = 1; // @todo: ballista. ?
    if (unit.library->traits.rangeAttack)
        unit.estimatedOnStart.maxAttacksRanged = 1;
    if (unit.library->traits.doubleAttack) {
        if (unit.library->traits.rangeAttack)
            unit.estimatedOnStart.maxAttacksRanged = 2;
        else
            unit.estimatedOnStart.maxAttacksMelee = 2;
    }

    calculateUnitStats(unit);

    unit.health          = unit.current.primary.maxHealth;
    unit.remainingShoots = unit.current.primary.shoots;

    unit.roundState = {};
}


void BattleEstimation::calculateArmyOnBattleStart(BattleArmy& army, const BattleArmy& opponent)
{
    if (army.battleHero.isValid()) {
        calculateHeroStatsStartBattle(army.battleHero, *army.squad, opponent);
    }
    for (auto & stack : army.squad->stacks) {
        calculateUnitStatsStartBattle(stack, *army.squad, opponent);
    }
}

void BattleEstimation::calculateArmyOnRoundStart(BattleArmy& army)
{
    // apply healing for trolls

    // reset abilities
    // decrement effects (and remove them)

    for (auto & stack : army.squad->stacks) {
        if (!stack.isAlive())
            continue;

        stack.roundState = {};

        for (auto & eff : stack.appliedEffects)
            eff.roundsRemain--;

        calculateUnitStats(stack);

    }

    if (army.battleHero.isValid())
        army.battleHero.castedInRound = false;
}



}
