/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <compare>
#include <set>

#include "PropertyTree.hpp"
#include "GameConstants.hpp"

namespace FreeHeroes {

enum class FHPlayerId
{
    Invalid = -2,
    None    = -1,
    Red     = 0,
    Blue,
    Tan,
    Green,
    Orange,
    Purple,
    Teal,
    Pink,
};
struct FHPos {
    uint32_t m_x{ 0 };
    uint32_t m_y{ 0 };
    uint8_t  m_z{ 0 };
    bool     m_valid{ false };

    constexpr auto operator<=>(const FHPos&) const = default;
};

struct FHPlayer {
    //int                      m_id{ 0 };
    bool                     m_humanPossible{ true };
    bool                     m_aiPossible{ true };
    std::vector<std::string> m_startingFactions;
};

struct FHCommonObject {
    FHPos m_pos{ .m_valid = true };
};

struct FHPlayerControlledObject : public FHCommonObject {
    FHPlayerId m_player = FHPlayerId::Invalid;
};

struct FHWandering : public FHPlayerControlledObject {
    bool        m_isMain{ false };
    std::string m_id;
};

struct FHTown : public FHPlayerControlledObject {
    bool        m_isMain{ false };
    std::string m_faction;
    bool        m_hasFort{ false };
};

struct FHZone {
    int m_id = 0;

    std::string        m_terrain;
    std::vector<FHPos> m_tiles;
    struct Rect {
        FHPos    m_pos;
        uint32_t m_width{ 0 };
        uint32_t m_height{ 0 };
    };
    Rect m_rect;

    std::set<FHPos> getResolved() const;
};

struct FHMap {
    using PlayersMap = std::map<FHPlayerId, FHPlayer>;

    Core::GameVersion m_version = Core::GameVersion::Invalid;
    uint64_t          m_seed{ 0 };

    uint32_t m_width{ 0 };
    uint32_t m_height{ 0 };
    bool     m_hasUnderground{ false };

    std::string m_name;
    std::string m_descr;

    PlayersMap               m_players;
    std::vector<FHWandering> m_wanderingHeroes;
    std::vector<FHTown>      m_towns;
    std::vector<FHZone>      m_zones;

    std::string m_defaultTerrain;

    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);
};

}
