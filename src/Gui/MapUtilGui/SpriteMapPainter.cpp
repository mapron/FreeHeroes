/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapPainter.hpp"

#include "SpriteMap.hpp"
#include "MapScore.hpp"

#include <QPainter>
#include <QDebug>
#include <QPainterPath>

namespace FreeHeroes {

struct SpriteMapPainter::Impl {
    struct CachePath {
        QPainterPath m_path;
        QFontMetrics m_metrics;
    };

    mutable std::map<QString, std::vector<CachePath>> m_cache;

    QPainterPath findCache(const QString& text, const QFontMetrics& fm)
    {
        auto it = m_cache.find(text);
        if (it == m_cache.cend())
            return {};
        auto& records = it->second;
        for (auto& rec : records) {
            if (1 || rec.m_metrics == fm)
                return rec.m_path;
        }
        return {};
    }
};

SpriteMapPainter::SpriteMapPainter(const SpritePaintSettings* settings, int depth)
    : m_settings(settings)
    , m_depth(depth)
    , m_impl(std::make_unique<Impl>())
{}

SpriteMapPainter::~SpriteMapPainter()
{
}

void SpriteMapPainter::paint(QPainter*        painter,
                             const SpriteMap* spriteMap,
                             uint32_t         animationFrameOffsetTerrain,
                             uint32_t         animationFrameOffsetObjects) const
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_settings->getEffectiveScale() < 100);
    const int tileSize = m_settings->m_tileSize;

    auto drawOverlayText = [painter, tileSize, this](const SpriteMap::Item& item, int x, const QTransform& posTransform) {
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPixelSize(item.m_overlayInfoFont);
        painter->setFont(font);
        painter->setTransform(posTransform);
        painter->translate((item.m_overlayInfoOffsetX) * tileSize, (0) * tileSize);

        QFontMetrics fm(font);

        const QString txt       = QString::fromStdString(item.m_overlayInfo);
        int           textWidth = fm.horizontalAdvance(txt);
        //int textHeight = fm.height();

        const int textX = tileSize / 2 - textWidth / 2 - 2;
        const int textY = tileSize / 2 - (tileSize / 4) * ((x + item.m_overlayInfoOffsetX) % 2);

        painter->translate(textX, textY);

        QPainterPath stroked = m_impl->findCache(txt, fm);

        if (stroked.isEmpty()) {
            QPainterPath myPath;
            myPath.addText(0, 0, font, txt);

            QPainterPathStroker stroker;
            stroker.setWidth(3);
            stroked = stroker.createStroke(myPath);
            m_impl->m_cache[txt].push_back({ stroked, fm });

            //qWarning() << "cache " << txt;
        }

        painter->setBrush(Qt::black);
        painter->setPen(Qt::NoPen);
        painter->drawPath(stroked);

        painter->setBrush(Qt::white);
        painter->setPen(Qt::white);
        painter->drawText(0, 0, txt);
    };

    auto drawHeroFlag = [painter](const QColor& color) {
        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawRect(10, 9, 10, 7);
        painter->drawPolygon(QPolygonF(QVector<QPointF>{ { 1, 10 }, { 10, 10 }, { 10, 14 } }));
        painter->drawPolygon(QPolygonF(QVector<QPointF>{ { 1, 16 }, { 10, 16 }, { 10, 12 } }));
        painter->drawRect(4, 16, 4, 1);

        painter->setBrush(color.darker(120));
        painter->drawRect(16, 9, 3, 7);
        //painter->draw
    };

    auto drawCell = [painter, tileSize, animationFrameOffsetTerrain, animationFrameOffsetObjects, &drawOverlayText, &drawHeroFlag, this](const SpriteMap::Cell& cell, int x, int y, bool isOverlayPass) {
        for (const auto& item : cell.m_items) {
            if (item.m_isOverlayItem && !m_settings->m_overlay)
                continue;

            if (item.m_flagColor.isValid()) {
                auto oldTransform = painter->transform();
                painter->translate(x * tileSize, y * tileSize);
                drawHeroFlag(item.m_flagColor);
                painter->setTransform(oldTransform);
                continue;
            }

            if (!item.m_sprite)
                continue;
            auto sprite = item.m_sprite->get();
            if (!sprite)
                continue;
            Gui::ISprite::SpriteSequencePtr seq = sprite->getFramesForGroup(item.m_spriteGroup);
            if (!seq)
                continue;
            auto containsAnyScore = [](const std::set<Core::ScoreAttr>& filter, const Core::MapScore& score) {
                for (auto& [key, val] : score)
                    if (filter.contains(key))
                        return true;
                return false;
            };
            const bool isFilteredOut = (!m_settings->m_filterLayer.empty() && !m_settings->m_filterLayer.contains(item.m_layer))
                                       || (!m_settings->m_filterGenerationId.empty() && item.m_generationId != m_settings->m_filterGenerationId)
                                       || (!m_settings->m_filterAttr.empty() && !containsAnyScore(m_settings->m_filterAttr, item.m_score));
            if (isFilteredOut && isOverlayPass) {
                continue;
            }

            const auto psrHash = item.m_x * 7U + item.m_y * 13U;

            const size_t frameIndex = psrHash + (item.m_layer == SpriteMap::Layer::Terrain ? animationFrameOffsetTerrain : animationFrameOffsetObjects);
            const auto   frame      = seq->m_frames[frameIndex % seq->m_frames.size()];

            const QSize boundingSize = seq->m_boundarySize;

            auto oldTransform = painter->transform();
            painter->translate(x * tileSize, y * tileSize);

            auto posTransform = painter->transform();

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

            double opacity = item.m_opacity;
            if (isFilteredOut) {
                if (item.m_layer == SpriteMap::Layer::Terrain)
                    opacity = 1.0;
                else if (item.m_opacity != 1.0) {
                    opacity = 0.0;
                } else {
                    opacity = 0.25;
                }
            }
            painter->setOpacity(opacity);

            if (!isOverlayPass)
                painter->drawPixmap(frame.m_paddingLeftTop, frame.m_frame);
            painter->setOpacity(1.0);
            if (!isOverlayPass && item.m_keyColor.isValid()) {
                QPixmap pix     = frame.m_frame;
                QImage  imgOrig = pix.toImage();
                pix.fill(Qt::transparent);
                QImage img = pix.toImage();

                for (int imgy = 0; imgy < img.height(); imgy++) {
                    for (int imgx = 0; imgx < img.width(); imgx++) {
                        if (imgOrig.pixelColor(imgx, imgy).alpha() == 1)
                            img.setPixelColor(imgx, imgy, item.m_keyColor);
                    }
                }
                pix = QPixmap::fromImage(img);
                painter->drawPixmap(frame.m_paddingLeftTop, pix);
            }
            if (isOverlayPass && !item.m_overlayInfo.empty())
                drawOverlayText(item, x, posTransform);

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
        for (const auto& [rowIndex, rowSlice] : grid.m_rowsSlices) {
            for (const auto& [rowPriority, row] : rowSlice.m_rows) {
                for (const auto& [colIndex, cell] : row.m_cells) {
                    drawCell(cell, colIndex, rowIndex, false);
                }
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
        for (const auto& [rowIndex, rowSlice] : grid.m_rowsSlices) {
            for (const auto& [rowPriority, row] : rowSlice.m_rows) {
                for (const auto& [colIndex, cell] : row.m_cells) {
                    drawCell(cell, colIndex, rowIndex, false);
                }
            }
        }
    }

    // item text overlay
    if (m_settings->m_overlay) {
        for (const auto& [priority, grid] : spriteMap->m_planes[m_depth].m_grids) {
            if (priority < 0)
                continue;
            for (const auto& [rowIndex, rowSlice] : grid.m_rowsSlices) {
                for (const auto& [rowPriority, row] : rowSlice.m_rows) {
                    for (const auto& [colIndex, cell] : row.m_cells) {
                        drawCell(cell, colIndex, rowIndex, true);
                    }
                }
            }
        }
    }

    // block mask
    if (m_settings->m_blockMask) {
        for (const auto& [y, row] : spriteMap->m_planes[m_depth].m_merged.m_rows) {
            for (const auto& [x, cell] : row.m_cells) {
                auto oldTransform = painter->transform();
                painter->translate(x * tileSize, y * tileSize);

                QRect cellRect(0, 0, tileSize, tileSize);
                painter->setPen(Qt::NoPen);
                if (cell.m_blocked && !cell.m_visitable) {
                    painter->fillRect(cellRect, QColor(0xff, 0, 0, 70));
                }
                if (cell.m_visitable) {
                    painter->fillRect(cellRect, QColor(0xff, 0xff, 0, 50));
                }

                painter->setTransform(oldTransform);
            }
        }
    }

    // debug paint
    for (const auto& [y, row] : spriteMap->m_planes[m_depth].m_merged.m_rows) {
        for (const auto& [x, cell] : row.m_cells) {
            for (const auto& debugPiece : cell.m_debug) {
                auto oldTransform = painter->transform();
                painter->translate(x * tileSize, y * tileSize);

                if (debugPiece.m_shape) {
                    painter->setBrush(debugPiece.m_shapeColor);
                    painter->setPen(QPen(debugPiece.m_penColor, 3));

                    const int radius    = std::clamp(debugPiece.m_shapeRadius, 1, 4);
                    const int shapeSize = tileSize * radius / 4;
                    QRect     shapeRect(tileSize / 2 - shapeSize / 2, tileSize / 2 - shapeSize / 2, shapeSize, shapeSize);
                    if (debugPiece.m_shape == 1)
                        painter->drawEllipse(shapeRect);
                    else if (debugPiece.m_shape == 2)
                        painter->drawRect(shapeRect);
                }

                if (!debugPiece.m_text.isEmpty()) {
                    QRect cellRect(0, 0, tileSize, tileSize);
                    QFont font = painter->font();
                    font.setPixelSize(8);
                    painter->setFont(font);
                    painter->setBrush(debugPiece.m_textColor);
                    painter->setPen(debugPiece.m_textColor);
                    painter->drawText(cellRect, Qt::AlignCenter, debugPiece.m_text);
                }

                painter->setTransform(oldTransform);
            }
        }
    }

    // top-level UI paint
    if (m_settings->m_grid && m_settings->m_gridOnTop) {
        drawGrid(QColor(0, 0, 0), m_settings->m_gridOpacity);
    }
}

void SpriteMapPainter::paintMinimap(QPainter*        painter,
                                    const SpriteMap* spriteMap,
                                    QSize            minimapSize,
                                    QRectF           visible) const
{
    if (spriteMap->m_planes.empty())
        return;
    QPixmap pixmap(spriteMap->m_width, spriteMap->m_height);
    auto    img = pixmap.toImage();

    for (const auto& [y, row] : spriteMap->m_planes[m_depth].m_merged.m_rows) {
        for (const auto& [x, cell] : row.m_cells) {
            if (cell.m_colorUnblocked.isValid() && cell.m_colorBlocked.isValid())
                img.setPixelColor(x, y, cell.m_player ? cell.m_colorPlayer : (cell.m_blocked && !cell.m_visitable ? cell.m_colorBlocked : cell.m_colorUnblocked));
        }
    }

    painter->drawPixmap(QRect(QPoint(0, 0), minimapSize), QPixmap::fromImage(img));

    if (visible.isNull())
        return;
    QSizeF  newSize(visible.width() * minimapSize.width(), visible.height() * minimapSize.height());
    QPointF newTopLeft(visible.left() * minimapSize.width(), visible.top() * minimapSize.height());
    QRectF  visibleScaled(newTopLeft, newSize);

    painter->setPen(QColor(Qt::white));
    painter->drawRect(visibleScaled);
}

}
