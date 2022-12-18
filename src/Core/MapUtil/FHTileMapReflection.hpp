/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTileMap.hpp"

#include "Reflection/EnumTraits.hpp"
#include "Reflection/MetaInfo.hpp"

namespace FreeHeroes::Core::Reflection {

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHRiverType> = EnumTraits::make(
    FHRiverType::Invalid,
    "Water"   , FHRiverType::Water,
    "Ice"     , FHRiverType::Ice,
    "Mud"     , FHRiverType::Mud,
    "Lava"    , FHRiverType::Lava
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHRoadType> = EnumTraits::make(
    FHRoadType::Invalid,
    "Dirt"         , FHRoadType::Dirt,
    "Gravel"       , FHRoadType::Gravel,
    "Cobblestone"  , FHRoadType::Cobblestone
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHPos>{
    Field("x", &FHPos::m_x),
    Field("y", &FHPos::m_y),
    Field("z", &FHPos::m_z),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHZone>{
    Field("id", &FHZone::m_id),
    Field("terrainId", &FHZone::m_terrainId),
    Field("tiles", &FHZone::m_tiles),
    Field("tilesVariants", &FHZone::m_tilesVariants),
    Field("rect", &FHZone::m_rect),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHRiver>{
    Field("type", &FHRiver::m_type),
    Field("tiles", &FHRiver::m_tiles),
    Field("tilesVariants", &FHRiver::m_tilesVariants),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHRoad>{
    Field("type", &FHRoad::m_type),
    Field("tiles", &FHRoad::m_tiles),
    Field("tilesVariants", &FHRoad::m_tilesVariants),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHTileMap>{
    Field("width", &FHTileMap::m_width),
    Field("height", &FHTileMap::m_height),
    Field("depth", &FHTileMap::m_depth),
};

}
