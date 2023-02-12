/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Stat.hpp"
#include "LibraryFwd.hpp"
#include "LibraryUnit.hpp"
#include "LibraryFaction.hpp"
#include "TranslationMap.hpp"

#include <string>
#include <vector>
#include <compare>

namespace FreeHeroes::Core {

struct SkillHeroItem {
    LibrarySecondarySkillConstPtr skill = nullptr;
    int                           level = 0; // 0 = basic.
    SkillHeroItem()                     = default;
    SkillHeroItem(LibrarySecondarySkillConstPtr skillPtr, int level)
        : skill(skillPtr)
        , level(level)
    {}

    constexpr auto asTuple() const noexcept { return std::tie(skill, level); }
    constexpr auto asTuple() noexcept { return std::tie(skill, level); }

    friend constexpr bool operator==(const SkillHeroItem& lh, const SkillHeroItem& rh) noexcept { return lh.asTuple() == rh.asTuple(); }
    friend constexpr bool operator!=(const SkillHeroItem& lh, const SkillHeroItem& rh) noexcept { return !(lh == rh); }
};
using HeroSkillsList = std::vector<SkillHeroItem>;

struct LibraryHero {
    struct StartStack {
        LibraryUnitConstPtr        unit = nullptr;
        LibraryUnit::HeroStackSize stackSize;
    };
    using StartStacks = std::vector<StartStack>;
    struct Presentation {
        enum class Gender
        {
            Female,
            Male,
            Unspec
        };

        int            order = 0;
        std::string    portrait;
        std::string    portraitSmall;
        TranslationMap name;
        TranslationMap bio;

        Gender gender = Gender::Unspec;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    LibraryFactionConstPtr faction = nullptr;

    bool isWarrior = true;

    LibraryHeroSpecConstPtr spec = nullptr;

    HeroSkillsList secondarySkills;

    LibrarySpellConstPtr startSpell = nullptr;

    StartStacks startStacks;

    bool isWaterContent     = false;
    bool isEnabledByDefault = true;

    Presentation presentationParams;

    LibraryFactionHeroClassConstPtr heroClass() const noexcept
    {
        return isWarrior ? &faction->warriorClass : &faction->mageClass;
    }

    auto getAdventureSprite() const
    {
        return presentationParams.gender == Presentation::Gender::Male ? heroClass()->presentationParams.adventureSpriteMale : heroClass()->presentationParams.adventureSpriteFemale;
    }

    bool sortLess(const LibraryHero& other) const noexcept
    {
        if (faction != other.faction)
            return faction->generatedOrder < other.faction->generatedOrder;

        if (isWarrior != other.isWarrior)
            return isWarrior > other.isWarrior;
        return presentationParams.order < other.presentationParams.order;
    }
};

}
