/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureReflection.hpp"

#include "AdventureArmy.hpp"

#include <rttr/registration>

using namespace rttr;

#define ref policy::prop::as_reference_wrapper

RTTR_REGISTRATION
{
    using namespace FreeHeroes::Core;

    registration::class_<AdventureHero>("AdventureHero")
        .constructor<>()
        .property("primary"                , &AdventureHero::currentBasePrimary)(ref)
        .property("artifactsOn"            , &AdventureHero::artifactsOn)(ref)
        .property("artifactsBag"           , &AdventureHero::artifactsBag)(ref)
        .property("secSkills"              , &AdventureHero::secondarySkills)(ref)
        .property("spellbook"              , &AdventureHero::spellbook)(ref)
        .property("hasSpellBook"           , &AdventureHero::hasSpellBook)
        .property("level"                  , &AdventureHero::level)
        .property("experience"             , &AdventureHero::experience)
        .property("mana"                   , &AdventureHero::mana)
        .property("movePointsRemain"       , &AdventureHero::movePointsRemain)
        .property("thisDayMovePoints"      , &AdventureHero::thisDayMovePoints)
        .property("newBornHero"            , &AdventureHero::newBornHero)
        .property("levelupsWithoutWisdom"  , &AdventureHero::levelupsWithoutWisdom)
        .property("levelupsWithoutSchool"  , &AdventureHero::levelupsWithoutSchool)
        .property("library"                , &AdventureHero::library)(metadata("optional", true))
        ;

    registration::class_<AdventureStack>("AdventureStack")
        .constructor<>()
        .property("n"    , &AdventureStack::count)
        .property("id"   , &AdventureStack::library)(metadata("optional", true))
        ;
    registration::class_<AdventureSquad>("AdventureSquad")
        .constructor<>()
        .property("stacks"                 , &AdventureSquad::stacks)(ref)
        .property("useCompactFormation"    , &AdventureSquad::useCompactFormation)
        ;

    registration::class_<AdventureArmy>("AdventureArmy")
        .constructor<>()
        .property("hero"           , &AdventureArmy::hero)(ref)
        .property("squad"          , &AdventureArmy::squad)(ref)
        ;


}

namespace FreeHeroes::Core::Reflection {

void adventureReflectionStub() {
// make linker happy
}

}
