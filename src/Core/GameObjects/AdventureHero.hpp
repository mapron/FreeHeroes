/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryHero.hpp"
#include "LibraryArtifact.hpp"
#include "LibraryFaction.hpp"
#include "LibrarySpell.hpp"

#include "BonusRatio.hpp"

#include <vector>
#include <set>

#include <cassert>

namespace FreeHeroes::Core {

struct AdventureHero {
    AdventureHero() = default;
    explicit AdventureHero(LibraryHeroConstPtr library) {
        reset(library);
    }
    void reset(LibraryHeroConstPtr library) {
        this->library = library;
        secondarySkills = getInitialSkills();
        currentBasePrimary = getInitialPrimary();
        name = getInitialName();
        setSpellAvailable(getInitialSpell(), true);
        level = 1;
    }
    HeroSkillsList getInitialSkills() const  {
        return library ? library->secondarySkills : HeroSkillsList{};
    }
    HeroPrimaryParams getInitialPrimary() const  {
        return library ? library->heroClass()->startParams : HeroPrimaryParams();
    }
    std::string getInitialName() const {
        return library ? library->untranslatedName : std::string();
    }
    LibrarySpellConstPtr getInitialSpell() const {
        return library ? library->startSpell : nullptr;
    }
    int getSkillLevel(LibrarySecondarySkillConstPtr skill) const {
        auto it = std::find_if(secondarySkills.cbegin(), secondarySkills.cend(), [skill](auto sk){return sk.skill == skill;});
        if (it == secondarySkills.cend())
            return -1;
        return it->level;
    };
    bool setSkillLevel(LibrarySecondarySkillConstPtr skill, int level, int limit) {
        if (level < 0 || level > 2)
            return false;
        auto it = std::find_if(secondarySkills.begin(), secondarySkills.end(), [skill](auto sk){return sk.skill == skill;});
        if (it == secondarySkills.cend()) {
            if (static_cast<int>(secondarySkills.size()) >= limit)
                return false;

            secondarySkills.emplace_back(skill, level);
            return true;
        }
        it->level = level;
        return true;
    };
    bool removeSkill(LibrarySecondarySkillConstPtr skill) {
        auto it = std::find_if(secondarySkills.begin(), secondarySkills.end(), [skill](auto sk){return sk.skill == skill;});
        if (it != secondarySkills.end())
            secondarySkills.erase(it);
        else
            return false;
        return true;
    }

    void setSpellAvailable(LibrarySpellConstPtr spell, bool state) {
        if (state && spell) {
            spellbook.insert(spell);
            return;
        }
        auto it = spellbook.find(spell);
        if (it != spellbook.end())
           spellbook.erase(it);
    }
    LibraryUnit::HeroStackSize getExpectedStackSize(size_t index) const {
        if (library && index < library->startStacks.size() ) {
            auto c = library->startStacks[index];
            return c.stackSize.isValid() ? c.stackSize : c.unit->countWithHeroBase;
        }
        return {};
    }

    LibraryArtifactConstPtr getArtifact(ArtifactSlotType slot) const {
        return artifactsOn.contains(slot) ? artifactsOn.at(slot) : nullptr;
    }
    int getBagCount(LibraryArtifactConstPtr art) const {
        return artifactsBag.contains(art) ? artifactsBag.at(art) : 0;
    }
    bool isArtifactOn(LibraryArtifactConstPtr art) const {
        for (auto & p : artifactsOn) {
            if (p.second == art)
                return true;
        }
        return false;
    }
    size_t getWearingCountFromSet(LibraryArtifactConstPtr artSet) const {
        size_t count = 0;
        for (auto part : artSet->parts) {
            if (!isArtifactOn(part))
                continue;
            count++;
        }
        return count;
    }
    std::vector<LibraryArtifactConstPtr> getMissingPartsFromSet(LibraryArtifactConstPtr artSet) const {
        std::vector<LibraryArtifactConstPtr> result;
        for (auto part : artSet->parts) {
            if (!isArtifactOn(part))
                result.push_back(part);
        }
        return result;
    }

    using ArtifactsBagMap = std::map<LibraryArtifactConstPtr, int>;
    using ArtifactsOnMap  = std::map<ArtifactSlotType, LibraryArtifactConstPtr>;

    // main params:
    LibraryHeroConstPtr library = nullptr;

    HeroPrimaryParams                                   currentBasePrimary; // base parameters without artifacts.
    ArtifactsOnMap                                      artifactsOn;
    ArtifactsBagMap                                     artifactsBag;
    HeroSkillsList                                      secondarySkills;
    std::set<LibrarySpellConstPtr>                      spellbook;
    bool                                                hasSpellBook = false;

    int          level = 0;
    int64_t      experience = 0;
    int          mana = 0;
    int          movePointsRemain = 0;
    int          thisDayMovePoints = 0;
    bool         newBornHero = true; // for full mana and MP.
    int          levelupsWithoutWisdom = 1;
    int          levelupsWithoutSchool = 1;

