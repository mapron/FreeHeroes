/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapItem.hpp"
#include "SpriteMap.hpp"

#include "IGraphicsLibrary.hpp"
#include "ISprites.hpp"

#include "SpriteMapPainter.hpp"

#include <QPainter>

namespace FreeHeroes {

namespace {
struct DrawHint {
    int  group;
    bool mirror;
};
/*
DrawHint directionToHint(HeroDirection direction)
{
    switch (direction) {
        case HeroDirection::T:
            return { 0, false };
        case HeroDirection::TR:
            return { 1, false };
        case HeroDirection::R:
            return { 2, false };
        case HeroDirection::BR:
            return { 3, false };
        case HeroDirection::B:
            return { 4, false };
        case HeroDirection::BL:
            return { 3, true };
        case HeroDirection::L:
            return { 2, true };
        case HeroDirection::TL:
            return { 1, true };
    }
    return {};
}*/
}

QRectF SpriteMapItem::boundingRect() const
{
    return QRectF{ QPointF{ 0., 0. }, QSizeF(m_spriteMap->m_width, m_spriteMap->m_height) * tileWidth };
}

void SpriteMapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    SpriteMapPainter p;
    p.paint(painter, m_spriteMap);
}

}
