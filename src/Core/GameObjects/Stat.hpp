/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BonusRatio.hpp"

#include <tuple>
#include <utility>
#include <algorithm>

namespace FreeHeroes::Core {

namespace internal
{
    template<template<typename> typename Operator, typename T, typename T2, size_t... Is>
    constexpr void opTuplesImpl(T&& t1, const T2& t2, std::integer_sequence<size_t, Is...>) {
        using namespace std;
        using TnoRef = std::remove_reference_t<T>;
        [[maybe_unused]] auto l = { ((std::get<Is>(t1) = Operator<std::remove_reference_t<std::tuple_element_t<Is, TnoRef>>>()(std::get<Is>(t1), std::get<Is>(t2))), 0)... };
    }

    template<typename ...T, typename T2>
    constexpr void addTuples(std::tuple<T...> && t1, const T2& t2) {
        opTuplesImpl<std::plus>(t1, t2, std::index_sequence_for<T...>{});
    }
    template<typename ...T, typename T2>
    constexpr void subTuples(std::tuple<T...> && t1, const T2& t2) {
        opTuplesImpl<std::minus>(t1, t2, std::index_sequence_for<T...>{});
    }

    /// I need badly Herb's Metaclasses: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0707r3.pdf
    /// Then instead of
    /// struct PrimaryRngParams : public internal::MakeAggregate<PrimaryRngParams> {}
    /// We can just
    /// value PrimaryRngParams {}
    ///
    /// @note: valarray also works, but no names for fields then.
    template <typename T>
    struct MakeAggregate {
        constexpr const T & self() const { return static_cast<const T&>(*this);}
        constexpr       T & self()       { return static_cast<T&>(*this);}

        // that line is not working for clang, but gcc is ok.
        // friend constexpr auto operator <=>(const T& lh, const T& rh) noexcept { return lh.asTuple() <=> rh.asTuple(); }
        friend constexpr bool operator ==(const T& lh, const T& rh) noexcept { return lh.asTuple() == rh.asTuple(); }
        friend constexpr bool operator !=(const T& lh, const T& rh) noexcept { return !(lh == rh); }

        constexpr T operator + (const T& rh) const noexcept {
            T result = self();
            internal::addTuples(result.asTuple(), rh.asTuple());
            return result;
        }
        constexpr T operator - (const T& rh) const noexcept {
            T result = self();
            internal::subTuples(result.asTuple(), rh.asTuple());
            return result;
        }
        constexpr T operator - () const noexcept {
            T result{};
            internal::subTuples(result.asTuple(), self().asTuple());
            return result;
        }
        constexpr T & operator += (const T& rh) noexcept {
            internal::addTuples(self().asTuple(), rh.asTuple());
            return self();
        }
        constexpr T & operator -= (const T& rh) noexcept {
            internal::subTuples(self().asTuple(), rh.asTuple());
            return self();
        }
    };
}

struct PrimaryRngParams : public internal::MakeAggregate<PrimaryRngParams> {
    int luck = 0;
    int morale = 0;

    void incLuck  (int value) { luck += value; }
    void incMorale(int value) { morale += value; }
    void incAll   (int value) { luck += value; morale += value; }

    constexpr auto asTuple() const noexcept { return std::tie(luck, morale);}
    constexpr auto asTuple() noexcept       { return std::tie(luck, morale);}
};

struct RngChanceParams : public internal::MakeAggregate<RngChanceParams> {
    BonusRatio luck     {1,1};
    BonusRatio morale   {1,1};
    BonusRatio unluck   {1,1};
    BonusRatio dismorale{1,1};

    constexpr auto asTuple() const noexcept { return std::tie(luck, morale, unluck, dismorale);}
    constexpr auto asTuple() noexcept       { return std::tie(luck, morale, unluck, dismorale);}
};

struct PrimaryAttackParams : public internal::MakeAggregate<PrimaryAttackParams> {
    int attack = 0;
    int defense = 0;

    void incAtt  (int value) { attack += value; }
    void incDef  (int value) { defense += value; }
    void incAll  (int value) { attack += value; defense += value; }

