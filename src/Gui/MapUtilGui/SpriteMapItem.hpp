/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsItem>

namespace FreeHeroes {

namespace Gui {
class LibraryModelsProvider;
}

struct SpriteMap;
struct SpritePaintSettings;
class SpriteMapItem : public QGraphicsItem {
public:
    SpriteMapItem(const SpriteMap*           spriteMap,
                  const SpritePaintSettings* spritePaintSettings,
                  int                        depth,
                  uint32_t                   animationFrameDurationMs)
        : m_spriteMap(spriteMap)
        , m_spritePaintSettings(spritePaintSettings)
        , m_currentDepth(depth)
        , m_animationFrameDurationMs(animationFrameDurationMs)
    {
    }

    void tick(uint32_t msecElapsed);

    // QGraphicsItem interface
public:
    QRectF boundingRect() const override;
    void   paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    const SpriteMap* const           m_spriteMap;
    const SpritePaintSettings* const m_spritePaintSettings;
    const int                        m_currentDepth;
    const uint32_t                   m_animationFrameDurationMs;

    uint32_t m_animationTick = 0;

    uint32_t m_animationFrameOffsetTerrain = 0;
    uint32_t m_animationFrameOffsetObjects = 0;
};

}
