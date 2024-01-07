/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SpriteMap.hpp"
#include "MapRenderUtilExport.hpp"

namespace FreeHeroes {
class Painter;

class MAPRENDERUTIL_EXPORT SpriteMapPainterPixmap {
public:
    SpriteMapPainterPixmap(const SpritePaintSettings* settings, int depth);
    ~SpriteMapPainterPixmap();

    void paint(Painter*         painter,
               const SpriteMap* spriteMap,
               uint32_t         animationFrameOffsetTerrain,
               uint32_t         animationFrameOffsetObjects) const;

private:
    const SpritePaintSettings* m_settings;
    const int                  m_depth;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
