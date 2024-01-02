/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "LibraryArtifact.hpp"
#include "LibraryHero.hpp"
#include "LibrarySpell.hpp"

#include "Reward.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

struct UnitWithCount {
    LibraryUnitConstPtr unit  = nullptr;
    int                 count = 0;

    bool operator==(const UnitWithCount&) const noexcept = default;
};
using UnitWithCountList = std::vector<UnitWithCount>;

struct UnitByValue {
    std::set<int> m_levels;
    int           m_value = 0;

    bool operator==(const UnitByValue&) const noexcept = default;
};
using UnitByValueList = std::vector<UnitByValue>;

struct Reward {
    ResourceAmount    resources;
    UnitWithCountList units;
    UnitByValueList   randomUnits;
    ArtifactReward    artifacts;

    int64_t gainedExp = 0;
    int     manaDiff  = 0;

    PrimaryRngParams  rngBonus;
    HeroPrimaryParams statBonus;

    HeroSkillsList secSkills;

    SpellFilter spells;

    enum class MovePointBehaviour
    {
        Invalid = -1,
        Give    = 0,
        Take,
        Nullify,
        Set,
        Replenish,
    };

    MovePointBehaviour movePointMode = MovePointBehaviour::Give;
    int                movePoints    = 0;

    // no spells in calculation of this. probably confusing.
    size_t totalItems() const noexcept { return resources.nonEmptyAmount()
                                                + (units.size())
                                                + artifacts.size()
                                                + rngBonus.nonEmptyAmount()
                                                + statBonus.nonEmptyAmount()
                                                + secSkills.size(); }

    bool operator==(const Reward&) const noexcept = default;
};
using Rewards = std::vector<Reward>;

}
