/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "Painter.hpp"

namespace FreeHeroes {

namespace {

void blend(PixmapColor& dest, const PixmapColor& src)
{
    uint16_t alpha        = src.m_a;
    uint16_t inverseAlpha = uint16_t(255) - alpha;

    dest.m_r = static_cast<uint8_t>((inverseAlpha * dest.m_r + alpha * src.m_r) / 256);
    dest.m_g = static_cast<uint8_t>((inverseAlpha * dest.m_g + alpha * src.m_g) / 256);
    dest.m_b = static_cast<uint8_t>((inverseAlpha * dest.m_b + alpha * src.m_b) / 256);
    dest.m_a = 255;
}

}

void Painter::drawPixmap(const PixmapPoint& offset, const Pixmap& pixmap, bool flipHor, bool flipVert)
{
    int h = pixmap.m_size.m_height;
    int w = pixmap.m_size.m_width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int destX = (flipHor ? w - x - 1 : x) + offset.m_x + m_offset.m_x;
            const int destY = (flipVert ? h - y - 1 : y) + offset.m_y + m_offset.m_y;
            if (!m_canvas.inBounds(destX, destY))
                continue;

            auto& srcColor  = pixmap.get(x, y);
            auto& destColor = m_canvas.get(destX, destY);
            if (srcColor.m_color.m_a == 255)
                destColor = srcColor;
            else if (srcColor.m_color.m_a == 0)
                continue;
            else
                blend(destColor.m_color, srcColor.m_color);
        }
    }
}

void Painter::drawRect(const PixmapPoint& topLeft, const PixmapSize& size, const PixmapColor& color)
{
    if (color.m_a == 0)
        return;
    int h = size.m_height;
    int w = size.m_width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int destX = x + topLeft.m_x + m_offset.m_x;
            const int destY = y + topLeft.m_y + m_offset.m_y;
            if (!m_canvas.inBounds(destX, destY))
                continue;
            auto& destColor = m_canvas.get(destX, destY);
            if (color.m_a == 255)
                destColor.m_color = color;
            else
                blend(destColor.m_color, color);
        }
    }
}
}
