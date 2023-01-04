/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "UnitAnimatedPortrait.hpp"

#include "SpriteItem.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QBoxLayout>
#include <QTimer>
#include <QElapsedTimer>

namespace FreeHeroes::Gui {

struct UnitAnimatedPortrait::Impl {
    std::unique_ptr<QGraphicsScene> scene;
    std::unique_ptr<QGraphicsView>  view;
    QTimer                          m_timer;
    QElapsedTimer                   elapsed;
    qint64                          last;
};

UnitAnimatedPortrait::UnitAnimatedPortrait(SpritePtr sprite, SpritePtr spriteBk, int count, bool animated, QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>())
{
    (void) count; // some day? @todo:
    QHBoxLayout* fakeLayout = new QHBoxLayout(this);
    fakeLayout->setContentsMargins(0, 0, 0, 0);
    fakeLayout->setSpacing(0);
    const int width  = 100;
    const int height = 130;
    this->setFixedSize(width + 2, height + 2);
    m_impl->scene = std::make_unique<QGraphicsScene>(0, 0, width, height);
    m_impl->view  = std::make_unique<QGraphicsView>(this);

    fakeLayout->addWidget(m_impl->view.get());
    m_impl->view->setScene(m_impl->scene.get());

    SpriteItem* item = new SpriteItem(); // scene will own this
    m_impl->scene->addItem(item);

    item->setSprite(sprite);

    // 1000 ms for move is ok... need to improve timings load
    item->setAnimGroup(SpriteItem::AnimGroupSettings{ animated ? 0 : 2, 1000 }.setLoopOver(true));

    connect(&m_impl->m_timer, &QTimer::timeout, this, [this, item] {
        if (!m_impl->elapsed.isValid()) {
            m_impl->elapsed.start();
            m_impl->last = 0;
        }
        const auto elapsedMs = m_impl->elapsed.elapsed();
        item->tick(elapsedMs - m_impl->last);
        m_impl->last = elapsedMs;
    });

    item->setPos(width / 2, height / 2);
    item->setZValue(1);

    if (spriteBk) {
        SpriteItem* back = new SpriteItem(); // scene will own this
        back->setDrawOriginH(SpriteItem::DrawOriginH::Left);
        back->setDrawOriginV(SpriteItem::DrawOriginV::Top);
        m_impl->scene->addItem(back);

        back->setSprite(spriteBk);
        back->setAnimGroup(SpriteItem::AnimGroupSettings{ 0, 1 });
    }
}

UnitAnimatedPortrait::~UnitAnimatedPortrait() = default;

void UnitAnimatedPortrait::startTimer()
{
    m_impl->m_timer.start(15);
}

}
