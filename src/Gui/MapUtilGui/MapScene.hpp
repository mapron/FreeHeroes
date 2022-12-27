/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsScene>

namespace FreeHeroes {

struct SpritePaintSettings;
class MapScene : public QGraphicsScene {
    Q_OBJECT
public:
    MapScene(const SpritePaintSettings* const settings, QObject* parent)
        : QGraphicsScene(parent)
        , m_settings(settings)
    {}

    void setCurrentDepth(int depth) { m_currentDepth = depth; }

signals:
    void cellHover(int x, int y, int depth);
    void cellPress(int x, int y, int depth);
    void cellRelease(int x, int y, int depth);

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    const SpritePaintSettings* const m_settings;
    int                              m_currentDepth = 0;
};

}
