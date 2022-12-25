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

void MiniMapWidget::paintEvent(QPaintEvent* e)
{
    QPainter     p(this);
    QStyleOption opt;
    opt.initFrom(this);

    QPoint offset(0, 0);

    const auto  minDimension = std::min(opt.rect.size().width(), opt.rect.size().height());
    const QSize minimapSize(minDimension, minDimension);

    if (opt.rect.size().width() > minDimension)
        offset.rx() += opt.rect.size().width() / 2;
    if (opt.rect.size().height() > minDimension)
        offset.ry() += opt.rect.size().height() / 2;

    p.translate(offset);
    SpriteMapPainter spainter(m_settings, m_depth);
    spainter.paintMinimap(&p, m_spriteMap, minimapSize);
}

}
