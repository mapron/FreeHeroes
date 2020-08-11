/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GeneralEstimation.hpp"

#include "LibraryUnit.hpp"
#include "IRandomGenerator.hpp"

#include <sol/sol.hpp>

namespace FreeHeroes::Core {

namespace {
const std::map<int, int64_t> levelTable = []()->std::map<int, int64_t>{
    std::map<int, int64_t> start {
        {1,  0     },
        {2,  1000  },
        {3,  2000  },
        {4,  3200  },
        {5,  4600  },
        {6,  6200  },
        {7,  8000  },
        {8,  10000 },
        {9,  12200 },
        {10, 14700 },
        {11, 17500 },
        {12, 20600 },
        {13, 24320 },
    };
    int64_t current = 24320;
    for (int i = 14; i < 100; i++) {
        current = current * 6 / 5;
        start[i] = current;
    }
    return start;
} ();
const std::map<int64_t, int> levelTableInv = []()->std::map<int64_t, int>{
    std::map<int64_t, int> ret;
    for (auto p : levelTable)
        ret[p.second] = p.first;
    return ret;
}();

}


void GeneralEstimation::bindTypes(sol::state & lua)
{
    lua.open_libraries(sol::lib::math);
    lua.new_usertype<MagicSchoolLevels>("MagicSchoolLevels",
        "air"              , &MagicSchoolLevels::air  ,
        "earth"            , &MagicSchoolLevels::earth,
        "fire"             , &MagicSchoolLevels::fire ,
        "water"            , &MagicSchoolLevels::water
    );

    lua.new_usertype<BonusRatio>( "BonusRatio",
        "set"  , &BonusRatio::set,
        "add"  , &BonusRatio::add,
        "mult" , &BonusRatio::mult,
        "num"  , &BonusRatio::num,
        "denom", &BonusRatio::denom
    );
    lua.new_usertype<PrimaryRngParams>( "PrimaryRngParams",
        "luck"     , &PrimaryRngParams::luck,
        "morale"   , &PrimaryRngParams::morale,
        "incLuck"  , &PrimaryRngParams::incLuck,
        "incMorale", &PrimaryRngParams::incMorale,
        "incAll"   , &PrimaryRngParams::incAll
    );
    lua.new_usertype<PrimaryAttackParams>( "PrimaryAttackParams",
        "att"      , &PrimaryAttackParams::attack,
        "def"      , &PrimaryAttackParams::defense,
        "incAtt"   , &PrimaryAttackParams::incAtt,
        "incDef"   , &PrimaryAttackParams::incDef,
        "incAll"   , &PrimaryAttackParams::incAll
    );
    lua.new_usertype<PrimaryMagicParams>( "PrimaryMagicParams",
        "sp"      , &PrimaryMagicParams::spellPower,
        "int"     , &PrimaryMagicParams::intelligence,
        "incSP"   , &PrimaryMagicParams::incSP,
        "incInt"  , &PrimaryMagicParams::incInt,
        "incAll"  , &PrimaryMagicParams::incAll
    );
    lua.new_usertype<DamageDesc>( "DamageDesc",
        "min"     , &DamageDesc::minDamage,
        "max"     , &DamageDesc::maxDamage
    );
    lua.new_usertype<HeroPrimaryParams>( "HeroPrimaryParams",
        "ad"      , &HeroPrimaryParams::ad,
        "magic"   , &HeroPrimaryParams::magic,
        "incAll"  , &HeroPrimaryParams::incAll
    );
    lua.new_usertype<UnitPrimaryParams>( "UnitPrimaryParams",
        "dmg"        , &UnitPrimaryParams::dmg,
        "ad"         , &UnitPrimaryParams::ad,
        "maxHealth"  , &UnitPrimaryParams::maxHealth,
        "speed"      , &UnitPrimaryParams::battleSpeed
    );
    lua.new_usertype<MagicReduce>( "MagicReduce",
        "all"        , &MagicReduce::allMagic,
        "air"        , &MagicReduce::air    ,
        "earth"      , &MagicReduce::earth  ,
        "fire"       , &MagicReduce::fire   ,
        "water"      , &MagicReduce::water
    );
    lua.new_usertype<MagicIncrease>( "MagicIncrease",
        "all"        , &MagicIncrease::allMagic,
        "air"        , &MagicIncrease::air    ,
        "earth"      , &MagicIncrease::earth  ,
        "fire"       , &MagicIncrease::fire   ,
        "water"      , &MagicIncrease::water
    );

    lua.new_usertype<ResourceAmount>( "ResourceAmount",
        "gold"     , &ResourceAmount::gold,
        "wood"     , &ResourceAmount::wood,
        "ore"      , &ResourceAmount::ore,
        "mercury"  , &ResourceAmount::mercury,
        "sulfur"   , &ResourceAmount::sulfur,
        "crystal"  , &ResourceAmount::crystal,
        "gems"     , &ResourceAmount::gems
   );

   lua.new_enum("CType",
                "Living",        UnitType::Living,
                "NonLiving",     UnitType::NonLiving,
                "SiegeMachine",  UnitType::SiegeMachine,
                "ArrowTower",    UnitType::ArrowTower,
                "Wall",          UnitType::Wall,
                "Unknown",       UnitType::Unknown
                 );
   lua.new_enum("NLType",
                "None",          UnitNonLivingType::None,
                "Undead",        UnitNonLivingType::Undead,
                "Golem",         UnitNonLivingType::Golem,
                "Gargoyle",      UnitNonLivingType::Gargoyle,
                "Elemental",     UnitNonLivingType::Elemental,
                "BattleMachine", UnitNonLivingType::BattleMachine
                 );
}


int64_t GeneralEstimation::getExperienceForLevel(int level)
{
    if (level <= 1)
        return 0;
    return levelTable.at(level);
}

int GeneralEstimation::getLevelByExperience(int64_t experience)
{
    if (experience <= 0)
        return 1;
    auto it = levelTableInv.lower_bound(experience + 1);
    it--;
    return it->second;
}

BonusRatio GeneralEstimation::calculatePhysicalBase(DamageDesc dmg, int count, DamageRollMode rollMode, IRandomGenerator& randomGenerator)
{
    auto makeSpreadRoll = [rollMode, &randomGenerator](int spread, int count) -> int {
        if (spread == 0 || rollMode == DamageRollMode::Min)
            return 0;

        if (rollMode == DamageRollMode::Max)
            return spread * count;
        if (rollMode == DamageRollMode::Avg)
            return spread * count / 2;

        //int result = 0;
        const int limitRngRequests = 10;
        const int count1 = std::min(limitRngRequests, count);
        assert(spread <= std::numeric_limits<uint8_t>::max()); // @todo: add this check to GameDatabase loader as well!

        int total = static_cast<int>(randomGenerator.genSumSmallN(count1, static_cast<uint8_t>(spread)));

        if (count > count1) {
            total = total * count / count1;
        }
        return total;
    };
    const int damageSpread = dmg.maxDamage - dmg.minDamage;
    const int damageBaseRoll = (dmg.minDamage * count + makeSpreadRoll(damageSpread, count));
    return BonusRatio(damageBaseRoll, 1);
}

std::pair<BonusRatio, BonusRatio> GeneralEstimation::calculateAttackPower(int attackerAttack, int targetDefense)
{
    const int maxEffectiveAttack = 60;
    const int maxEffectiveDefense = 28;
    const BonusRatio attackValue ( 1, 20); // 5% per attack power;
    const BonusRatio defenseValue(-1, 40); // -2.5% per neg. attack power;
    const int attackPower = std::clamp(attackerAttack - targetDefense, -maxEffectiveDefense, maxEffectiveAttack);
    if (attackPower > 0) {
        return {attackValue * attackPower, {0,1}};
    } else {
        return {{0,1}, defenseValue * attackPower};
    }
}

int GeneralEstimation::spellBaseDamage(int targetUnitLevel, const SpellCastParams & castParams, int targetIndex)
{
    sol::state lua;

    bindTypes(lua);

    lua["damage"] = 0;
    lua["spellPower"] = castParams.spellPower;
    lua["level"] = castParams.skillLevel;
    lua["isSpec"] = castParams.heroSpecLevel != -1;
    lua["unitLevel"] = targetUnitLevel;
    lua["heroLevel"] = castParams.heroSpecLevel;
    lua["index"] = targetIndex;

    for (const auto & calc : castParams.spell->calcScript)
        lua.script(calc);

    int damage = lua["damage"];
    return damage;
}


}
