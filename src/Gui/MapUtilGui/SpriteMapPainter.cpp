/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapPainter.hpp"

#include "SpriteMap.hpp"

#include <QPainter>
#include <QDebug>

namespace FreeHeroes {

namespace {
// just arbitrary and subjective color tables for debug paint purpose.

const std::vector<QColor> g_neatDarkColors{
    "#FF7A66",
    "#FFC966",
    "#87D849",
    "#40BCBC",
    "#5261EA",
    "#A247C6",
    "#6D6D6D",
};
const std::vector<QColor> g_neatLightColors{
    "#FFD3CC",
    "#FFECCC",
    "#E2FFCC",
    "#C0F7F7",
    "#D9DCF9",
    "#E3C1F2",
    "#E0E0E0",
};
}

void SpriteMapPainter::paint(QPainter*        painter,
                             const SpriteMap* spriteMap,
                             uint32_t         animationFrameOffsetTerrain,
                             uint32_t         animationFrameOffsetObjects) const
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_settings->getEffectiveScale() < 100);
    const int tileSize = m_settings->m_tileSize;

    auto drawCell = [painter, tileSize, animationFrameOffsetTerrain, animationFrameOffsetObjects](const SpriteMap::Cell& cell, int x, int y) {
        for (const auto& item : cell.m_items) {
            auto sprite = item.m_sprite->get();
            if (!sprite)
                continue;
            Gui::ISprite::SpriteSequencePtr seq = sprite->getFramesForGroup(item.m_spriteGroup);
            if (!seq)
                continue;

            const auto psrHash = item.m_x * 7U + item.m_y * 13U;

            const size_t frameIndex = psrHash + (item.m_layer == SpriteMap::Layer::Terrain ? animationFrameOffsetTerrain : animationFrameOffsetObjects);
            const auto   frame      = seq->m_frames[frameIndex % seq->m_frames.size()];

            const QSize boundingSize = seq->m_boundarySize;

            auto oldTransform = painter->transform();
            painter->translate(x * tileSize, y * tileSize);

            if (item.m_shiftHalfTile) {
                painter->translate(0, tileSize / 2);
            }

            // @todo:
            //painter->translate(frame.paddingLeftTop.x(), frame.paddingLeftTop.y());
            painter->scale(item.m_flipHor ? -1 : 1, item.m_flipVert ? -1 : 1);

            if (item.m_flipHor) {
                painter->translate(-boundingSize.width(), 0);
            }
            if (item.m_flipVert) {
                painter->translate(0, -boundingSize.height());
            }
            if (boundingSize.width() > tileSize || boundingSize.height() > tileSize) {
                painter->translate(-boundingSize.width() + tileSize, -boundingSize.height() + tileSize);
            }
            painter->drawPixmap(frame.m_paddingLeftTop, frame.m_frame);

            // debug diamond
            {
                //                        QVector<QPointF> points;
                //                        points << drawOrigin + QPointF{ tileWidth / 2, 0 };
                //                        points << drawOrigin + QPointF{ tileWidth, tileWidth / 2 };
                //                        points << drawOrigin + QPointF{ tileWidth / 2, tileWidth };
                //                        points << drawOrigin + QPointF{ 0, tileWidth / 2 };
                //                        points << drawOrigin + QPointF{ tileWidth / 2, 0 };
                //                        painter->setPen(QPen(QBrush{ QColor(192, 0, 0, 255) }, 2.));
                //                        painter->setBrush(QColor(Qt::green));
                //                        painter->drawPolyline(QPolygonF(points));
                //   debug cross
                //    {
                //        painter->setPen(Qt::SolidLine);
                //        painter->drawLine(m_boundingOrigin, QPointF(m_boundingSize.width(), m_boundingSize.height()) + m_boundingOrigin);
                //        painter->drawLine(QPointF(m_boundingSize.width(), 0) + m_boundingOrigin, QPointF(0, m_boundingSize.height()) + m_boundingOrigin);
                //    }
            }

            painter->setTransform(oldTransform);
        }
    };

    auto drawGrid = [painter, spriteMap, tileSize](QColor color, int alpha) {
        const int width  = spriteMap->m_width;
        const int height = spriteMap->m_height;
        color.setAlpha(alpha);
        painter->setPen(QColor(0, 0, 0, 150));
        // grid
        for (int y = 0; y < height; ++y) {
            painter->drawLine(QLineF(0, y * tileSize, width * tileSize, y * tileSize));
        }
        for (int x = 0; x < width; ++x) {
            painter->drawLine(QLineF(x * tileSize, 0, x * tileSize, height * tileSize));
        }
    };

    // low level (terrains/roads) paint

    for (const auto& [priority, grid] : spriteMap->m_planes[m_depth].m_grids) {
        if (priority >= 0)
            break;
        for (const auto& [rowIndex, row] : grid.m_rows) {
            for (const auto& [colIndex, cell] : row.m_cells) {
                drawCell(cell, colIndex, rowIndex);
            }
        }
    }

    // middle-layer paint
    if (m_settings->m_grid && !m_settings->m_gridOnTop) {
        drawGrid(QColor(0, 0, 0), m_settings->m_gridOpacity);
    }

    // top-level (objects) paint
    for (const auto& [priority, grid] : spriteMap->m_planes[m_depth].m_grids) {
        if (priority < 0)
            continue;
        for (const auto& [rowIndex, row] : grid.m_rows) {
            for (const auto& [colIndex, cell] : row.m_cells) {
                drawCell(cell, colIndex, rowIndex);
            }
        }
    }

    // debug paint
    for (const auto& [y, row] : spriteMap->m_planes[m_depth].m_merged.m_rows) {
        for (const auto& [x, cell] : row.m_cells) {
            if (!cell.m_debug)
                continue;

            auto darkColor  = g_neatDarkColors[cell.m_debugA * 17 % g_neatDarkColors.size()];
            auto lightColor = g_neatLightColors[cell.m_debugA * 37 % g_neatLightColors.size()];

            auto oldTransform = painter->transform();
            painter->translate(x * tileSize, y * tileSize);
            const auto halfTile = tileSize / 2;

            for (int x1 = 0; x1 < 2; ++x1) {
                for (int y1 = 0; y1 < 2; ++y1) {
                    auto  color = (x1 + y1) % 2 ? darkColor : lightColor;
                    QRect cellRect(x1 * halfTile, y1 * halfTile, halfTile, halfTile);
                    painter->fillRect(cellRect, color);
                }
            }
            QRect cellRect(0, 0, tileSize, tileSize);
            painter->setPen(Qt::black);
            QFont font = painter->font();
            font.setPixelSize(8);
            painter->setFont(font);
            //
            if (cell.m_debugB == 1)
                painter->setBrush(Qt::red);
            if (cell.m_debugB == 2)
                painter->setBrush(Qt::blue);
            if (cell.m_debugB == 3)
                painter->setBrush(Qt::yellow);

            if (cell.m_debugB)
                painter->drawEllipse(cellRect);
            else
                painter->drawText(cellRect, Qt::AlignCenter, QString("A: %1\nB: %2\nC: %3").arg(cell.m_debugA).arg(cell.m_debugB).arg(cell.m_debugC));

            painter->setTransform(oldTransform);
        }
    }

    // top-level UI paint
    if (m_settings->m_grid && m_settings->m_gridOnTop) {
        drawGrid(QColor(0, 0, 0), m_settings->m_gridOpacity);
    }
}

void SpriteMapPainter::paintMinimap(QPainter* painter, const SpriteMap* spriteMap, QSize minimapSize) const
{
    QPixmap pixmap(spriteMap->m_width, spriteMap->m_height);
    auto    img = pixmap.toImage();

    for (const auto& [y, row] : spriteMap->m_planes[m_depth].m_merged.m_rows) {
        for (const auto& [x, cell] : row.m_cells) {
            if (cell.m_colorUnblocked.isValid() && cell.m_colorBlocked.isValid())
                img.setPixelColor(x, y, cell.m_blocked ? cell.m_colorBlocked : cell.m_colorUnblocked);
        }
    }

    painter->drawPixmap(QRect(QPoint(0, 0), minimapSize), QPixmap::fromImage(img));
}

}
