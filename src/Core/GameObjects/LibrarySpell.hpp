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
    enum class Type        { Temp, Offensive, Special, Summon, Rising, Adventure };
    enum class Qualify     { None, Good, Bad };
    enum class TargetClass { Units, Land, Immediate, None };
    enum class Range       { Single, R1, R1NoCenter, R2, R3, Obstacle2, Obstacle3, Chain4, Chain5, All };
    enum class Tag         { Mind, Vision, Ice, Lightning };

    struct Presentation {
        std::string iconBonus;
        std::string iconInt;
        std::string iconTrans;
        std::string iconScroll;

        std::string animation;
        std::string sound;
        std::string soundSpecial;

        std::string bottomAnimation;
        std::string projectile;
        bool bottomPosition = false;
        bool animationOnMainPosition = false;
        bool tile = false;

        int configOrder = 0;
        int order = 0;
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

    int level = 0;
    int manaCost = 0;

    bool indistinctive = false;

    std::vector<std::string> calcScript;
    std::vector<std::string> filterScript;
    std::vector<Range> rangeByLevel;

    std::vector<LibrarySpellConstPtr> counterSpells;

    Presentation presentationParams;
};

struct SpellCastParams {
    LibrarySpellConstPtr spell = nullptr;
    int spellPower = 1;
    int skillLevel = 0; // 0-3
    int durationBonus = 0;
    int heroSpecLevel = -1;
    int probability = 1;

    // reference only
    LibraryArtifactConstPtr art = nullptr;
};
using SpellCastParamsList = std::vector<SpellCastParams>;

struct SpellFilter {
    std::vector<LibrarySpellConstPtr> onlySpells;
    std::vector<LibrarySpellConstPtr> notSpells;
    std::vector<int> levels;
    std::vector<MagicSchool> schools;
    std::vector<LibrarySpell::Tag> tags;
    bool teachableOnly = false;

    bool isDefault() const {
        return onlySpells.empty() &&
                notSpells.empty() &&
                levels.empty() &&
                tags.empty()  &&
                schools.empty() &&
                !teachableOnly;
    }

    bool contains(LibrarySpellConstPtr spell) const {
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

    void fillFilterCache(const std::vector<LibrarySpell*> & allPossibleSpells) {
        if (isDefault())
            return;
        for (auto spell : allPossibleSpells) {
            if (contains(spell))
                populatedFilter.insert(spell);
        }
    }

    void makeUnion(const SpellFilter & another) {
        if (another.isDefault())
            return;
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
        std::set<LibrarySpellConstPtr> out;
        std::set_union(populatedFilter.cbegin(), populatedFilter.cend(),
                       another.populatedFilter.cbegin(), another.populatedFilter.cend(),
                       std::inserter(out, out.end()));
        populatedFilter = out;

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

    std::set<LibrarySpellConstPtr> populatedFilter;
};

}
