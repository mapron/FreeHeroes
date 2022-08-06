/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureReflection.hpp"

#include "IBattleControl.hpp"
#include "LibrarySpell.hpp"
#include "BattleField.hpp"

namespace FreeHeroes::Core::Reflection {

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattleDirection> = EnumTraits::make(
    BattleDirection::None,
    ""          ,   BattleDirection::None,
    "TR"        ,   BattleDirection::TR,
    "R"         ,   BattleDirection::R,
    "BR"        ,   BattleDirection::BR,
    "BL"        ,   BattleDirection::BL,
    "L"         ,   BattleDirection::L,
    "TL"        ,   BattleDirection::TL
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattleAttackDirection> = EnumTraits::make(
    BattleAttackDirection::None,
    ""          ,   BattleAttackDirection::None,
    "TR"        ,   BattleAttackDirection::TR,
    "R"         ,   BattleAttackDirection::R,
    "BR"        ,   BattleAttackDirection::BR,
    "BL"        ,   BattleAttackDirection::BL,
    "L"         ,   BattleAttackDirection::L,
    "TL"        ,   BattleAttackDirection::TL,
    "T"         ,   BattleAttackDirection::T,
    "B"         ,   BattleAttackDirection::B
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattlePositionExtended::Side> = EnumTraits::make(
    BattlePositionExtended::Side::Right,
    "r"        ,   BattlePositionExtended::Side::Right,
    "l"        ,   BattlePositionExtended::Side::Left
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattlePositionExtended::Sight> = EnumTraits::make(
    BattlePositionExtended::Sight::ToRight,
    "tr"        ,   BattlePositionExtended::Sight::ToRight,
    "tl"        ,   BattlePositionExtended::Sight::ToLeft
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattlePositionExtended::Sub> = EnumTraits::make(
    BattlePositionExtended::Sub::Main,
    "main"        ,   BattlePositionExtended::Sub::Main,
    "sec"         ,   BattlePositionExtended::Sub::Secondary
    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattlePosition>{
    Field("x"    , &BattlePosition::x),
    Field("y"    , &BattlePosition::y),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattlePositionExtended>{
    SetGet("main"    , &BattlePositionExtended::setMainPos, &BattlePositionExtended::mainPos       ), // (metadata("reassign", true))
    SetGet("sight"   , &BattlePositionExtended::setSight  , &BattlePositionExtended::sightDirection),
    SetGet("large"   , &BattlePositionExtended::setLarge  , &BattlePositionExtended::isLarge       ),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattleFieldGeometry>{
    Field("w"    , &BattleFieldGeometry::width),
    Field("h"    , &BattleFieldGeometry::height),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattleFieldPreset>{
    Field("obstacles"    , &BattleFieldPreset::obstacles),
    Field("field"        , &BattleFieldPreset::field),
    Field("layout"       , &BattleFieldPreset::layout),
};

template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattlePlanAttackParams::Alteration> = EnumTraits::make(
    BattlePlanAttackParams::Alteration::None,
    "none"         ,   BattlePlanAttackParams::Alteration::None,
    "forceMelee"   ,   BattlePlanAttackParams::Alteration::ForceMelee,
    "freeAttack"   ,   BattlePlanAttackParams::Alteration::FreeAttack
    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattlePlanAttackParams>{
    Field("target"    , &BattlePlanAttackParams::m_attackTarget),
    Field("dir"       , &BattlePlanAttackParams::m_attackDirection),
    Field("alt"       , &BattlePlanAttackParams::m_alteration),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattlePlanMoveParams>{
    Field("target"       , &BattlePlanMoveParams::m_movePos),
    Field("targetDbg"    , &BattlePlanMoveParams::m_moveFrom),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattlePlanCastParams>{
    Field("target"    , &BattlePlanCastParams::m_target),
    Field("spell"     , &BattlePlanCastParams::m_spell),//(metadata("optional", true))
    Field("isHeroCast", &BattlePlanCastParams::m_isHeroCast),
    Field("isUnitCast", &BattlePlanCastParams::m_isUnitCast),
};
// clang-format on

}
