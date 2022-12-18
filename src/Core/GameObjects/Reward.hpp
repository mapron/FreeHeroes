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
};

struct Reward {
    ResourceAmount             resources;
    std::vector<UnitWithCount> units;
    ArtifactReward             artifacts;

    int64_t gainedExp = 0;
    int     manaDiff  = 0;

    PrimaryRngParams  rngBonus;
    HeroPrimaryParams statBonus;

    HeroSkillsList secSkills;

    SpellFilter spells;

    // no spells in calculation of this. probably confusing.
    size_t totalItems() const noexcept { return resources.nonEmptyAmount()
                                                + (units.size())
                                                + artifacts.size()
                                                + rngBonus.nonEmptyAmount()
                                                + statBonus.nonEmptyAmount()
                                                + secSkills.size(); }
};
using Rewards = std::vector<Reward>;

}