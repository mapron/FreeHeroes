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
class IGraphicsLibrary;
}

class AdventureMap;
class AdventureMapItem : public QGraphicsItem {
public:
    AdventureMapItem(AdventureMap& adventureMap, Gui::IGraphicsLibrary& graphicsLibrary)
        : m_adventureMap(adventureMap)
        , m_graphicsLibrary(graphicsLibrary)
    {
    }

    // QGraphicsItem interface
public:
    QRectF boundingRect() const override;
    void   paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    const AdventureMap&    m_adventureMap;
    Gui::IGraphicsLibrary& m_graphicsLibrary;
    static constexpr qreal tileWidth = 32.;
    //  static constexpr QSizeF tileSize {tileWidth, tileWidth};
    int m_currentDepth = 0;
};

}