    constexpr auto asTuple() const noexcept { return std::tie(attack, defense);}
    constexpr auto asTuple() noexcept       { return std::tie(attack, defense);}
};

struct PrimaryMagicParams : public internal::MakeAggregate<PrimaryMagicParams> {
    int spellPower = 0;
    int intelligence = 0;

    void incSP   (int value) { spellPower += value; }
    void incInt  (int value) { intelligence += value; }
    void incAll  (int value) { spellPower += value; intelligence += value; }

    constexpr auto asTuple() const noexcept { return std::tie(spellPower, intelligence);}
    constexpr auto asTuple() noexcept       { return std::tie(spellPower, intelligence);}
};

struct DamageDesc : public internal::MakeAggregate<DamageDesc>  {
    int minDamage = 0;
    int maxDamage = 0;

    constexpr auto asTuple() const noexcept { return std::tie(minDamage, maxDamage);}
    constexpr auto asTuple() noexcept       { return std::tie(minDamage, maxDamage);}
};

struct UnitPrimaryParams : public internal::MakeAggregate<UnitPrimaryParams> {
    DamageDesc dmg;
    PrimaryAttackParams ad;

    int maxHealth = 0;
    int battleSpeed = 0;
    int armySpeed = 0;
    int shoots = 0;

    constexpr auto asTuple() const noexcept { return std::tie(dmg, ad, maxHealth, battleSpeed, armySpeed, shoots);}
    constexpr auto asTuple() noexcept       { return std::tie(dmg, ad, maxHealth, battleSpeed, armySpeed, shoots);}
};

enum class HeroPrimaryParamType { Attack, Defense, SpellPower, Intelligence, Experience, Mana };

struct HeroPrimaryParams : public internal::MakeAggregate<HeroPrimaryParams> {
    PrimaryAttackParams ad;
    PrimaryMagicParams magic;

    void incAll  (int value) { ad.incAll(value); magic.incAll(value); }

    constexpr auto asTuple() const noexcept { return std::tie(ad, magic);}
    constexpr auto asTuple() noexcept       { return std::tie(ad, magic);}
};

struct MoraleDetails : public internal::MakeAggregate<MoraleDetails> {
    int undead = 0;
    int extraUnwantedFactions = 0;
    int factionsPenalty = 0;  // morale bonus by extraFactions
    int unitBonus = 0;

    int skills = 0;
    int artifacts = 0;

    bool unaffected = false;

    int total = 0;
    int minimalMoraleLevel = -3;

    bool neutralizedPositive = false;

    BonusRatio rollChance {0,1};

    // @todo: isn't it weird not to have all the fields here?
    constexpr auto asTuple() const noexcept { return std::tie(undead, extraUnwantedFactions, factionsPenalty, unitBonus, skills, artifacts);}
    constexpr auto asTuple() noexcept       { return std::tie(undead, extraUnwantedFactions, factionsPenalty, unitBonus, skills, artifacts);}
};
struct LuckDetails : public internal::MakeAggregate<LuckDetails>  {
    int unitBonus = 0;

    int skills = 0;
    int artifacts = 0;

    int total = 0;
    int minimalLuckLevel = -3;

    bool neutralizedPositive = false;

    BonusRatio rollChance {0,1};

    // @todo: isn't it weird not to have all the fields here?
    constexpr auto asTuple() const noexcept { return std::tie(unitBonus, skills, artifacts);}
    constexpr auto asTuple() noexcept       { return std::tie(unitBonus, skills, artifacts);}
};


struct DamageResult {

   int damageBaseRoll = 0;
   int damagePercent = 0;
   //int damageTotal = 0;
   struct Loss {
       int damageTotal = 0;
       int deaths = 0;
       int remainCount = 0;
       int remainTopStackHealth = 0;
   };
   Loss loss;

   bool isKilled() const { return loss.remainCount <= 0; }
};

struct DamageEstimate {
   bool isValid = false;
   DamageResult lowRoll;
   DamageResult avgRoll;
   DamageResult maxRoll;
};

enum class RangeAttackPenalty { Melee, Distance, Obstacle, Blocked };

enum class MagicSchool { Any, Air, Earth, Fire, Water };

struct MagicSchoolLevels {

    int air   = 0;
    int earth = 0;
    int fire  = 0;
    int water = 0;

