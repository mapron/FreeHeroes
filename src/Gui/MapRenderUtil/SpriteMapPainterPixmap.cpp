/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapPainterPixmap.hpp"

#include "SpriteMap.hpp"
#include "Painter.hpp"

namespace FreeHeroes {

struct SpriteMapPainterPixmap::Impl {
};

SpriteMapPainterPixmap::SpriteMapPainterPixmap(const SpritePaintSettings* settings, int depth)
    : m_settings(settings)
    , m_depth(depth)
    , m_impl(std::make_unique<Impl>())
{}

SpriteMapPainterPixmap::~SpriteMapPainterPixmap()
{
}

void SpriteMapPainterPixmap::paint(Painter*         painter,
                                   const SpriteMap* spriteMap,
                                   uint32_t         animationFrameOffsetTerrain,
                                   uint32_t         animationFrameOffsetObjects) const
{
    const int tileSize = m_settings->m_tileSize;

    auto drawHeroFlag = [painter](const PixmapColor& color) {
        // @todo:
        /*
        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawRect(10, 9, 10, 7);
        painter->drawPolygon(QPolygonF(QVector<QPointF>{ { 1, 10 }, { 10, 10 }, { 10, 14 } }));
        painter->drawPolygon(QPolygonF(QVector<QPointF>{ { 1, 16 }, { 10, 16 }, { 10, 12 } }));
        painter->drawRect(4, 16, 4, 1);

        painter->setBrush(color.darker(120));
        painter->drawRect(16, 9, 3, 7);*/
        //painter->draw
    };

    auto drawCell = [painter, tileSize, animationFrameOffsetTerrain, animationFrameOffsetObjects, &drawHeroFlag, this](const SpriteMap::Cell& cell, int x, int y) {
        for (const auto& item : cell.m_items) {
            if (item.m_isOverlayItem && !m_settings->m_overlay)
                continue;

            if (item.m_flagColor.isValid()) {
                auto oldTransform = painter->getTransform();
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
            if (isFilteredOut) {
                continue;
            }

            const auto psrHash = item.m_x * 7U + item.m_y * 13U;

            const size_t frameIndex = psrHash + (item.m_layer == SpriteMap::Layer::Terrain ? animationFrameOffsetTerrain : animationFrameOffsetObjects);
            const auto   frame      = seq->m_frames[frameIndex % seq->m_frames.size()];

            const auto boundingSize = seq->m_boundarySize;

            auto oldTransform = painter->getTransform();
            painter->translate(x * tileSize, y * tileSize);

            if (item.m_shiftHalfTile) {
                painter->translate(0, tileSize / 2);
            }

            if (boundingSize.m_width > tileSize || boundingSize.m_height > tileSize) {
                painter->translate(-boundingSize.m_width + tileSize, -boundingSize.m_height + tileSize);
            }

            Pixmap pix = frame.m_frame;
            if (item.m_keyColor.isValid()) {
                for (auto& p : pix.m_pixels) {
                    if (p.m_color.m_a == 1) {
                        p.m_color = item.m_keyColor;
                    }
                }
            }
            painter->drawPixmap(frame.m_paddingLeftTop, pix, item.m_flipHor, item.m_flipVert);

            painter->setTransform(oldTransform);
        }
    };

    for (const auto& [priority, grid] : spriteMap->m_planes[m_depth].m_grids) {
        for (const auto& [rowIndex, rowSlice] : grid.m_rowsSlices) {
            for (const auto& [rowPriority, row] : rowSlice.m_rows) {
                for (const auto& [colIndex, cell] : row.m_cells) {
                    drawCell(cell, colIndex, rowIndex);
                }
            }
        }
    }
}

}
