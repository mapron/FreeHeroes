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

// return position between from and to, in fraction (nom/den) of distance.
constexpr inline FHPos posMidPoint(FHPos from, FHPos to, int nom, int den)
{
    int64_t dx = to.m_x - from.m_x;
    int64_t dy = to.m_y - from.m_y;
    dx         = dx * nom / den;
    dy         = dy * nom / den;
    return FHPos{ static_cast<int>(from.m_x + dx), static_cast<int>(from.m_y + dy), from.m_z };
}

enum class FHPosDirection
{
    Invalid,
    T,
    TR,
    R,
    BR,
    B,
    BL,
    L,
    TL,
};

// assume coorditane plane oriented from top-left corner to bottom-right
constexpr FHPosDirection posDirectionTo(const FHPos& from, const FHPos& to) noexcept
{
    if (from == to || from.m_z != to.m_z)
        return FHPosDirection::Invalid;

    const int dx = to.m_x - from.m_x;
    const int dy = to.m_y - from.m_y;

    if (dx == 0 && dy == 0)
        return FHPosDirection::Invalid;
    if (dx == 0) {
        return dy > 0 ? FHPosDirection::B : FHPosDirection::T;
    }
    if (dy == 0) {
        return dx > 0 ? FHPosDirection::R : FHPosDirection::L;
    }

    const int  absDx      = dx > 0 ? dx : -dx;
    const int  absDy      = dy > 0 ? dy : -dy;
    const bool horizontal = absDx > absDy;

    // 2/5 is a approximation for tangent(22.5)=0.414
    // 5/2 is a approximation for tangent(67.5)=2.41
    // diagonal = atan of absolute shifts somewhat between 22.5 and 67.5
    const bool diag = (absDx * 5 > absDy * 2) && (absDx * 2 < absDy * 5);
    if (diag) {
    }

    if (dx >= 0 && dy >= 0) { // Bottom-right square
        return diag ? FHPosDirection::BR : (horizontal ? FHPosDirection::R : FHPosDirection::B);
    } else if (dx <= 0 && dy <= 0) { // Top-left square
        return diag ? FHPosDirection::TL : (horizontal ? FHPosDirection::L : FHPosDirection::T);
    } else if (dx <= 0 && dy >= 0) { // Bottom-left square
        return diag ? FHPosDirection::BL : (horizontal ? FHPosDirection::L : FHPosDirection::B);
    } else { // Top-right square
        return diag ? FHPosDirection::TR : (horizontal ? FHPosDirection::R : FHPosDirection::T);
    }
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
