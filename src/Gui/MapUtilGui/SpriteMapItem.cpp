/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SpriteMapItem.hpp"
#include "SpriteMap.hpp"

#include "SpriteMapPainter.hpp"

#include <QDebug>

namespace FreeHeroes {

SpriteMapItem::SpriteMapItem(const SpriteMap* spriteMap, const SpritePaintSettings* spritePaintSettings, int depth, uint32_t animationFrameDurationMs)
    : m_spriteMap(spriteMap)
    , m_spritePaintSettings(spritePaintSettings)
    , m_currentDepth(depth)
    , m_animationFrameDurationMs(animationFrameDurationMs)
    , m_painter(std::make_unique<SpriteMapPainter>(m_spritePaintSettings, m_currentDepth))
{
}

SpriteMapItem::~SpriteMapItem()
{
}

void SpriteMapItem::tick(uint32_t msecElapsed)
{
    m_animationTick += msecElapsed;

    uint32_t frameTick = m_animationTick / m_animationFrameDurationMs;
    m_animationTick -= frameTick * m_animationFrameDurationMs;

    if (!frameTick)
        return;
    if (!m_spritePaintSettings->m_animateTerrain && !m_spritePaintSettings->m_animateObjects)
        return;

    if (m_spritePaintSettings->m_animateTerrain)
        m_animationFrameOffsetTerrain += frameTick;
    if (m_spritePaintSettings->m_animateObjects)
        m_animationFrameOffsetObjects += frameTick;

    update();
}

QRectF SpriteMapItem::boundingRect() const
{
    return QRectF{ QPointF{ 0., 0. }, QSizeF(m_spriteMap->m_width, m_spriteMap->m_height) * m_spritePaintSettings->m_tileSize };
}

void SpriteMapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    m_painter->paint(painter, m_spriteMap, m_animationFrameOffsetTerrain, m_animationFrameOffsetObjects);
}

}
