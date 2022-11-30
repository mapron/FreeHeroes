/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "FHMap.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

namespace FreeHeroes {

namespace Core::Reflection {

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHPlayerId> = EnumTraits::make(
    FHPlayerId::Invalid,
    "Invalid"  , FHPlayerId::Invalid,
    "None"     , FHPlayerId::None,
    "Red"      , FHPlayerId::Red,
    "Blue"     , FHPlayerId::Blue,
    "Tan"      , FHPlayerId::Tan,
    "Green"    , FHPlayerId::Green,
    "Orange"   , FHPlayerId::Orange,
    "Purple"   , FHPlayerId::Purple,
    "Teal"     , FHPlayerId::Teal,
    "Pink"     , FHPlayerId::Pink
    );
// clang-format on

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<Core::GameVersion> = EnumTraits::make(
    Core::GameVersion::Invalid,
    "SOD"  , Core::GameVersion::SOD,
    "HOTA" , Core::GameVersion::HOTA
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHPos>{
    Field("x", &FHPos::m_x),
    Field("y", &FHPos::m_y),
    Field("z", &FHPos::m_z),
    Field("v", &FHPos::m_valid),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHPlayer>{
    Field("ai", &FHPlayer::m_aiPossible),
    Field("human", &FHPlayer::m_humanPossible),
    Field("factions", &FHPlayer::m_startingFactions),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHWandering>{
    Field("pos", &FHWandering::m_pos),
    Field("player", &FHWandering::m_player),
    Field("main", &FHWandering::m_isMain),
    Field("id", &FHWandering::m_id),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHTown>{
    Field("pos", &FHTown::m_pos),
    Field("player", &FHTown::m_player),
    Field("main", &FHTown::m_isMain),
    Field("faction", &FHTown::m_faction),
    Field("hasFort", &FHTown::m_hasFort),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHZone::Rect>{
    Field("pos", &FHZone::Rect::m_pos),
    Field("w", &FHZone::Rect::m_width),
    Field("h", &FHZone::Rect::m_height),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHZone>{
    Field("id", &FHZone::m_id),
    Field("terrain", &FHZone::m_terrain),
    Field("tiles", &FHZone::m_tiles),
    Field("rect", &FHZone::m_rect),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMap>{
    Field("version", &FHMap::m_version),
    Field("seed", &FHMap::m_seed),
    Field("width", &FHMap::m_width),
    Field("height", &FHMap::m_height),
    Field("hasUnderground", &FHMap::m_hasUnderground),
    Field("name", &FHMap::m_name),
    Field("descr", &FHMap::m_descr),
    Field("players", &FHMap::m_players),
    Field("wanderingHeroes", &FHMap::m_wanderingHeroes),
    Field("towns", &FHMap::m_towns),
    Field("zones", &FHMap::m_zones),
    Field("defaultTerrain", &FHMap::m_defaultTerrain),
};

template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::PlayersMap>{ true };

}

void FHMap::toJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void FHMap::fromJson(const PropertyTree& data)
{
    Core::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

std::set<FHPos> FHZone::getResolved() const
{
    std::set<FHPos> result;
    for (auto& pos : m_tiles)
        result.insert(pos);
    for (uint32_t x = 0; x < m_rect.m_width; ++x) {
        for (uint32_t y = 0; y < m_rect.m_height; ++y) {
            result.insert(FHPos{ .m_x     = x + m_rect.m_pos.m_x,
                                 .m_y     = y + m_rect.m_pos.m_y,
                                 .m_z     = m_rect.m_pos.m_z,
                                 .m_valid = true });
        }
    }
    return result;
}

}
