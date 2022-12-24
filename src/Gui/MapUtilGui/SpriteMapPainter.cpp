/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapPainter.hpp"

#include "SpriteMap.hpp"

#include <QPainter>

namespace FreeHeroes {

void SpriteMapPainter::paint(QPainter* painter, const SpriteMap* spriteMap) const
{
    /*
    
    // QList<QColor> colors {QColor{82, 56, 8 }, QColor{222, 207, 140}, QColor{ 0, 65, 0}, QColor{ 181, 199, 198 }, QColor{74, 134, 107 }};
    for (int y = 0; y < m_adventureMap.height(); ++y) {
        for (int x = 0; x < m_adventureMap.width(); ++x) {
            const auto& tile       = m_adventureMap.get(x, y, m_currentDepth);
            const auto* terrainGui = m_modelsProvider.terrains()->find(tile.m_terrain);
            const auto  pix        = terrainGui->getTile(tile.m_terrainVariant);

            const qreal drawX = x * tileWidth;
            const qreal drawY = y * tileWidth;

            painter->drawPixmap(QPointF{ drawX, drawY }, pix);
        }
    }

    for (auto&& hero : m_adventureMap.m_heroes) {
        const qreal drawX      = hero.m_pos.x * tileWidth;
        const qreal drawY      = hero.m_pos.y * tileWidth;
        const auto  drawOrigin = QPointF{ drawX, drawY };

        // debug diamond
        {
            QVector<QPointF> points;
            points << drawOrigin + QPointF{ tileWidth / 2, 0 };
            points << drawOrigin + QPointF{ tileWidth, tileWidth / 2 };
            points << drawOrigin + QPointF{ tileWidth / 2, tileWidth };
            points << drawOrigin + QPointF{ 0, tileWidth / 2 };
            points << drawOrigin + QPointF{ tileWidth / 2, 0 };
            painter->setPen(QPen(QBrush{ QColor(192, 0, 0, 255) }, 2.));
            painter->setBrush(QColor(Qt::green));
            painter->drawPolyline(QPolygonF(points));
        }
        const auto [group, mirror] = directionToHint(hero.m_direction);

        const auto& libHero   = hero.m_army->hero.library;
        const auto* heroGui   = m_modelsProvider.heroes()->find(libHero);
        const auto  advSprite = heroGui->getAdventureSprite();
        const auto  seq       = advSprite->getFramesForGroup(group);
        const auto  frame     = seq->frames[0];

        const auto oldTransform = painter->transform();
        auto       drawOffset   = drawOrigin + QPointF{ -tileWidth, -tileWidth } + frame.paddingLeftTop;
        if (mirror) {
            auto t2 = oldTransform;
            t2.scale(-1, 1);
            painter->setTransform(t2);
            drawOffset = { -drawOffset.x() - tileWidth * 3, drawOffset.y() };
        }
        painter->drawPixmap(drawOffset, frame.frame);

        painter->setTransform(oldTransform);
    }

    painter->setPen(QColor(0, 0, 0, 50));

    // grid
    for (int y = 0; y < m_adventureMap.height(); ++y) {
        painter->drawLine(QLineF(0, y * tileWidth, m_adventureMap.width() * tileWidth, y * tileWidth));
    }
    for (int x = 0; x < m_adventureMap.width(); ++x) {
        painter->drawLine(QLineF(x * tileWidth, 0, x * tileWidth, m_adventureMap.height() * tileWidth));
    }
    */
}

}
