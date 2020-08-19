/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleReflection.hpp"
#include "LibraryReflection.hpp"

#include "IBattleControl.hpp"
#include "LibrarySpell.hpp"
#include "BattleField.hpp"

#include <rttr/registration>

using namespace rttr;

#define ref policy::prop::as_reference_wrapper

RTTR_REGISTRATION
{
    using namespace FreeHeroes::Core;
    registration::enumeration<FieldLayout>("FieldLayout")
        (
        value("std"          ,   FieldLayout::Standard),
        value("obj"          ,   FieldLayout::Object),
        value("churchyard1"  ,   FieldLayout::Churchyard1),
        value("churchyard2"  ,   FieldLayout::Churchyard2),
        value("ruins"        ,   FieldLayout::Ruins),
        value("spit"         ,   FieldLayout::Spit)
        );

    registration::enumeration<BattleDirection>("BattleDirection")
        (
        value(""          ,   BattleDirection::None),
        value("TR"        ,   BattleDirection::TR),
        value("R"         ,   BattleDirection::R),
        value("BR"        ,   BattleDirection::BR),
        value("BL"        ,   BattleDirection::BL),
        value("L"         ,   BattleDirection::L),
        value("TL"        ,   BattleDirection::TL)
        );
    registration::enumeration<BattleAttackDirection>("BattleAttackDirection")
        (
        value(""          ,   BattleAttackDirection::None),
        value("TR"        ,   BattleAttackDirection::TR),
        value("R"         ,   BattleAttackDirection::R),
        value("BR"        ,   BattleAttackDirection::BR),
        value("BL"        ,   BattleAttackDirection::BL),
        value("L"         ,   BattleAttackDirection::L),
        value("TL"        ,   BattleAttackDirection::TL),
        value("T"         ,   BattleAttackDirection::T),
        value("B"         ,   BattleAttackDirection::B)
        );
    registration::enumeration<BattlePositionExtended::Side>("BattlePosSide")
        (
        value("r"        ,   BattlePositionExtended::Side::Right),
        value("l"        ,   BattlePositionExtended::Side::Left)
        );
    registration::enumeration<BattlePositionExtended::Sight>("BattlePosSight")
        (
        value("tr"        ,   BattlePositionExtended::Sight::ToRight),
        value("tl"        ,   BattlePositionExtended::Sight::ToLeft)
        );
    registration::enumeration<BattlePositionExtended::Sub>("BattlePosSub")
        (
        value("main"        ,   BattlePositionExtended::Sub::Main),
        value("sec"         ,   BattlePositionExtended::Sub::Secondary)
        );
    registration::class_<BattlePosition>("BattlePosition")
        .constructor<>()
        .property("x"    , &BattlePosition::x)
        .property("y"    , &BattlePosition::y)
        ;
    registration::class_<BattlePositionExtended>("BattlePositionExtended")
        .constructor<>()
        .property("main"    , &BattlePositionExtended::mainPos       , &BattlePositionExtended::setMainPos)(metadata("reassign", true))
        .property("sight"   , &BattlePositionExtended::sightDirection, &BattlePositionExtended::setSight)
        .property("large"   , &BattlePositionExtended::isLarge       , &BattlePositionExtended::setLarge)
        ;
    registration::class_<BattleFieldGeometry>("BattleFieldGeometry")
        .constructor<>()
        .property("w"    , &BattleFieldGeometry::width)
        .property("h"    , &BattleFieldGeometry::height)
        ;
    registration::class_<BattleFieldPreset>("BattleFieldPreset")
        .constructor<>()
        .property("obstacles"    , &BattleFieldPreset::obstacles)
        .property("field"        , &BattleFieldPreset::field)(ref)
        .property("layout"       , &BattleFieldPreset::layout)
        ;

    registration::enumeration<BattlePlanAttackParams::Alteration>("BattlePlanAttackParamsAlteration")
        (
        value("none"         ,   BattlePlanAttackParams::Alteration::None),
        value("forceMelee"   ,   BattlePlanAttackParams::Alteration::ForceMelee),
        value("freeAttack"   ,   BattlePlanAttackParams::Alteration::FreeAttack)
        );
    registration::class_<BattlePlanAttackParams>("BattlePlanAttackParams")
        .constructor<>()
        .property("target"    , &BattlePlanAttackParams::m_attackTarget)(ref)
        .property("dir"       , &BattlePlanAttackParams::m_attackDirection)
        .property("alt"       , &BattlePlanAttackParams::m_alteration)
        ;

    registration::class_<BattlePlanMoveParams>("BattlePlanMoveParams")
        .constructor<>()
        .property("target"       , &BattlePlanMoveParams::m_movePos)(ref)
        .property("targetDbg"    , &BattlePlanMoveParams::m_moveFrom)(ref)
        ;
    registration::class_<BattlePlanCastParams>("BattlePlanCastParams")
        .constructor<>()
        .property("target"    , &BattlePlanCastParams::m_target)(ref)
        .property("spell"     , &BattlePlanCastParams::m_spell)(metadata("optional", true))
        .property("isHeroCast", &BattlePlanCastParams::m_isHeroCast)
        .property("isUnitCast", &BattlePlanCastParams::m_isUnitCast)
        ;

}


namespace FreeHeroes::Core::Reflection {

void battleReflectionStub() {
// make linker happy
}

}

