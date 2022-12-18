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
ENUM_REFLECTION_STRINGIY(BattleDirection,
    None,
    TR,
    R,
    BR,
    BL,
    L,
    TL
)
ENUM_REFLECTION_STRINGIY(BattleAttackDirection,
    None,
    TR,
    R,
    BR,
    BL,
    L,
    TL,
    T,
    B
)
ENUM_REFLECTION_PAIRED(BattlePositionExtended::Side,
    Right,
    "r"  ,  Right,
    "l"  ,  Left
)
ENUM_REFLECTION_PAIRED(BattlePositionExtended::Sight,
    ToRight,
    "tr"  ,  ToRight,
    "tl"  ,  ToLeft
)
ENUM_REFLECTION_PAIRED(BattlePositionExtended::Sub,
    Main,
    "main"  ,  Main,
    "sec"   ,  Secondary
)
ENUM_REFLECTION_PAIRED( BattlePlanAttackParams::Alteration,
    None,
    "none"         , None,
    "forceMelee"   , ForceMelee,
    "freeAttack"   , FreeAttack
)

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
