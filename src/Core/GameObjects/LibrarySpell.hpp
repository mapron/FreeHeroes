/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Stat.hpp"
#include "LibraryFwd.hpp"

#include <string>
#include <map>
#include <vector>
#include <set>
#include <iterator>

namespace FreeHeroes::Core {

struct LibrarySpell {
    // clang-format off
    enum class Type        { Temp, Offensive, Special, Summon, Rising, Adventure };
    enum class Qualify     { None, Good, Bad };
    enum class TargetClass { Units, Land, Immediate, None };
    enum class Range       { Single, R1, R1NoCenter, R2, R3, Obstacle2, Obstacle3, Chain4, Chain5, All };
    enum class Tag         { Mind, Vision, Ice, Lightning, AirElem, FireElem };
    enum class EndCondition{ Time, GetHit, MakingAttack };
    // clang-format on

    struct Presentation {
        std::string iconBonus;
        std::string iconInt;
        std::string iconTrans;
        std::string iconScroll;

        std::string animation;
        std::string sound;

        std::string bottomAnimation;
        std::string projectile;
        bool        bottomPosition          = false;
        bool        animationOnMainPosition = false;
        bool        tile                    = false;

        int configOrder = 0;
        int order       = 0;
    };

    std::string id;
    std::string untranslatedName;

    // Teachable means spell is obtainable from the Mage Guild, scrolls, etc.
    // Otherwise it can be only on a unit or an artifact.
    bool isTeachable = true;

    Type type = Type::Special;

    Qualify qualify = Qualify::None;

    TargetClass targetClass = TargetClass::None;

    MagicSchool school = MagicSchool::Any;

    std::vector<Tag> tags;

    int level    = 0;
    int manaCost = 0;

    bool indistinctive = false;

    std::vector<std::string>  calcScript;
    std::vector<std::string>  filterScript;
    std::vector<Range>        rangeByLevel;
    std::vector<EndCondition> endConditions;

    std::vector<LibrarySpellConstPtr> counterSpells;
    std::vector<BonusRatio>           retaliationWhenCancel;
    LibraryUnitConstPtr               summonUnit = nullptr;

    Presentation presentationParams;

    bool hasEndCondition(EndCondition condition) const { return std::find(endConditions.cbegin(), endConditions.cend(), condition) != endConditions.cend(); }
};

struct SpellCastParams {
    LibrarySpellConstPtr spell         = nullptr;
    int                  spellPower    = 1;
    int                  skillLevel    = 0; // 0-3
    int                  durationBonus = 0;
    int                  heroSpecLevel = -1;
    bool                 spPerUnit     = false;

    // reference only
    LibraryArtifactConstPtr art = nullptr;
};
using SpellCastParamsList = std::vector<SpellCastParams>;

struct SpellFilter {
    std::vector<LibrarySpellConstPtr> onlySpells;
    std::vector<LibrarySpellConstPtr> notSpells;
    std::vector<int>                  levels;
    std::vector<MagicSchool>          schools;
    std::vector<LibrarySpell::Tag>    tags;
    bool                              teachableOnly = false;
    bool                              all           = false;

    bool isDefault() const
    {
        return onlySpells.empty()
               && notSpells.empty()
               && levels.empty()
               && tags.empty()
               && schools.empty()
               && !teachableOnly
               && !all;
    }

    bool contains(LibrarySpellConstPtr spell) const
    {
        if (!spell)
            return false;

        if (isDefault())
            return false;

        if (containsAll())
            return true;

        bool result = true;
        if (!onlySpells.empty()) {
            result = result && std::find(onlySpells.cbegin(), onlySpells.cend(), spell) != onlySpells.cend();
        }
        if (!notSpells.empty()) {
            result = result && std::find(notSpells.cbegin(), notSpells.cend(), spell) == notSpells.cend();
        }
        if (!levels.empty()) {
            result = result && std::find(levels.cbegin(), levels.cend(), spell->level) != levels.cend();
        }
        if (!schools.empty()) {
            result = result && std::find(schools.cbegin(), schools.cend(), spell->school) != schools.cend();
        }
        if (!tags.empty()) {
            bool tagsMatch = false;
            for (auto tag : tags) {
                if (std::find(spell->tags.cbegin(), spell->tags.cend(), tag) != spell->tags.cend()) {
                    tagsMatch = true;
                    break;
                }
            }
            result = result && tagsMatch;
        }
        if (teachableOnly) {
            result = result && spell->isTeachable;
        }

        return result;
    }

