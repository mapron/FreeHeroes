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

        int         order = 0;
        std::string portrait;
        std::string portraitSmall;

        Gender gender = Gender::Unspec;
    };

    std::string id;
    std::string untranslatedName;

    LibraryFactionConstPtr faction = nullptr;

    bool isFighter = true;

    LibraryHeroSpecConstPtr spec = nullptr;

    HeroSkillsList secondarySkills;

    LibrarySpellConstPtr startSpell = nullptr;

    StartStacks startStacks;

    Presentation presentationParams;

    LibraryFactionHeroClassConstPtr heroClass() const noexcept
    {
        return isFighter ? &faction->fighterClass : &faction->mageClass;
    }
};

}
