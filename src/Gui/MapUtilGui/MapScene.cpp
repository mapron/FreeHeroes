/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapScene.hpp"

#include "SpriteMap.hpp"

#include <QGraphicsSceneMouseEvent>

namespace FreeHeroes {

void MapScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mouseMoveEvent(event);
    auto pos = event->scenePos() / m_settings->m_tileSize;
    emit cellHover(static_cast<int>(pos.x()), static_cast<int>(pos.y()), m_currentDepth);
}

void MapScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mousePressEvent(event);
    auto pos = event->scenePos() / m_settings->m_tileSize;
    emit cellPress(static_cast<int>(pos.x()), static_cast<int>(pos.y()), m_currentDepth);
}

void MapScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    auto pos = event->scenePos() / m_settings->m_tileSize;
    emit cellRelease(static_cast<int>(pos.x()), static_cast<int>(pos.y()), m_currentDepth);
}

}
