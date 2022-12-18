/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureArmy.hpp"

#include "LibraryReflection.hpp"

namespace FreeHeroes::Core::Reflection {

// clang-format off
STRUCT_REFLECTION_PAIRED(
    AdventureHero,
    "primary",               currentBasePrimary,
    "artifactsOn",           artifactsOn,
    "artifactsBag",          artifactsBag,
    "secSkills",             secondarySkills,
    "spellbook",             spellbook,
    "hasSpellBook",          hasSpellBook,
    "level",                 level,
    "experience",            experience,
    "mana",                  mana,
    "movePointsRemain",      movePointsRemain,
    "thisDayMovePoints",     thisDayMovePoints,
    "newBornHero",           newBornHero,
    "levelupsWithoutWisdom", levelupsWithoutWisdom,
    "levelupsWithoutSchool", levelupsWithoutSchool,
    "library",               library 
)

STRUCT_REFLECTION_PAIRED(
    AdventureStack,
    "n",                     count,
    "id",                    library
)


STRUCT_REFLECTION_PAIRED(
    AdventureSquad,
    "stacks",                stacks,
    "useCompactFormation",   useCompactFormation
)

STRUCT_REFLECTION_PAIRED(
    AdventureArmy,
    "hero",                  hero,
    "squad",                 squad
)
// clang-format on
}
