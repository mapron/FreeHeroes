/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SpriteMap.hpp"

class QPainter;

namespace FreeHeroes {

class SpriteMapPainter {
public:
    SpriteMapPainter(const SpritePaintSettings* settings, int depth)
        : m_settings(settings)
        , m_depth(depth)
    {}

    void paint(QPainter*        painter,
               const SpriteMap* spriteMap,
               uint32_t         animationFrameOffsetTerrain,
               uint32_t         animationFrameOffsetObjects) const;

private:
    const SpritePaintSettings* m_settings;
    const int                  m_depth;
};

}
