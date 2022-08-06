/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureArmy.hpp"

#include "LibraryReflection.hpp"

namespace FreeHeroes::Core::Reflection {

template<>
inline constexpr const std::tuple MetaInfo::s_fields<AdventureHero>{
    Field("primary", &AdventureHero::currentBasePrimary),
    Field("artifactsOn", &AdventureHero::artifactsOn),
    Field("artifactsBag", &AdventureHero::artifactsBag),
    Field("secSkills", &AdventureHero::secondarySkills),
    Field("spellbook", &AdventureHero::spellbook),
    Field("hasSpellBook", &AdventureHero::hasSpellBook),
    Field("level", &AdventureHero::level),
    Field("experience", &AdventureHero::experience),
    Field("mana", &AdventureHero::mana),
    Field("movePointsRemain", &AdventureHero::movePointsRemain),
    Field("thisDayMovePoints", &AdventureHero::thisDayMovePoints),
    Field("newBornHero", &AdventureHero::newBornHero),
    Field("levelupsWithoutWisdom", &AdventureHero::levelupsWithoutWisdom),
    Field("levelupsWithoutSchool", &AdventureHero::levelupsWithoutSchool),
    Field("library", &AdventureHero::library), //(metadata("optional", true))

};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<AdventureStack>{
    Field("n", &AdventureStack::count),
    Field("id", &AdventureStack::library), //(metadata("optional", true))
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<AdventureSquad>{
    Field("stacks", &AdventureSquad::stacks),
    Field("useCompactFormation", &AdventureSquad::useCompactFormation),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<AdventureArmy>{
    Field("hero", &AdventureArmy::hero),
    Field("squad", &AdventureArmy::squad),
};

}