    [[nodiscard]] constexpr int getMax() const noexcept{ return std::max({air, earth, fire, water});}
    [[nodiscard]] constexpr int getLevelForSpell(MagicSchool spell) const noexcept {
        switch(spell) {
            case MagicSchool::Any:   return getMax();
            case MagicSchool::Air:   return air;
            case MagicSchool::Earth: return earth;
            case MagicSchool::Fire:  return fire;
            case MagicSchool::Water: return water;
        }
        return 0;
    }
};

struct MagicReduce : public internal::MakeAggregate<MagicReduce>  {
    BonusRatio allMagic = {1, 1};
    BonusRatio air      = {1, 1};
    BonusRatio earth    = {1, 1};
    BonusRatio fire     = {1, 1};
    BonusRatio water    = {1, 1};

    constexpr auto asTuple() const noexcept { return std::tie(allMagic, air, earth, fire, water);}
    constexpr auto asTuple() noexcept       { return std::tie(allMagic, air, earth, fire, water);}

    [[nodiscard]] constexpr BonusRatio getMax() const noexcept{ return std::max({air, earth, fire, water});}
    [[nodiscard]] constexpr BonusRatio getReduceForSpell(MagicSchool spell) const noexcept {
        switch(spell) {
            case MagicSchool::Any:   return getMax() * allMagic;
            case MagicSchool::Air:   return air      * allMagic;
            case MagicSchool::Earth: return earth    * allMagic;
            case MagicSchool::Fire:  return fire     * allMagic;
            case MagicSchool::Water: return water    * allMagic;
        }
        return {1, 1};
    }
    bool isAllSchoolsEqual() const noexcept {
        return air == earth && air == fire && air == water;
    }
    bool isDefault() const noexcept {
        return asTuple() == std::tuple{BonusRatio{1,1},BonusRatio{1,1},BonusRatio{1,1},BonusRatio{1,1},BonusRatio{1,1}};
    }
};

struct MagicIncrease : public internal::MakeAggregate<MagicIncrease>  {
    BonusRatio allMagic = {0, 1};
    BonusRatio air      = {0, 1};
    BonusRatio earth    = {0, 1};
    BonusRatio fire     = {0, 1};
    BonusRatio water    = {0, 1};

    constexpr auto asTuple() const noexcept { return std::tie(allMagic, air, earth, fire, water);}
    constexpr auto asTuple() noexcept       { return std::tie(allMagic, air, earth, fire, water);}

    [[nodiscard]] constexpr BonusRatio getMax() const noexcept{ return std::max({air, earth, fire, water});}
    [[nodiscard]] constexpr BonusRatio getIncreaseForSpell(MagicSchool spell) const noexcept {
        switch(spell) {
            case MagicSchool::Any:   return getMax() + allMagic;
            case MagicSchool::Air:   return air      + allMagic;
            case MagicSchool::Earth: return earth    + allMagic;
            case MagicSchool::Fire:  return fire     + allMagic;
            case MagicSchool::Water: return water    + allMagic;
        }
        return {0, 1};
    }
};

struct ResourceAmount : public internal::MakeAggregate<ResourceAmount>  {
    int gold    = 0;

    int wood    = 0;
    int ore     = 0;

    int mercury = 0;
    int sulfur  = 0;
    int crystal = 0;
    int gems    = 0;

    constexpr auto asTuple() const noexcept { return std::tie(gold, wood, ore, mercury, sulfur, crystal, gems);}
    constexpr auto asTuple() noexcept       { return std::tie(gold, wood, ore, mercury, sulfur, crystal, gems);}

    void maxWith(const ResourceAmount & another) {
        gold     = std::max(gold    , another.gold    );
        wood     = std::max(wood    , another.wood    );
        ore      = std::max(ore     , another.ore     );
        mercury  = std::max(mercury , another.mercury );
        sulfur   = std::max(sulfur  , another.sulfur  );
        crystal  = std::max(crystal , another.crystal );
        gems     = std::max(gems    , another.gems    );
    }

    constexpr size_t nonEmptyAmount() const noexcept { return (gold != 0) + (wood != 0) + (ore != 0) + (mercury != 0) + (sulfur != 0) + (crystal != 0) + (gems != 0);}
};


}
