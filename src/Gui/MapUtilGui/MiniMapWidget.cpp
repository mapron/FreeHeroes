/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MiniMapWidget.hpp"

#include "SpriteMapPainter.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>
#include <QDebug>

namespace FreeHeroes {

MiniMapWidget::MiniMapWidget(const SpritePaintSettings* settings, const SpriteMap* spriteMap, QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_spriteMap(spriteMap)
{
    setMinimumSize(150, 150);
}

MiniMapWidget::~MiniMapWidget()
{
}

void MiniMapWidget::setDepth(int depth)
{
    if (depth >= m_spriteMap->m_depth)
        return;
    m_depth = depth;
    update();
}

void MiniMapWidget::updateVisible(QRectF visible)
{
    m_visible = visible;
    update();
}

void MiniMapWidget::paintEvent(QPaintEvent* e)
{
    QPainter     p(this);
    QStyleOption opt;
    opt.initFrom(this);

    auto [offset, minimapSize] = getMiniMapRect(opt.rect);

    p.translate(offset);
    SpriteMapPainter spainter(m_settings, m_depth);
    spainter.paintMinimap(&p, m_spriteMap, minimapSize, m_visible);
}

void MiniMapWidget::mousePressEvent(QMouseEvent* event)
{
    m_dragEnabled = true;
    QWidget::mousePressEvent(event);
}

void MiniMapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragEnabled = false;
    QWidget::mouseReleaseEvent(event);
}

void MiniMapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragEnabled) {
        auto [offset, minimapSize] = getMiniMapRect(QRect(QPoint(), size()));

        QPoint pos = event->pos();
        pos -= offset;
        QRect boundRect(QPoint(), minimapSize);
        if (boundRect.contains(pos)) {
            QPointF relPos = pos;
            relPos.rx() /= minimapSize.width();
            relPos.ry() /= minimapSize.height();
            emit minimapDrag(relPos);
        }
    }
    QWidget::mouseMoveEvent(event);
}

QPair<QPoint, QSize> MiniMapWidget::getMiniMapRect(QRect widgetRect) const
{
    QPoint offset(0, 0);

    const auto  minDimension = std::min(widgetRect.size().width(), widgetRect.size().height());
    const QSize minimapSize(minDimension, minDimension);

    if (widgetRect.size().width() > minDimension)
        offset.rx() += (widgetRect.size().width() - minDimension) / 2;
    if (widgetRect.size().height() > minDimension)
        offset.ry() += (widgetRect.size().height() - minDimension) / 2;
    return { offset, minimapSize };
}

}
