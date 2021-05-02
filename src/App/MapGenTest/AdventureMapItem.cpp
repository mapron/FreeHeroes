/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "AdventureMapItem.hpp"
#include "AdventureMap.hpp"

#include "IGraphicsLibrary.hpp"
#include "ISprites.hpp"

#include "LibraryTerrain.hpp"

#include <QPainter>

namespace FreeHeroes {

QRectF AdventureMapItem::boundingRect() const
{
    return QRectF{ QPointF{ 0., 0. }, QSizeF(m_adventureMap.width(), m_adventureMap.height()) * tileWidth };
}

void AdventureMapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    // QList<QColor> colors {QColor{82, 56, 8 }, QColor{222, 207, 140}, QColor{ 0, 65, 0}, QColor{ 181, 199, 198 }, QColor{74, 134, 107 }};
    for (int y = 0; y < m_adventureMap.height(); ++y) {
        for (int x = 0; x < m_adventureMap.width(); ++x) {
            const auto& tile            = m_adventureMap.get(x, y, m_currentDepth);
            const auto& variantSet      = tile.m_terrain->presentationParams.centerTiles;
            auto        terrainPixAsync = m_graphicsLibrary.getPixmap(variantSet[tile.m_terrainVariant % variantSet.size()]);
            assert(terrainPixAsync && terrainPixAsync->exists());
            auto pix = terrainPixAsync->get();

            const qreal drawX = x * tileWidth;
            const qreal drawY = y * tileWidth;

            painter->drawPixmap(QPointF{ drawX, drawY }, pix);
        }
    }
}

}
