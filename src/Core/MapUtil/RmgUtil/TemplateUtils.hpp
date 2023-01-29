/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHTileMap.hpp"

namespace FreeHeroes {

inline constexpr int rotateChebyshevArcLength(int degree, int w, int h, int padding)
{
    const int halfPerimeter = (w + h) - padding * 4 - 2;
    const int arcLength     = halfPerimeter * degree / 180; // not really circle arc, but a rectangle-arc.
    return arcLength;
}

static_assert(rotateChebyshevArcLength(90, 10, 10, 2) == 5);
static_assert(rotateChebyshevArcLength(48, 20, 20, 2) == 8);
static_assert(rotateChebyshevArcLength(360, 20, 20, 2) == 60);

inline constexpr FHPos rotateChebyshev(FHPos pos, int degree, int w, int h)
{
    int        x       = pos.m_x;
    int        y       = pos.m_y;
    const bool reverse = degree < 0;
    if (reverse)
        degree = -degree;

    int paddingX = x;
    if (x > w / 2)
        paddingX = w - x - 1;
    int paddingY = y;
    if (y > h / 2)
        paddingY = h - x - 1;
    const int padding   = std::min(paddingX, paddingY);
    const int arcLength = rotateChebyshevArcLength(degree, w, h, padding);
    for (int i = 0; i < arcLength; ++i) {
        const bool T  = y <= h / 2;
        const bool L  = x <= w / 2;
        const bool R  = x > w / 2;
        const bool B  = y > h / 2;
        const bool TL = T && L;
        const bool TR = T && R;
        const bool BL = B && L;
        const bool BR = B && R;

        const int rx = w - x - 1;
        const int ry = h - y - 1;

        //std::cerr << "x=" << x << ", y=" << y << "(" << (TL ? "TL" : "") << " " << (TR ? "TR" : "") << " " << (BR ? "BR" : "") << " " << (BL ? "BL" : "") << "_" << '\n';

        if (!reverse) { // clockwise
            if (TL) {
                if (y > x)
                    y--;
                else
                    x++;
            }
            if (TR) {
                if (rx > y)
                    x++;
                else
                    y++;
            }
            if (BR) {
                if (rx < ry)
                    y++;
                else
                    x--;
            }
            if (BL) {
                if (ry < x)
                    x--;
                else
                    y--;
            }

        } else {
            if (TL) {
                if (y >= x)
                    y++;
                else
                    x--;
            }
            if (TR) {
                if (rx >= y)
                    x--;
                else
                    y--;
            }
            if (BR) {
                if (rx <= ry)
                    y--;
                else
                    x++;
            }
            if (BL) {
                if (ry <= x)
                    x++;
                else
                    y++;
            }
        }
    }
    return FHPos{ x, y, pos.m_z };
}

static_assert(rotateChebyshev(FHPos{ 2, 2 }, 48, 20, 20) == FHPos{ 10, 2 });
static_assert(rotateChebyshev(FHPos{ 2, 5 }, 48, 20, 20) == FHPos{ 7, 2 });
static_assert(rotateChebyshev(FHPos{ 2, 2 }, 180, 20, 20) == FHPos{ 17, 17 });
static_assert(rotateChebyshev(FHPos{ 2, 2 }, 360, 20, 20) == FHPos{ 2, 2 });

static_assert(rotateChebyshev(FHPos{ 2, 2 }, -48, 20, 20) == FHPos{ 2, 10 });
static_assert(rotateChebyshev(FHPos{ 2, 5 }, -48, 20, 20) == FHPos{ 2, 13 });
static_assert(rotateChebyshev(FHPos{ 2, 2 }, -360, 20, 20) == FHPos{ 2, 2 });

/// (I hope) crossplatform integral square root implementation.
/// we need this to reproduceability between different CPU's.
constexpr inline int64_t intSqrt(const int64_t value) noexcept
{
    if (value <= 0)
        return 0;
    if (value <= 3)
        return 1;

    int64_t result = 3;
    if (!std::is_constant_evaluated()) { // DO NOT 'if constexpr' here! Also, replace to 'if consteval' later.
        // speedup so runtime will only use 1-2 loop iterations.
        result = static_cast<int64_t>(std::sqrtl(value)) - 1;
    }

    int64_t estimate = result * result;

    // Starting from 1, try all numbers until
    // i*i is greater than or equal to x.

    while (estimate <= value) {
        result++;
        estimate = result * result;
    }
    return result - 1;
}

constexpr inline int64_t posDistance(const FHPos& from, const FHPos& to)
{
    const auto dx = from.m_x - to.m_x;
    const auto dy = from.m_y - to.m_y;
    return intSqrt(dx * dx + dy * dy);
}

}
