#pragma once

#include "BattleReflection.hpp"

#include "BattleReplay.hpp"

namespace FreeHeroes::Core::Reflection {
// clang-format off
ENUM_REFLECTION_PAIRED( BattleReplayData::EventRecord::Type,
    Guard,
    "guard"       ,  Guard,
    "wait"        ,  Wait,
    "move"        ,  MoveAttack,
    "cast"        ,  Cast
)

STRUCT_REFLECTION_PAIRED(
    BattleReplayData::EventRecord,
    "type"    ,   type,
    "move"    ,   moveParams  ,
    "attack"  ,   attackParams,
    "cast"    ,   castParams  
)
// clang-format on
}
