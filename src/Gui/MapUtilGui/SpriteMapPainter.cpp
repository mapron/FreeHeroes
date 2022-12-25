/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapPainter.hpp"

#include "SpriteMap.hpp"

#include <QPainter>

namespace FreeHeroes {

void SpriteMapPainter::paint(QPainter*        painter,
                             const SpriteMap* spriteMap,
                             uint32_t         animationFrameOffsetTerrain,
                             uint32_t         animationFrameOffsetObjects) const
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    const int tileSize = m_settings->m_tileSize;
    const int width    = spriteMap->m_width;
    const int height   = spriteMap->m_height;

    for (const auto& [rowIndex, row] : spriteMap->m_planes[m_depth].m_rows) {
        for (const auto& [colIndex, cell] : row.m_cells) {
            for (const auto& [priority, items] : cell.m_items) {
                for (const auto& item : items) {
                    auto                   sprite = item.m_sprite->get();
                    Gui::SpriteSequencePtr seq    = sprite->getFramesForGroup(item.m_spriteGroup);

                    const size_t frameIndex = item.m_animationOffset + animationFrameOffsetTerrain;
                    const auto   frame      = seq->frames[frameIndex % seq->frames.size()];

                    const QSize boundingSize = seq->boundarySize;

                    auto oldTransform = painter->transform();
                    painter->translate(colIndex * tileSize, rowIndex * tileSize);
                    // @todo:
                    //painter->translate(frame.paddingLeftTop.x(), frame.paddingLeftTop.y());
                    painter->scale(item.m_flipHor ? -1 : 1, item.m_flipVert ? -1 : 1);

                    if (item.m_flipHor) {
                        painter->translate(-boundingSize.width(), 0);
                    }
                    if (item.m_flipVert) {
                        painter->translate(0, -boundingSize.height());
                    }

                    painter->drawPixmap(frame.paddingLeftTop, frame.frame);

                    painter->setTransform(oldTransform);
                }
            }
        }
    }

    //   debug cross
    //    {
    //        painter->setPen(Qt::SolidLine);
    //        painter->drawLine(m_boundingOrigin, QPointF(m_boundingSize.width(), m_boundingSize.height()) + m_boundingOrigin);
    //        painter->drawLine(QPointF(m_boundingSize.width(), 0) + m_boundingOrigin, QPointF(0, m_boundingSize.height()) + m_boundingOrigin);
    //    }

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

    
*/
    if (m_settings->m_grid) {
        painter->setPen(QColor(0, 0, 0, 150));
        // grid
        for (int y = 0; y < height; ++y) {
            painter->drawLine(QLineF(0, y * tileSize, width * tileSize, y * tileSize));
        }
        for (int x = 0; x < width; ++x) {
            painter->drawLine(QLineF(x * tileSize, 0, x * tileSize, height * tileSize));
        }
    }
}

}