    auto asTuple() const noexcept { return std::tie(library, currentBasePrimary, artifactsOn, artifactsBag, secondarySkills, spellbook,
                                                    hasSpellBook, level, experience, mana, movePointsRemain);}

    bool isEqualTo(const AdventureHero & another) const noexcept {
        if (isValid() != another.isValid())
            return false;
        if (!isValid() && !another.isValid())
            return true;
        return asTuple() == another.asTuple();
    }
    bool isValid() const noexcept { return !!library; }
    // editor params
    struct EditorParams {
        bool expIsDirty = false;
        bool levelIsDirty = false;
    };
    EditorParams editorParams;

    // display params
    std::string name;

    // estimation cache:
    struct SpellDetails {
        LibrarySpellConstPtr spell = nullptr;
        int manaCost = 0;
        int level = 0;
        int hintDamage = 0;
    };
    using SpellList = std::vector<SpellDetails>;

    struct EstimatedParams {
        int nextDayMovePoints = 0;
        int nextDayMovePointsWater = 0;
        int extraMovePoints = 0;     // artifacts + stables/map objects etc
        int extraMovePointsWater = 0;// artifacts + beacons/etc
        int armyMovePoints = 0;

        int maxMana = 0;                            // skills
        int manaRegenAbs = 1;                       // skills,artifacts
        int scoutingRadius = 5;                     // skills,artifacts
        int maxLearningSpellLevel = 2;
        int maxTeachingSpellLevel = 0;

        int64_t experienceStartLevel = 0;
        int64_t experienceNextLevel = 0;

        HeroPrimaryParams   primary;                // skills + artifacts
        PrimaryRngParams    rngParams;              // skills + artifacts
        PrimaryRngParams    rngParamsForOpponent;   // artifacts
        PrimaryRngParams    rngMax;                 // artifacts: 0 morale, 0  luck

        BonusRatio meleeAttack  = {0, 1};          // skills
        BonusRatio rangedAttack = {0, 1};          // skills
        BonusRatio defense      = {0, 1};          // skills

        MagicIncrease magicIncrease;               // skills
        MagicReduce   magicReduce;                  // this currently never used by database. created for symmetry..
        BonusRatio    magicResistChance = {0, 1};  //  skills + artifacts
        SpellFilter   immunes;                      // protector artifacts

        int unitBattleSpeedAbs = 0;             //  artifacts, spec on speed
        int unitLifeAbs = 0;                    //  artifacts
        BonusRatio unitLife = {0, 1};           //  artifacts

        BonusRatio spReduceOpp     = {0, 1};      // skills + artifacts
        BonusRatio manaIncrease    = {0, 1};      // skills
        BonusRatio mpIncrease      = {0, 1};      // skills
        BonusRatio mpWaterIncrease = {0, 1};      // skills

        bool factionsAllianceSpecial = false;    //  AA.

        MoraleDetails moraleDetails;
        LuckDetails   luckDetails;

        SpellList availableSpells; // sorted by order.

        MagicSchoolLevels schoolLevels; // skills
        int extraRounds = 0;

        SpellCastParamsList castsBeforeStart;

        ResourceAmount dayIncome;

        struct LevelupParams {
            struct Special {
                bool canBeSuggested = false;
                bool canBeSuggestedNew = false;
                bool canBeSuggestedUpgrade = false;
                bool forceNew = false;
                bool forceUpgrade = false;
                int suggestEveryLevel = 0;
            };
            struct PriorSkillWeights {
                LibraryFactionHeroClass::SkillWeights high;
                LibraryFactionHeroClass::SkillWeights normal;
                size_t size() const { return high.size() + normal.size();}
                bool isEmpty() const { return high.empty() && normal.empty(); }
                void erase(LibrarySecondarySkillConstPtr key) {
                    if (!high.empty()) high.erase(key);
                    if (!normal.empty()) normal.erase(key);
                }
            };

            Special wisdom;
            Special school;
            LibraryFactionHeroClass::PrimaryWeights primaryWeights;
            PriorSkillWeights weightsForNew;
            PriorSkillWeights weightsForUpgrade;
            int unupgradedSkillCount = 0;
        };
        LevelupParams levelUp;
        struct SlotsInfo {
            ArtifactSlotRequirement mainUsed;
            ArtifactSlotRequirement allUsed;
            ArtifactSlotRequirement extraUsed;
            ArtifactSlotRequirement freeUsed;

            ArtifactWearingSet allWearing;
            ArtifactWearingSet mainWearing;
            ArtifactWearingSet extraWearing;
            ArtifactWearingSet freeWearing;
        };
        SlotsInfo slotsInfo;
    };
    // not really used; for UI usage only.
    struct EstimatedParamsSquad {
        PrimaryRngParams    rngParams;
        PrimaryRngParams    rngParamsForOpponent;
        MoraleDetails       moraleDetails;
        LuckDetails         luckDetails;
        int                 armySpeed = 0;
        int                 fastestBattleSpeed = 0;
    };

    EstimatedParams estimated;
    EstimatedParamsSquad estimatedFromSquad;
};

using AdventureHeroConstPtr = const AdventureHero*;
using AdventureHeroMutablePtr = AdventureHero*;

}
