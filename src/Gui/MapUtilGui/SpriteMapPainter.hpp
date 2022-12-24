/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

class QPainter;

namespace FreeHeroes {

struct SpriteMap;
class SpriteMapPainter {
public:
    void paint(QPainter* painter, const SpriteMap* spriteMap) const;
};

}
