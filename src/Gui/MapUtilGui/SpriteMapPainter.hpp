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
    SpriteMapPainter(const SpritePaintSettings* settings, int depth);
    ~SpriteMapPainter();

    void paint(QPainter*        painter,
               const SpriteMap* spriteMap,
               uint32_t         animationFrameOffsetTerrain,
               uint32_t         animationFrameOffsetObjects) const;

    void paintMinimap(QPainter*        painter,
                      const SpriteMap* spriteMap,
                      QSize            minimapSize,
                      QRectF           visible) const;

private:
    const SpritePaintSettings* m_settings;
    const int                  m_depth;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
