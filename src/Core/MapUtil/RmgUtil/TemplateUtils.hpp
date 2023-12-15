/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHPos.hpp"

#include <cmath>

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
        result = static_cast<int64_t>(sqrtl(value)) - 1;
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

/**
Implements the 5-order polynomial approximation to sin(x).
@param i   angle (with 2^15 units/circle)
@return    16 bit fixed point Sine value (4.12) (ie: +4096 = +1 & -4096 = -1)
@author Andrew Steadman 
@link https://www.nullhardware.com/blog/fixed-point-sine-and-cosine-for-embedded-systems

The result is accurate to within +- 1 count. ie: +/-2.44e-4.
*/
constexpr inline int16_t fpsin_fixed(int16_t i)
{
    /* Convert (signed) input to a value between 0 and 8192. (8192 is pi/2, which is the region of the curve fit). */
    /* ------------------------------------------------------------------- */
    i <<= 1;
    uint8_t c = i < 0; //set carry for output pos/neg

    if (i == (i | 0x4000)) // flip input value to corresponding value in range [0..8192)
        i = (1 << 15) - i;
    i = (i & 0x7FFF) >> 1;
    /* ------------------------------------------------------------------- */

    /* The following section implements the formula:
     = y * 2^-n * ( A1 - 2^(q-p)* y * 2^-n * y * 2^-n * [B1 - 2^-r * y * 2^-n * C1 * y]) * 2^(a-q)
    Where the constants are defined as follows:
    */
    constexpr const uint32_t A1 = 3370945099UL,
                             B1 = 2746362156UL,
                             C1 = 292421UL;
    constexpr const uint32_t n  = 13,
                             p  = 32,
                             q  = 31,
                             r  = 3,
                             a  = 12;

    uint32_t y = (C1 * ((uint32_t) i)) >> n;
    y          = B1 - (((uint32_t) i * y) >> r);
    y          = (uint32_t) i * (y >> n);
    y          = (uint32_t) i * (y >> n);
    y          = A1 - (y >> (p - q));
    y          = (uint32_t) i * (y >> n);
    y          = (y + (1UL << (q - a - 1))) >> (q - a); // Rounding

    int16_t result = y;
    return c ? -result : result;
}

constexpr inline int16_t fpcos_fixed(int16_t i)
{
    return fpsin_fixed((int16_t) (((uint16_t) (i)) + 8192U));
}

/**
Wrapper over fpsin_fixed
@param i   angle int degrees (0...360)
@return    sine value multiplied by 10000 (-10000...+10000)
*/
constexpr int64_t fpsin_deg_impl(int64_t deg)
{
    return static_cast<int64_t>(fpsin_fixed(static_cast<int16_t>(deg * 8192 / 90))) * 10000 / 4096;
}

/// wrapper over fpsin_deg_impl to provide fixed values for common inputs
constexpr int64_t fpsin_deg(int64_t deg)
{
    switch (deg) {
        case 0:
        case 180:
        case 360:
            return 0;
        case 30:
        case 150:
            return 5000;
        case 45:
        case 135:
            return 7071;
        case 60:
        case 120:
            return 8660;
        case 90:
            return 10000;
        case 210:
        case 330:
            return -5000;
        case 225:
        case 315:
            return -7071;
        case 240:
        case 300:
            return -8660;
        case 270:
            return -10000;
    }
    return fpsin_deg_impl(deg);
}

constexpr int64_t fpcos_deg(int64_t deg)
{
    return fpsin_deg(deg > 270 ? deg - 270 : deg + 90);
}

constexpr inline int64_t posDistance(const FHPos& from, const FHPos& to, int64_t mult = 1)
{
    const auto dx = mult * (from.m_x - to.m_x);
    const auto dy = mult * (from.m_y - to.m_y);
    return intSqrt(dx * dx + dy * dy);
}

/// radius is 10000. degree=0  => (10000, 0)
constexpr FHPos radiusVector(int degree) noexcept
{
    return { static_cast<int>(fpcos_deg(degree)), static_cast<int>(fpsin_deg(degree)) };
}

constexpr FHPos radiusVector(FHPosDirection direction) noexcept
{
    return radiusVector(directionToDegree(direction));
}

}
