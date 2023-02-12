/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>

namespace FreeHeroes {

struct FHPos {
    int m_x{ 0 };
    int m_y{ 0 };
    int m_z{ 0 };

    constexpr size_t getHash() const noexcept
    {
        return (size_t(m_z) << 16) | (size_t(m_y) << 10) | size_t(m_x);
    }

    constexpr auto operator<=>(const FHPos&) const = default;

    std::string toPrintableString() const noexcept
    {
        return "(" + std::to_string(m_x) + ", " + std::to_string(m_y) + ", " + std::to_string(m_z) + ")";
    }
};

constexpr inline FHPos posNeighbour(FHPos point, int dx, int dy)
{
    point.m_x += dx;
    point.m_y += dy;
    return point;
}

inline FHPos operator+(const FHPos& left_, const FHPos& right_)
{
    return { left_.m_x + right_.m_x, left_.m_y + right_.m_y };
}
inline FHPos operator-(const FHPos& left_, const FHPos& right_)
{
    return { left_.m_x - right_.m_x, left_.m_y - right_.m_y };
}

[[maybe_unused]] static inline constexpr const FHPos g_invalidPos{ -1, -1, -1 };

enum class FHRiverType
{
    Invalid,
    Water,
    Ice,
    Mud,
    Lava,
};
enum class FHRoadType
{
    Invalid,
    Dirt,
    Gravel,
    Cobblestone,
};

}

namespace std {
template<>
struct hash<FreeHeroes::FHPos> {
    constexpr std::size_t operator()(const FreeHeroes::FHPos& p) const noexcept
    {
        return p.getHash();
    }
};
}
