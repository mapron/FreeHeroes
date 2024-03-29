/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureEstimation.hpp"
#include "GeneralEstimation.hpp"

#include "LibrarySecondarySkill.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryGameRules.hpp"
#include "LibraryTerrain.hpp"

#include "IRandomGenerator.hpp"
#include "IGameDatabase.hpp"

#include "MernelPlatform/Logger.hpp"

#include <sol/sol.hpp>

namespace FreeHeroes::Core {
using namespace Mernel;

namespace {
bool isWisdom(LibrarySecondarySkillConstPtr skill)
{
    if (!skill)
        return false;
    return skill->handler == LibrarySecondarySkill::HandlerType::Wisdom;
};
bool isSchool(LibrarySecondarySkillConstPtr skill)
{
    if (!skill)
        return false;
    return skill->handler == LibrarySecondarySkill::HandlerType::School;
};

template<typename T>
T selectKeyFromWeightMap(const std::map<T, int>& weights, IRandomGenerator& rng)
{
    if (weights.size() == 1)
        return weights.begin()->first;
    int totalWeight = 0;
    for (const auto& p : weights)
        totalWeight += p.second;
    int value       = rng.genSmall(static_cast<uint8_t>(totalWeight - 1));
    int indexWeight = 0;
    for (const auto& p : weights) {
        indexWeight += p.second;
        if (indexWeight > value)
            return p.first;
    }
    assert(!"How?");
    return T();
}

LibrarySecondarySkillConstPtr selectKeyFromWeightMap(const AdventureHero::EstimatedParams::LevelupParams::PriorSkillWeights& weights, IRandomGenerator& rng)
{
    if (!weights.high.empty())
        return selectKeyFromWeightMap(weights.high, rng);
    if (!weights.normal.empty())
        return selectKeyFromWeightMap(weights.normal, rng);
    return nullptr;
}

}

void AdventureEstimation::bindTypes(sol::state& lua)
{
    GeneralEstimation(m_rules).bindTypes(lua);
    // clang-format off
    lua.new_usertype<AdventureHero::EstimatedParams>( "AdventureHeroEstimatedParams",
        "meleeAttack"           , &AdventureHero::EstimatedParams::meleeAttack,
        "rangedAttack"          , &AdventureHero::EstimatedParams::rangedAttack,
        "defense"               , &AdventureHero::EstimatedParams::defense,
        "rngParams"             , &AdventureHero::EstimatedParams::rngParams,
        "primary"               , &AdventureHero::EstimatedParams::primary,

        "magicIncrease"         , &AdventureHero::EstimatedParams::magicIncrease,
        "magicResistChance"     , &AdventureHero::EstimatedParams::magicResistChance,

        "rngParamsOpp"          , &AdventureHero::EstimatedParams::rngParamsForOpponent,
        "spReduceOpp"           , &AdventureHero::EstimatedParams::spReduceOpp,
        "manaIncrease"          , &AdventureHero::EstimatedParams::manaIncrease,
        "mpIncrease"            , &AdventureHero::EstimatedParams::mpIncrease,
        "mpWaterIncrease"       , &AdventureHero::EstimatedParams::mpWaterIncrease,

        "rngMult"               , &AdventureHero::EstimatedParams::rngMult,

        "unitSpeedAbs"          , &AdventureHero::EstimatedParams::unitBattleSpeedAbs,
        "unitLifeAbs"           , &AdventureHero::EstimatedParams::unitLifeAbs,
        "unitLife"              , &AdventureHero::EstimatedParams::unitLife,

        "extraMP"               , &AdventureHero::EstimatedParams::extraMovePoints,
        "extraMPWater"          , &AdventureHero::EstimatedParams::extraMovePointsWater,

        "manaRegenAbs"          , &AdventureHero::EstimatedParams::manaRegenAbs,
        "scoutingRadius"        , &AdventureHero::EstimatedParams::scoutingRadius,

        "maxLearningSpell"      , &AdventureHero::EstimatedParams::maxLearningSpellLevel,
        "maxTeachingSpell"      , &AdventureHero::EstimatedParams::maxTeachingSpellLevel,

        "schoolLevels"          , &AdventureHero::EstimatedParams::schoolLevels,
        "extraRounds"           , &AdventureHero::EstimatedParams::extraRounds,
        "dayIncome"             , &AdventureHero::EstimatedParams::dayIncome,

        "surrenderDiscount"       , &AdventureHero::EstimatedParams::surrenderDiscount,
        "neutralJoinChance"       , &AdventureHero::EstimatedParams::neutralJoinChance,
        "greatLibraryVisitLevel"  , &AdventureHero::EstimatedParams::greatLibraryVisitLevel,
        "bonusExperience"         , &AdventureHero::EstimatedParams::bonusExperience,
        "eagleEyeChance"          , &AdventureHero::EstimatedParams::eagleEyeChance,
        "necromancy"              , &AdventureHero::EstimatedParams::necromancy,

        "regenerateStackHealth"   , &AdventureHero::EstimatedParams::regenerateStackHealth
    );
    // clang-format on
}

void AdventureEstimation::calculateHeroLevelUp(AdventureHero& hero)
{
    using SkillWeights      = LibraryFactionHeroClass::SkillWeights;
    auto&     levelupParams = hero.estimated.levelUp;
    auto      heroClass     = hero.library->heroClass();
    const int skillLimit    = 8; // @todo: settings

    // @todo: check exact boundary around level 10. Maybe <= 10 ?
    LibraryFactionHeroClass::PrimaryWeights primaryWeights = hero.level < 10 ? heroClass->lowLevelIncrease : heroClass->highLevelIncrease;
    levelupParams.primaryWeights                           = primaryWeights;

    auto hasWisdom = [](const SkillWeights& skillWeights) {
        for (auto w : skillWeights) {
            if (isWisdom(w.first))
                return true;
        }
        return false;
    };

    auto hasSchool = [](const SkillWeights& skillWeights) {
        for (auto w : skillWeights) {
            if (isSchool(w.first))
                return true;
        }
        return false;
    };

    SkillWeights allSkillWeights = heroClass->secondarySkillWeights;
    for (auto& p : allSkillWeights) {
        p.second += hero.library->isWarrior ? p.first->frequencyWarrior : p.first->frequencyMage;
    }
    SkillWeights                            skillWeightsForUpgrade, schoolWeightsForUpgrade;
    std::set<LibrarySecondarySkillConstPtr> heroExisting;
    for (auto skillRec : hero.secondarySkills) {
        heroExisting.insert(skillRec.skill);
        if (skillRec.level >= 2)
            continue;
        skillWeightsForUpgrade[skillRec.skill] = std::max(1, allSkillWeights.at(skillRec.skill));
        if (isSchool(skillRec.skill))
            schoolWeightsForUpgrade[skillRec.skill] = std::max(1, allSkillWeights.at(skillRec.skill));
    }
    levelupParams.unupgradedSkillCount = skillWeightsForUpgrade.size();
    SkillWeights skillWeightsForNew, wisdomOnlyWeights, schoolWeightsForNew;
    for (auto w : allSkillWeights) {
        if (isWisdom(w.first))
            wisdomOnlyWeights[w.first] = w.second;
    }
    const bool canLearnNewSkill = hero.secondarySkills.size() < skillLimit;
    if (canLearnNewSkill) {
        for (auto w : allSkillWeights) {
            if (heroExisting.contains(w.first))
                continue;
            skillWeightsForNew[w.first] = w.second;
            if (isSchool(w.first))
                schoolWeightsForNew[w.first] = w.second;
        }
    }

    auto processSpecial = [&skillWeightsForUpgrade, &skillWeightsForNew](AdventureHero::EstimatedParams::LevelupParams::Special& specialParams, auto hasCb, int suggestEveryLevel, int levelupsWithout) {
        specialParams.canBeSuggestedUpgrade = hasCb(skillWeightsForUpgrade);
        specialParams.canBeSuggestedNew     = hasCb(skillWeightsForNew);
        specialParams.canBeSuggested        = specialParams.canBeSuggestedNew || specialParams.canBeSuggestedUpgrade;
        specialParams.forceUpgrade          = levelupsWithout >= (suggestEveryLevel - 1) && specialParams.canBeSuggestedUpgrade;
        specialParams.forceNew              = levelupsWithout >= (suggestEveryLevel - 1) && specialParams.canBeSuggestedNew;
        specialParams.suggestEveryLevel     = suggestEveryLevel;
    };

    const int suggestSchoolEveryLevel = hero.library->isWarrior ? 4 : 3;
    const int suggestWisdomEveryLevel = hero.library->isWarrior ? 6 : 3;
    processSpecial(levelupParams.wisdom, hasWisdom, suggestWisdomEveryLevel, hero.levelupsWithoutWisdom);
    processSpecial(levelupParams.school, hasSchool, suggestSchoolEveryLevel, hero.levelupsWithoutSchool);

    levelupParams.weightsForNew.normal     = skillWeightsForNew;
    levelupParams.weightsForUpgrade.normal = skillWeightsForUpgrade;

    // check for Magic Schools and Wisdom

    // if we have not recieve upgrade or suggestions for Wisdom or school for several levels,

    if (levelupParams.wisdom.forceNew) {
        levelupParams.weightsForNew.high = wisdomOnlyWeights;
    } else if (levelupParams.school.forceNew) {
        levelupParams.weightsForNew.high = schoolWeightsForNew;
    }
    // and new is not suitable (we have all req wisdom/school), suggest as upgrade.
    if (levelupParams.wisdom.forceUpgrade) {
        levelupParams.weightsForUpgrade.high = wisdomOnlyWeights;
    } else if (levelupParams.school.forceUpgrade && !schoolWeightsForUpgrade.empty()) {
        levelupParams.weightsForUpgrade.high = schoolWeightsForUpgrade;
    }
}

AdventureEstimation::LevelUpResult AdventureEstimation::calculateHeroLevelUp(AdventureHero& hero, IRandomGenerator& rng)
{
    using PriorSkillWeights = AdventureHero::EstimatedParams::LevelupParams::PriorSkillWeights;

    LevelUpResult result;
    if (hero.experience < hero.estimated.experienceNextLevel) {
        return result;
    }

    result.valid                                    = true;
    result.primarySkillUpdated                      = HeroPrimaryParamType::Attack;
    const PriorSkillWeights& skillWeightsForNew     = hero.estimated.levelUp.weightsForNew;
    const PriorSkillWeights& skillWeightsForUpgrade = hero.estimated.levelUp.weightsForUpgrade;
    const bool               possibleNew            = !skillWeightsForNew.isEmpty();
    const bool               possibleUpgrade        = !skillWeightsForUpgrade.isEmpty();
    const bool               possibleAnyNewSkill    = possibleNew || possibleUpgrade;

    // 1. Try left skill = upgrade common prob, right = new common prob.
    if (possibleUpgrade && possibleNew) {
        auto skillUpgrade = selectKeyFromWeightMap(skillWeightsForUpgrade, rng);
        auto skillNew     = selectKeyFromWeightMap(skillWeightsForNew, rng);
        result.choices.push_back(skillUpgrade);
        result.choices.push_back(skillNew);
        result.choicesLevels.push_back(hero.getSkillLevel(skillUpgrade) + 1);
        result.choicesLevels.push_back(0);
    }
    // 2. if we cannot upgrade anything, but can get new skill (so all existing is upgraded), then get 1-2 new
    else if (possibleNew) {
        auto skillNew1 = selectKeyFromWeightMap(skillWeightsForNew, rng);
        result.choices.push_back(skillNew1);
        result.choicesLevels.push_back(0);
        auto skillWeightsForNewCopy = skillWeightsForNew;
        skillWeightsForNewCopy.erase(skillNew1);
        if (!skillWeightsForNewCopy.isEmpty()) {
            auto skillNew2 = selectKeyFromWeightMap(skillWeightsForNewCopy, rng);
            result.choices.push_back(skillNew2);
            result.choicesLevels.push_back(0);
        }
    }
    // 3. if we cannot get any new skill, but upgrades left - try to get 1-2 upgrades
    else if (possibleUpgrade) {
        auto skillUpgrade1 = selectKeyFromWeightMap(skillWeightsForUpgrade, rng);
        result.choices.push_back(skillUpgrade1);
        result.choicesLevels.push_back(hero.getSkillLevel(skillUpgrade1) + 1);
        auto skillWeightsForUpgradeCopy = skillWeightsForUpgrade;
        skillWeightsForUpgradeCopy.erase(skillUpgrade1);

        //Logger(Logger::Info) << "Upgrade: skillWeightsForUpgrade.size=" << skillWeightsForUpgrade.size() << ", skillWeightsForUpgradeCopy.size=" << skillWeightsForUpgradeCopy.size() ;
        if (!skillWeightsForUpgradeCopy.isEmpty()) {
            auto skillUpgrade2 = selectKeyFromWeightMap(skillWeightsForUpgradeCopy, rng);
            result.choices.push_back(skillUpgrade2);
            result.choicesLevels.push_back(hero.getSkillLevel(skillUpgrade2) + 1);
        }
    }

    result.primarySkillUpdated = selectKeyFromWeightMap(hero.estimated.levelUp.primaryWeights, rng);
    // clang-format off
    switch (result.primarySkillUpdated) {
        case HeroPrimaryParamType::Attack      : hero.currentBasePrimary.ad   .incAtt(1); break;
        case HeroPrimaryParamType::Defense     : hero.currentBasePrimary.ad   .incDef(1); break;
        case HeroPrimaryParamType::SpellPower  : hero.currentBasePrimary.magic.incSP(1) ; break;
        case HeroPrimaryParamType::Intelligence: hero.currentBasePrimary.magic.incInt(1); break;
        default:
        assert(!"Invalid database config."); break;
    }
    // clang-format on
    result.newLevel = hero.level + 1;
    hero.level      = result.newLevel;

    auto hasWisdomInResult = [&result]() {
        for (auto skill : result.choices) {
            if (isWisdom(skill))
                return true;
        }
        return false;
    };
    auto hasSchoolInResult = [&result]() {
        for (auto skill : result.choices) {
            if (isSchool(skill))
                return true;
        }
        return false;
    };
    const bool wisdomWasSuggested = hasWisdomInResult();
    const bool schoolWasSuggested = hasSchoolInResult();
    if (wisdomWasSuggested)
        hero.levelupsWithoutWisdom = 0;
    else if (hero.estimated.levelUp.wisdom.canBeSuggested)
        hero.levelupsWithoutWisdom++;
    if (schoolWasSuggested)
        hero.levelupsWithoutSchool = 0;
    else if (hero.estimated.levelUp.school.canBeSuggested)
        hero.levelupsWithoutSchool++;

    Logger log(Logger::Info);
    log << "Hero " << hero.library->id << " levelup to " << result.newLevel << ". Choices:";
    for (auto skill : result.choices)
        log << skill->id << ", ";
    auto printSpecial = [&log](const AdventureHero::EstimatedParams::LevelupParams::Special& special) {
        log << "SuggestUpg: " << special.canBeSuggestedUpgrade << ", SuggestNew:" << special.canBeSuggestedNew
            << ", ForceUpg: " << special.forceUpgrade << ", ForceNew:" << special.forceNew;
    };

    log << "possibleNew: " << possibleNew << ", possibleUpgrade:" << possibleUpgrade << ", total non-upgraded skills:" << hero.estimated.levelUp.unupgradedSkillCount;
    if (possibleAnyNewSkill) {
        log << "\n"
            << "wisdom:";
        printSpecial(hero.estimated.levelUp.wisdom);
        log << ", school:";
        printSpecial(hero.estimated.levelUp.school);
    }

    return result;
}

void AdventureEstimation::applyLevelUpChoice(AdventureHero& hero, LibrarySecondarySkillConstPtr skill)
{
    if (!skill)
        return;
    [[maybe_unused]] bool result = hero.setSkillLevel(skill, hero.getSkillLevel(skill) + 1, 8);
    assert(result);
}

void AdventureEstimation::calculateHeroStats(AdventureHero& hero)
{
    if (!hero.isValid())
        return;

    // reset
    hero.estimated         = {};
    hero.estimated.primary = hero.currentBasePrimary;
    auto spellbook         = hero.spellbook;

    // Set level and Exp.
    if (hero.editorParams.expIsDirty) {
        hero.experience              = GeneralEstimation(m_rules).getExperienceForLevel(hero.level);
        hero.editorParams.expIsDirty = false;
    }
    if (hero.editorParams.levelIsDirty) {
        hero.level                     = GeneralEstimation(m_rules).getLevelByExperience(hero.experience);
        hero.editorParams.levelIsDirty = false;
    }
    hero.estimated.experienceStartLevel = GeneralEstimation(m_rules).getExperienceForLevel(hero.level);
    hero.estimated.experienceNextLevel  = GeneralEstimation(m_rules).getExperienceForLevel(hero.level + 1);

    for (auto* resId : m_gameDatabase->resources()->records())
        hero.estimated.dayIncome.data[resId] = 0;

    sol::state lua;

    // bindings
    bindTypes(lua);

    // Skills
    {
        lua["h"]         = hero.estimated;
        lua["heroLevel"] = hero.level;
        for (auto skillRec : hero.secondarySkills) {
            int        level  = skillRec.level;
            auto       skill  = skillRec.skill;
            const bool isStat = skill->handler == LibrarySecondarySkill::HandlerType::Stat
                                || skill->handler == LibrarySecondarySkill::HandlerType::Wisdom
                                || skill->handler == LibrarySecondarySkill::HandlerType::School;
            if (isStat) {
                lua["skillLevel"] = level;
                lua["isSpec"]     = hero.library->spec->type == LibraryHeroSpec::Type::Skill && hero.library->spec->skill == skill;
                for (const auto& calc : skill->calc)
                    lua.script(calc);
            }
        }
        hero.estimated                      = lua["h"];
        hero.estimated.moraleDetails.skills = hero.estimated.rngParams.morale;
        hero.estimated.luckDetails.skills   = hero.estimated.rngParams.luck;
    }
    PrimaryRngParams lastRng = hero.estimated.rngParams;

    // artifacts
    {
        const auto&              reqFree      = ArtifactSlotRequirement::defaultFreeSlots();
        ArtifactSlotRequirement& mainUsed     = hero.estimated.slotsInfo.mainUsed; //used slots without extra parts;
        ArtifactSlotRequirement& allUsed      = hero.estimated.slotsInfo.allUsed;
        ArtifactSlotRequirement& extraUsed    = hero.estimated.slotsInfo.extraUsed;
        ArtifactSlotRequirement& freeUsed     = hero.estimated.slotsInfo.freeUsed;
        ArtifactWearingSet&      allWearing   = hero.estimated.slotsInfo.allWearing;
        ArtifactWearingSet&      mainWearing  = hero.estimated.slotsInfo.mainWearing;
        ArtifactWearingSet&      extraWearing = hero.estimated.slotsInfo.extraWearing;
        ArtifactWearingSet&      freeWearing  = hero.estimated.slotsInfo.freeWearing;
        freeUsed                              = reqFree;

        std::set<LibraryArtifactConstPtr> usedForCalculation;
        //std::set<ArtifactSlotType> occupiedSlots;
        // std::set<ArtifactSlotType> freeSlots(allCommon.begin(), allCommon.end());
        for (const auto& artItem : hero.artifactsOn) {
            if (!artItem.second)
                continue;
            if (artItem.first == ArtifactSlotType::BmAmmo
                || artItem.first == ArtifactSlotType::BmShoot
                || artItem.first == ArtifactSlotType::BmTent)
                continue;
            usedForCalculation.insert(artItem.second);
            //reqFree.subtract(artItem.second->slotReq);
            allUsed.add(artItem.second->slotReq);
            mainUsed.add(artItem.second->slot);
            mainWearing.wearingSlots.insert(artItem.first);
            for (auto* subArtifact : artItem.second->parts)
                usedForCalculation.insert(subArtifact);
        }
        freeUsed.subtract(allUsed);
        assert(freeUsed.allSlotsNonNegative());
        extraUsed = allUsed - mainUsed;
        assert(extraUsed.allSlotsNonNegative());

        const auto& wearingFreeDefault = ArtifactWearingSet::defaultFreeSlots();

        if (!extraUsed.getWearingTypes(extraWearing, mainWearing))
            assert(false);

        allWearing = extraWearing + mainWearing;
        assert(allWearing.wearingSlots.size() == extraWearing.wearingSlots.size() + mainWearing.wearingSlots.size());

        freeWearing = wearingFreeDefault - allWearing;

        lua["h"] = hero.estimated;

        for (auto* art : usedForCalculation) {
            for (const auto& calc : art->calc)
                lua.script(calc);
        }
        hero.estimated = lua["h"];
        SpellCastParamsList extraCasts;
        for (auto* art : usedForCalculation) {
            for (auto* spell : art->provideSpellsCache)
                spellbook.insert(spell);
            if (art->scrollSpell)
                spellbook.insert(art->scrollSpell);
            hero.estimated.immunities.makeUnion(art->protectSpells);
            hero.estimated.forbidSpells.makeUnion(art->forbidSpells);
            hero.estimated.primary += art->statBonus;
            for (auto penalty : art->disabledPenalties)
                hero.estimated.disabledPenalties.insert(penalty);
            for (auto cast : art->spellCasts) {
                cast.art = art;
                extraCasts.push_back(cast);
            }
            if (art->special != LibraryArtifact::SpecialEffect::None)
                hero.estimated.specialArtifactEffects.insert(art->special);
        }

        hero.estimated.moraleDetails.artifacts = hero.estimated.rngParams.morale - lastRng.morale;
        hero.estimated.luckDetails.artifacts   = hero.estimated.rngParams.luck - lastRng.luck;
        hero.estimated.castsBeforeStart        = extraCasts;
        lastRng                                = hero.estimated.rngParams;
    }

    // Clamp parameters.

    // clang-format off
    hero.estimated.primary.ad.attack          = std::clamp(hero.estimated.primary.ad.attack         , 0, m_rules->limits.maxHeroAd.attack );
    hero.estimated.primary.ad.defense         = std::clamp(hero.estimated.primary.ad.defense        , 0, m_rules->limits.maxHeroAd.defense);
    hero.estimated.primary.magic.spellPower   = std::clamp(hero.estimated.primary.magic.spellPower  , 1, m_rules->limits.maxHeroMagic.spellPower );
    hero.estimated.primary.magic.intelligence = std::clamp(hero.estimated.primary.magic.intelligence, 1, m_rules->limits.maxHeroMagic.intelligence);
    // clang-format on

    hero.estimated.availableSpells.reserve(spellbook.size());

    for (auto spell : spellbook) {
        int       manaCost    = spell->manaCost;
        const int schoolLevel = hero.estimated.schoolLevels.getLevelForSpell(spell->school);
        if (schoolLevel > 0) {
            manaCost -= spell->level; // @todo: teleport spell has more reduce.
        }
        const int hintDamage = 0; // @todo:
        hero.estimated.availableSpells.push_back({ spell, manaCost, schoolLevel, hintDamage });
    }
    std::sort(hero.estimated.availableSpells.begin(), hero.estimated.availableSpells.end(), [](auto& l, auto& r) {
        return l.spell->presentationParams.order < r.spell->presentationParams.order;
    });
    calculateHeroLevelUp(hero);

    // hero spec check
    if (hero.library->spec->type == LibraryHeroSpec::Type::Income) {
        hero.estimated.dayIncome += hero.library->spec->dayIncome;
    }
    hero.estimated.dayIncome.removeEmpty();
}

void AdventureEstimation::calculateHeroStatsAfterSquad(AdventureHero& hero, const AdventureSquad& squad)
{
    if (!hero.isValid())
        return;

    {
        auto& maxMana = hero.estimated.maxMana;
        maxMana       = hero.estimated.primary.magic.intelligence * 10;
        maxMana       = BonusRatio::calcAddIncrease(maxMana, hero.estimated.manaIncrease);
    }

    hero.estimatedFromSquad.fastestBattleSpeed = squad.estimated.fastestBattleSpeed;
    {
        static const std::vector<int> armySpeedToMovePoints{
            1300, 1360, 1430, 1500, 1560, 1630, 1700, 1760, 1830, 1900, 1960
        };
        auto& as = hero.estimatedFromSquad.armySpeed;
        as       = squad.estimated.armySpeed;

        hero.estimated.armyMovePoints = as >= (int) armySpeedToMovePoints.size() ? 2000 : armySpeedToMovePoints[as];

        auto& mp = hero.estimated.nextDayMovePoints;
        mp       = hero.estimated.armyMovePoints;
        mp       = BonusRatio::calcAddIncrease(mp, hero.estimated.mpIncrease);

        mp += hero.estimated.extraMovePoints;

        auto& mpWater = hero.estimated.nextDayMovePointsWater;
        mpWater       = 1500;
        mpWater       = BonusRatio::calcAddIncrease(mpWater, hero.estimated.mpWaterIncrease);
        mpWater += hero.estimated.extraMovePointsWater;
    }

    if (hero.newBornHero) {
        hero.mana              = hero.estimated.maxMana;
        hero.thisDayMovePoints = hero.estimated.nextDayMovePoints;
        hero.movePointsRemain  = hero.thisDayMovePoints;
    }
    hero.estimatedFromSquad.rngParams            = squad.estimated.squadBonus.rngParams;
    hero.estimatedFromSquad.rngParamsForOpponent = squad.estimated.oppBonus.rngParams;
    hero.estimatedFromSquad.moraleDetails        = squad.estimated.moraleDetails;
    hero.estimatedFromSquad.luckDetails          = squad.estimated.luckDetails;
}

void AdventureEstimation::calculateSquad(AdventureSquad& squad, bool reduceExtraFactionsPenalty, LibraryTerrainConstPtr terrain)
{
    std::set<LibraryFactionConstPtr> factions;
    std::set<LibraryUnitConstPtr>    units;

    auto isUniqueUnit = [&units](LibraryUnitConstPtr unit) {
        if (units.contains(unit->baseUpgrade))
            return false;

        units.insert(unit->baseUpgrade);
        return true;
    };
    squad.estimated = {};
    if (terrain) {
        squad.estimated.squadBonus.rngMult *= terrain->bonusAll.rngMult;
    }

    for (auto& stack : squad.stacks) {
        if (!stack.isValid())
            continue;
        factions.insert(stack.library->faction);

        stack.estimated.primary = stack.library->primary; // armySpeed later
        const bool isUndead     = stack.library->abilities.nonLivingType == UnitNonLivingType::Undead;
        if (isUndead)
            squad.estimated.moraleDetails.undead = -1;

        auto& squadBonus = stack.library->abilities.squadBonus;
        auto& oppBonus   = stack.library->abilities.opponentBonus;

        auto& squadEst     = squad.estimated.squadBonus.rngParams;
        auto& oppEst       = squad.estimated.oppBonus.rngParams;
        auto& squadChances = squad.estimated.squadBonus.rngMult;
        auto& oppChances   = squad.estimated.oppBonus.rngMult;

        // getting the maximum positive effect of all stacks for our army, and the minimum negative.
        // for the opponent bonus - doing the opposite
        squadEst.luck   = std::max(squadEst.luck, squadBonus.luck);
        squadEst.morale = std::max(squadEst.morale, squadBonus.morale);
        oppEst.luck     = std::min(oppEst.luck, oppBonus.luck);
        oppEst.morale   = std::min(oppEst.morale, oppBonus.morale);

        if (isUniqueUnit(stack.library)) {
            squadChances *= squadBonus.rngMult;
            oppChances *= oppBonus.rngMult;
        }

        squad.estimated.squadBonus.manaCost = std::min(squad.estimated.squadBonus.manaCost, squadBonus.manaCost);
        squad.estimated.oppBonus.manaCost   = std::max(squad.estimated.oppBonus.manaCost, oppBonus.manaCost);

        squad.estimated.weekIncomeMax.maxWith(stack.library->abilities.weekIncome);
    }

    squad.estimated.moraleDetails.unitBonus = squad.estimated.squadBonus.rngParams.morale;
    squad.estimated.luckDetails.unitBonus   = squad.estimated.squadBonus.rngParams.luck;
    int totalExtraFactionsCount             = static_cast<int>(factions.size()) - 1;
    if (reduceExtraFactionsPenalty) {
        int totalGoodAndNeutral = 0;
        int totalOther          = 0;
        for (auto faction : factions) {
            if (faction->alignment == LibraryFaction::Alignment::Good || faction->alignment == LibraryFaction::Alignment::Neutral)
                totalGoodAndNeutral++;
            else
                totalOther++;
        }
        if (totalGoodAndNeutral > 0)
            totalExtraFactionsCount = totalOther;
    }
    squad.estimated.moraleDetails.extraUnwantedFactions = std::max(0, totalExtraFactionsCount);
    squad.estimated.moraleDetails.factionsPenalty       = 1 - totalExtraFactionsCount;
    squad.estimated.squadBonus.rngParams.morale += squad.estimated.moraleDetails.factionsPenalty;
    squad.estimated.squadBonus.rngParams.morale += squad.estimated.moraleDetails.undead;

    squad.estimated.moraleDetails.total = squad.estimated.squadBonus.rngParams.morale;
    squad.estimated.luckDetails.total   = squad.estimated.squadBonus.rngParams.luck;

    squad.estimated.moraleDetails.rollChance = GeneralEstimation(m_rules).estimateMoraleRoll(squad.estimated.squadBonus.rngParams.morale, squad.estimated.squadBonus.rngMult);
    squad.estimated.luckDetails.rollChance   = GeneralEstimation(m_rules).estimateLuckRoll(squad.estimated.squadBonus.rngParams.luck, squad.estimated.squadBonus.rngMult);
}

void AdventureEstimation::calculateSquadHeroRng(AdventureSquad& squad, const AdventureHero& hero)
{
    if (!hero.isValid())
        return;

    squad.estimated.moraleDetails += hero.estimated.moraleDetails;
    squad.estimated.luckDetails += hero.estimated.luckDetails;

    squad.estimated.squadBonus.rngParams += hero.estimated.rngParams;
    squad.estimated.oppBonus.rngParams += hero.estimated.rngParamsForOpponent;
    // we assume now hero has no chance affecting artifacts that affects only one side.
    // like no 'double morale chance for me' kind ones.
    squad.estimated.squadBonus.rngMult *= hero.estimated.rngMult;

    squad.estimated.moraleDetails.total = squad.estimated.squadBonus.rngParams.morale;
    squad.estimated.luckDetails.total   = squad.estimated.squadBonus.rngParams.luck;

    squad.estimated.moraleDetails.rollChance = GeneralEstimation(m_rules).estimateMoraleRoll(squad.estimated.squadBonus.rngParams.morale, squad.estimated.squadBonus.rngMult);
    squad.estimated.luckDetails.rollChance   = GeneralEstimation(m_rules).estimateLuckRoll(squad.estimated.squadBonus.rngParams.luck, squad.estimated.squadBonus.rngMult);
}

void AdventureEstimation::calculateSquadSpeed(AdventureSquad& squad)
{
    int fastestBattleSpeed = 0;
    int armySpeed          = -1;
    for (auto& stack : squad.stacks) {
        if (!stack.isValid())
            continue;
        fastestBattleSpeed = std::max(fastestBattleSpeed, stack.estimated.primary.battleSpeed);
        if (armySpeed == -1)
            armySpeed = stack.estimated.primary.armySpeed;
        else
            armySpeed = std::min(stack.estimated.primary.armySpeed, armySpeed);
    }
    if (armySpeed == -1)
        armySpeed = 0;
    squad.estimated.fastestBattleSpeed = fastestBattleSpeed;
    squad.estimated.armySpeed          = armySpeed;
}

void calculateUnitStats(LibraryGameRulesConstPtr rules, AdventureStack& unit, const AdventureSquad& squad, LibraryTerrainConstPtr currentTerrain, const AdventureHero& hero)
{
    if (!unit.isValid())
        return;
    auto& cur         = unit.estimated;
    cur.luckDetails   = squad.estimated.luckDetails;
    cur.moraleDetails = squad.estimated.moraleDetails;
    cur.primary       = unit.library->primary;
    cur.rngParams     = squad.estimated.squadBonus.rngParams; // already included possible hero rng.
    const auto& abils = unit.library->abilities;

    cur.hasMorale = abils.type == UnitType::Living || abils.nonLivingType == UnitNonLivingType::Gargoyle;
    if (!cur.hasMorale) {
        cur.rngParams.morale         = 0;
        cur.moraleDetails            = {};
        cur.moraleDetails.unaffected = true;
    }
    cur.moraleDetails.total = cur.rngParams.morale;
    cur.luckDetails.total   = cur.rngParams.luck;

    cur.rngMult = squad.estimated.squadBonus.rngMult;
    if (currentTerrain) {
        // @todo: evil/good specific bonuses.
    }

    cur.moraleDetails.minimalMoraleLevel = unit.library->abilities.minimalMoraleLevel;
    cur.luckDetails.minimalLuckLevel     = unit.library->abilities.minimalLuckLevel;

    cur.rngParams.morale = std::max(cur.rngParams.morale, cur.moraleDetails.minimalMoraleLevel);
    cur.rngParams.luck   = std::max(cur.rngParams.luck, cur.luckDetails.minimalLuckLevel);

    cur.moraleDetails.rollChance = GeneralEstimation(rules).estimateMoraleRoll(cur.rngParams.morale, cur.rngMult);
    cur.luckDetails.rollChance   = GeneralEstimation(rules).estimateLuckRoll(cur.rngParams.luck, cur.rngMult);

    if (currentTerrain && unit.library->faction->nativeTerrain == currentTerrain) {
        cur.primary.battleSpeed += 1;
        cur.primary.ad.incAll(1);
    }
    cur.magicReduce           = unit.library->abilities.magicReduce;
    cur.magicOppSuccessChance = unit.library->abilities.magicOppSuccessChance;
    cur.immunities            = {};
    cur.immunities.general    = unit.library->abilities.immunes;
    if (!unit.library->abilities.immuneBreakable)
        cur.immunities.withoutBreakable = cur.immunities.general;

    cur.regenerate = unit.library->abilities.regenerate;
    for (auto penalty : unit.library->abilities.disabledPenalties)
        cur.disabledPenalties.insert(penalty);

    cur.castsOnHit = unit.library->abilities.castsOnHit;
    cur.fixedCast  = unit.library->abilities.fixedCast;

    if (!hero.isValid())
        return;

    auto& heroEstimate = hero.estimated;
    cur.primary.ad += heroEstimate.primary.ad;
    cur.primary.battleSpeed += heroEstimate.unitBattleSpeedAbs;

    if (unit.library->battleMachineArtifact) {
        cur.primary.dmg.minDamage *= (heroEstimate.primary.ad.attack + 1);
        cur.primary.dmg.maxDamage *= (heroEstimate.primary.ad.attack + 1);
    }

    // clang-format off
    cur.magicReduce.allMagic  *= heroEstimate.magicReduce.allMagic;
    cur.magicReduce.air       *= heroEstimate.magicReduce.air     ;
    cur.magicReduce.earth     *= heroEstimate.magicReduce.earth   ;
    cur.magicReduce.fire      *= heroEstimate.magicReduce.fire    ;
    cur.magicReduce.water     *= heroEstimate.magicReduce.water   ;
    cur.magicOppSuccessChance *= BonusRatio(1,1) - std::min(heroEstimate.magicResistChance, BonusRatio(99,100));
    // clang-format on

    auto& maxHealth = cur.primary.maxHealth;
    if (unit.library->abilities.type == UnitType::Living) {
        cur.regenerate = cur.regenerate || heroEstimate.regenerateStackHealth;
        maxHealth      = BonusRatio::calcAddIncrease(maxHealth, heroEstimate.unitLife);
    }

    maxHealth += heroEstimate.unitLifeAbs;

    cur.primary.ad.attack  = std::clamp(cur.primary.ad.attack, 0, rules->limits.maxUnitAd.attack);
    cur.primary.ad.defense = std::clamp(cur.primary.ad.defense, 0, rules->limits.maxUnitAd.defense);

    cur.immunities.general.makeUnion(heroEstimate.immunities);
    cur.immunities.withoutBreakable.makeUnion(heroEstimate.immunities);
    cur.immunities.brokenGeneral  = heroEstimate.specialArtifactEffects.contains(LibraryArtifact::SpecialEffect::BreakImmunities);
    cur.immunities.brokenPositive = cur.immunities.brokenGeneral;

    for (auto penalty : heroEstimate.disabledPenalties)
        cur.disabledPenalties.insert(penalty);

    if (hero.library->spec->type == LibraryHeroSpec::Type::SpecialSpeed) {
        cur.primary.battleSpeed += 2; // no army speed bonus intentional.
        return;
    }

    if (hero.library->spec->type != LibraryHeroSpec::Type::Unit)
        return;

    if (hero.library->spec->unit != unit.library->baseUpgrade)
        return;
    cur.primary.armySpeed += 1;
    cur.primary.battleSpeed += 1;

    int level = unit.library->level / 10;
    assert(level > 0);
    int levelRatio = hero.level / level;

    // +5% attack and defense for hero level.
    cur.primary.ad.attack += (unit.library->primary.ad.attack * levelRatio + 19) / 20;
    cur.primary.ad.defense += (unit.library->primary.ad.defense * levelRatio + 19) / 20;

    cur.primary.ad.attack  = std::clamp(cur.primary.ad.attack, 0, rules->limits.maxUnitAd.attack);
    cur.primary.ad.defense = std::clamp(cur.primary.ad.defense, 0, rules->limits.maxUnitAd.defense);
}

AdventureEstimation::AdventureEstimation(const IGameDatabase* gameDatabase)
    : m_gameDatabase(gameDatabase)
    , m_rules(gameDatabase->gameRules())
{
}

void AdventureEstimation::calculateArmy(AdventureArmy& army, LibraryTerrainConstPtr terrain)
{
    int validIndex = -1;
    for (size_t i = 0; i < army.squad.stacks.size(); ++i) {
        if (army.squad.stacks[i].isValid()) {
            validIndex++;
            army.squad.stacks[i].armyParams.indexInArmyValid = validIndex;
        }
        army.squad.stacks[i].armyParams.indexInArmy = static_cast<int>(i);
    }
    if (army.hasHero())
        calculateHeroStats(army.hero);

    const bool factionPenalty = army.hasHero() ? army.hero.estimated.specialArtifactEffects.contains(LibraryArtifact::SpecialEffect::FactionsAlliance) : false;
    calculateSquad(army.squad, factionPenalty, terrain);

    if (army.hasHero())
        calculateSquadHeroRng(army.squad, army.hero);

    for (auto& stack : army.squad.stacks)
        calculateUnitStats(m_rules, stack, army.squad, terrain, army.hasHero() ? army.hero : AdventureHero());

    calculateSquadSpeed(army.squad);

    if (army.hasHero()) {
        calculateHeroStatsAfterSquad(army.hero, army.squad);
        army.hero.newBornHero = false;
    }
    army.estimated               = {};
    army.estimated.weekIncomeMax = army.squad.estimated.weekIncomeMax;
    if (army.hasHero())
        army.estimated.dayIncome = army.hero.estimated.dayIncome;
}

void AdventureEstimation::calculateDayStart(AdventureHero& hero)
{
    const int needRegenMana = std::max(hero.estimated.maxMana - hero.mana, 0);

    hero.mana = hero.mana + std::min(needRegenMana, hero.estimated.manaRegenAbs);

    hero.movePointsRemain  = hero.estimated.nextDayMovePoints;
    hero.thisDayMovePoints = hero.movePointsRemain;
}

void AdventureEstimation::calculateArmySummon(const AdventureArmy& army, LibraryTerrainConstPtr terrain, AdventureStackMutablePtr stack)
{
    calculateUnitStats(m_rules, *stack, army.squad, terrain, army.hasHero() ? army.hero : AdventureHero());
}

}
