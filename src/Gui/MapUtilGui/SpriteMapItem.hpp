/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsItem>

#include <compare>
#include <set>
#include <vector>

namespace FreeHeroes {

namespace Gui {
class LibraryModelsProvider;
}

struct SpriteMap;
class SpriteMapItem : public QGraphicsItem {
public:
    SpriteMapItem(const SpriteMap* spriteMap)
        : m_spriteMap(spriteMap)
    {
    }

    // QGraphicsItem interface
public:
    QRectF boundingRect() const override;
    void   paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    const SpriteMap* const m_spriteMap;
    static constexpr qreal tileWidth      = 32.;
    int                    m_currentDepth = 0;
};

}
