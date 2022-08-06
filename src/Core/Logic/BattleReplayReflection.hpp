#pragma once

#include "BattleReflection.hpp"

#include "BattleReplay.hpp"

namespace FreeHeroes::Core::Reflection {
// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<BattleReplayData::EventRecord::Type> = EnumTraits::make(
   BattleReplayData::EventRecord::Type::Guard,
    "guard"       ,   BattleReplayData::EventRecord::Type::Guard,
    "wait"        ,   BattleReplayData::EventRecord::Type::Wait,
    "move"        ,   BattleReplayData::EventRecord::Type::MoveAttack,
    "cast"        ,   BattleReplayData::EventRecord::Type::Cast
);

template<>
inline constexpr const std::tuple MetaInfo::s_fields<BattleReplayData::EventRecord>{
    Field("type"    , &BattleReplayData::EventRecord::type),
    Field("move"    , &BattleReplayData::EventRecord::moveParams  ),
    Field("attack"  , &BattleReplayData::EventRecord::attackParams),
    Field("cast"    , &BattleReplayData::EventRecord::castParams  ),
};
// clang-format on
}
