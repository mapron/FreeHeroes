/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTileMap.hpp"

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

}