    bool containsAll() const
    {
        if (isDefault())
            return false;

        if (all)
            return true;

        if (!levels.empty()) {
            auto levelsTmp = levels;
            std::sort(levelsTmp.begin(), levelsTmp.end());
            if (levelsTmp == std::vector{ 1, 2, 3, 4, 5 })
                return true;
        }
        if (!schools.empty()) {
            auto schoolsTmp = schools;
            std::sort(schoolsTmp.begin(), schoolsTmp.end());
            if (schoolsTmp == std::vector{ Core::MagicSchool::Air, Core::MagicSchool::Earth, Core::MagicSchool::Fire, Core::MagicSchool::Water })
                return true;
        }

        return false;
    }

    bool containsMind() const
    {
        if (isDefault())
            return false;
        if (containsAll())
            return true;
        if (std::find(tags.cbegin(), tags.cend(), LibrarySpell::Tag::Mind) != tags.cend())
            return true;

        return false;
    }

    bool contains(MagicSchool school) const
    {
        if (containsAll())
            return true;

        return std::find(schools.cbegin(), schools.cend(), school) != schools.cend();
    }

    std::set<LibrarySpellConstPtr> filterPossible(const std::vector<LibrarySpell*>& allPossibleSpells)
    {
        if (isDefault())
            return {};

        std::set<LibrarySpellConstPtr> populatedFilter;
        for (auto spell : allPossibleSpells) {
            if (contains(spell))
                populatedFilter.insert(spell);
        }
        return populatedFilter;
    }

    void makeUnion(const SpellFilter& another)
    {
        if (another.isDefault())
            return;

        all = all || another.all;

        for (auto spell : another.onlySpells) {
            if (std::find(onlySpells.cbegin(), onlySpells.cend(), spell) == onlySpells.cend())
                onlySpells.push_back(spell);
        }
        auto tmp = notSpells;
        for (auto spell : tmp) {
            if (another.contains(spell))
                notSpells.erase(std::find(notSpells.cbegin(), notSpells.cend(), spell));
        }
        for (auto level : another.levels) {
            if (std::find(levels.cbegin(), levels.cend(), level) == levels.cend())
                levels.push_back(level);
        }
        std::sort(levels.begin(), levels.end());
        for (auto school : another.schools) {
            if (std::find(schools.cbegin(), schools.cend(), school) == schools.cend())
                schools.push_back(school);
        }
        std::sort(schools.begin(), schools.end());
        for (auto tag : another.tags) {
            if (std::find(tags.cbegin(), tags.cend(), tag) == tags.cend())
                tags.push_back(tag);
        }

        auto onlySpellsCopy = onlySpells;
        onlySpells.clear();
        if (isDefault()) {
            onlySpells = onlySpellsCopy;
            return;
        }
        for (auto spell : onlySpellsCopy) {
            if (!contains(spell))
                onlySpells.push_back(spell);
        }
    }
};

struct ImmunitiesParams {
    SpellFilter general;
    SpellFilter withoutBreakable;
    bool        brokenGeneral  = false;
    bool        brokenPositive = false;

    const SpellFilter& determine(bool positive) const
    {
        if (positive) {
            if (brokenPositive)
                return withoutBreakable;
            else
                return general;
        }
        if (brokenGeneral)
            return withoutBreakable;
        else
            return general;
    }

    bool immuneTo(LibrarySpellConstPtr spell) const
    {
        if (!spell)
            return false;

        const bool positive = spell->qualify == LibrarySpell::Qualify::Good;
        return determine(positive).contains(spell);
    }

    bool immuneTo(MagicSchool school, bool positive) const
    {
        return determine(positive).contains(school);
    }
};

}
