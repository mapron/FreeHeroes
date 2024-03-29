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

namespace Mernel::Reflection {

// clang-format off
ENUM_REFLECTION_STRINGIFY(BattleDirection,
    None,
    TR,
    R,
    BR,
    BL,
    L,
    TL
)
ENUM_REFLECTION_STRINGIFY(BattleAttackDirection,
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

STRUCT_REFLECTION_PAIRED(
    BattlePosition,
    "x"                   , x,
    "y"                   , y
)

template<>
struct MetaInfo::MetaFields<BattlePositionExtended> {
static inline constexpr const std::tuple s_fields{
    SetGetNoexcept("main"    , &BattlePositionExtended::setMainPos, &BattlePositionExtended::mainPos       ),
    SetGetNoexcept("sight"   , &BattlePositionExtended::setSight  , &BattlePositionExtended::sightDirection),
    SetGetNoexcept("large"   , &BattlePositionExtended::setLarge  , &BattlePositionExtended::isLarge       ),
};
};
STRUCT_REFLECTION_PAIRED(
    BattleFieldGeometry,
    "w"                   , width,
    "h"                   , height
)
STRUCT_REFLECTION_STRINGIFY(
    BattleFieldPreset,
    obstacles,
    field,
    layout
)
STRUCT_REFLECTION_PAIRED(
    BattlePlanAttackParams,
    "target"                   , m_attackTarget,
    "dir"                      , m_attackDirection,
    "alt"                      , m_alteration
)
STRUCT_REFLECTION_PAIRED(
    BattlePlanMoveParams,
    "target"                   , m_movePos,
    "targetDbg"                , m_moveFrom
)
STRUCT_REFLECTION_PAIRED(
    BattlePlanCastParams,
    "target"                    , m_target,
    "spell"                     , m_spell,
    "isHeroCast"                , m_isHeroCast,
    "isUnitCast"                , m_isUnitCast
)
// clang-format on

}
