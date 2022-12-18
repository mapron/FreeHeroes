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
template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattleReplayData::EventRecord>{
    Field("type"    , &BattleReplayData::EventRecord::type),
    Field("move"    , &BattleReplayData::EventRecord::moveParams  ),
    Field("attack"  , &BattleReplayData::EventRecord::attackParams),
    Field("cast"    , &BattleReplayData::EventRecord::castParams  ),
};
// clang-format on
}
