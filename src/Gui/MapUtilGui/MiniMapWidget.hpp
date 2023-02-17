/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include "SpriteMap.hpp"

namespace FreeHeroes {

class MiniMapWidget : public QWidget {
    Q_OBJECT
public:
    MiniMapWidget(const SpritePaintSettings* settings, const SpriteMap* spriteMap, QWidget* parent);
    ~MiniMapWidget();

    void setDepth(int depth);

    void updateVisible(QRectF visible);

signals:
    void minimapDrag(QPointF point);

protected:
    void paintEvent(QPaintEvent* e);

    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

    QPair<QPoint, QSize> getMiniMapRect(QRect widgetRect) const;

private:
    const SpritePaintSettings* const m_settings;
    const SpriteMap* const           m_spriteMap;

    int    m_depth = 0;
    QRectF m_visible;
    bool   m_dragEnabled = false;
};

}
