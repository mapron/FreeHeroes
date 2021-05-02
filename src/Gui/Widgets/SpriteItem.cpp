/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SpriteItem.hpp"

#include <QPainter>
#include <QBitmap>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>

namespace FreeHeroes::Gui {

SpriteItem::SpriteItem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
}

void SpriteItem::setSprite(const SpritePtr& frames)
{
    m_sprite = frames;
    Q_ASSERT(m_sprite);
}

const SpritePtr& SpriteItem::getSprite() const
{
    return m_sprite;
}

void SpriteItem::setAnimGroup(const AnimGroupSettings& settings)
{
    // @todo: debug: at the moment we should not move away from death state.
    if (m_animSettings.group == 5 && settings.group != 5)
        Q_ASSERT(false);

    m_animSettings = settings;
    m_inSporadic   = false;

    setAnimGroupInternal();
}

void SpriteItem::setAnimGroupSporadic(const SpriteItem::AnimGroupSettings& settings)
{
    m_sporadicSettings = settings;
}

void SpriteItem::setMirrorHor(bool state)
{
    m_mirrorHor = state;
    update();
}

void SpriteItem::setMirrorVert(bool state)
{
    m_mirrorVert = state;
    update();
}

void SpriteItem::triggerSporadic()
{
    if (!m_sporadicSettings.isValid() || m_inSporadic)
        return;

    m_inSporadic = true;
    setAnimGroupInternal();
}

void SpriteItem::tick(int msecElapsed)
{
    m_frameMsTick += msecElapsed;
    //m_sporadic.tick(msecElapsed);

    //if (m_sporadic.startSporadic(this))
    //    return;
    const AnimGroupSettings& settings = m_inSporadic ? m_sporadicSettings : m_animSettings;

    const auto size = m_currentSequence->frames.size();

    const auto prevIndex = m_currentFrameIndex;
    m_currentFrameIndex  = m_frameMsTick * size / settings.durationMs;

    if (settings.repeatOnFrame >= 0 && m_currentFrameIndex >= settings.repeatOnFrame) {
        assert(!settings.loopOver);
        // repeat on 2 count 2, endRepeatFrame 4
        // origIndex  0 1 2 3 4 5 6 7 8
        // newIndex   0 1 2 2 2 3 4 5 6
        const int endRepeatFrame = settings.repeatOnFrame + settings.repeatOnFrameCount;
        m_currentFrameIndex      = m_currentFrameIndex > endRepeatFrame
                                       ? m_currentFrameIndex - settings.repeatOnFrameCount
                                       : settings.repeatOnFrame;
    }

    if (m_currentFrameIndex >= size) {
        if (m_inSporadic) {
            m_inSporadic = false;
            setAnimGroupInternal();
            return;
        }

        m_currentFrameIndex = settings.loopOver ? (m_currentFrameIndex % size) : (size - 1);
    }

    if (prevIndex == m_currentFrameIndex)
        return;

    displayCurrentPixmap();
}

void SpriteItem::displayCurrentPixmap()
{
    const AnimGroupSettings& settings    = m_inSporadic ? m_sporadicSettings : m_animSettings;
    const auto               size        = m_currentSequence->frames.size();
    const auto               index       = settings.reverse ? size - m_currentFrameIndex - 1 : m_currentFrameIndex;
    auto&                    spriteFrame = m_currentSequence->frames[index];
    m_pixmap                             = spriteFrame.frame;

    m_boundingOrigin = QPointF(0, 0);
    if (m_drawOriginH == DrawOriginH::Right)
        m_boundingOrigin.rx() -= m_boundingSize.width();
    if (m_drawOriginV == DrawOriginV::Bottom)
        m_boundingOrigin.ry() -= m_boundingSize.height();

    if (m_drawOriginH == DrawOriginH::Center)
        m_boundingOrigin.rx() -= m_boundingSize.width() / 2;
    if (m_drawOriginV == DrawOriginV::Center)
        m_boundingOrigin.ry() -= m_boundingSize.height() / 2;

    m_pixmapPadding = spriteFrame.paddingLeftTop;
    update();
}

QRectF SpriteItem::boundingRect() const
{
    if (m_pixmap.isNull())
        return QRectF();

    //const qreal pw = (this->flags() & ItemIsSelectable) ? 1.0 : 0.0;
    //  "/ m_pixmap.devicePixelRatio()" ???
    return QRectF(m_boundingOrigin, m_boundingSize);
}

QPainterPath SpriteItem::shape() const
{
    QPainterPath shape;
    shape.addRect(boundingRect());
    return shape;
}

bool SpriteItem::contains(const QPointF& point) const
{
    return QGraphicsItem::contains(point);
}

void SpriteItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    //   debug cross
    //    {
    //        painter->setPen(Qt::SolidLine);
    //        painter->drawLine(m_boundingOrigin, QPointF(m_boundingSize.width(), m_boundingSize.height()) + m_boundingOrigin);
    //        painter->drawLine(QPointF(m_boundingSize.width(), 0) + m_boundingOrigin, QPointF(0, m_boundingSize.height()) + m_boundingOrigin);
    //    }

    auto oldTransform = painter->transform();
    if (m_mirrorHor || m_mirrorVert) {
        auto t2 = oldTransform;
        t2.scale(m_mirrorHor ? -1 : 1, m_mirrorVert ? -1 : 1);
        painter->setTransform(t2);
    }

    const auto offset = m_boundingOrigin + m_pixmapPadding;
    painter->drawPixmap(offset, m_pixmap);

    painter->setTransform(oldTransform);
}

bool SpriteItem::isObscuredBy(const QGraphicsItem* item) const
{
    return QGraphicsItem::isObscuredBy(item);
}

QPainterPath SpriteItem::opaqueArea() const
{
    return shape();
}

int SpriteItem::type() const
{
    return Type;
}

void SpriteItem::setAnimGroupInternal()
{
    const AnimGroupSettings& settings = m_inSporadic ? m_sporadicSettings : m_animSettings;
    Q_ASSERT(settings.durationMs > 0);
    m_currentSequence   = m_sprite->getFramesForGroup(settings.group);
    m_currentFrameIndex = settings.startFrame;
    if (m_currentFrameIndex >= m_currentSequence->frames.size())
        m_currentFrameIndex = 0;
    m_frameMsTick = 0;

    prepareGeometryChange();
    m_boundingSize = m_currentSequence->boundarySize;

    displayCurrentPixmap();
}

bool SporadicHandle::tick(int msecElapsed, bool ignoreCheck)
{
    if (!cfg.enabled)
        return false;

    frameMsTick += msecElapsed;
    if (frameMsTick < currentDelay)
        return false;

    reset();
    if (cfg.check() || ignoreCheck)
        cfg.action();

    return true;
}

void SporadicHandle::reset()
{
    frameMsTick = 0;
    if (cfg.maxExtraDelayMs > 0)
        currentDelay = cfg.delayMs + QRandomGenerator::global()->bounded(cfg.maxExtraDelayMs);
}

void SporadicHandle::runNow()
{
    tick(currentDelay, true);
}

bool SporadicOrchestrator::checkEventLimit(int interval, int maxCount)
{
    auto                            ms      = QDateTime::currentMSecsSinceEpoch();
    auto                            cleanup = ms - interval;
    QMutableListIterator<qlonglong> i(m_timestamps);
    while (i.hasNext()) {
        if (i.next() < cleanup)
            i.remove();
    }
    const bool result = m_timestamps.size() <= maxCount;
    if (result)
        m_timestamps << ms;
    return result;
}

void SpriteItemObj::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    emit hoverIn();
    SpriteItem::hoverEnterEvent(event);
}

void SpriteItemObj::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    SpriteItem::hoverLeaveEvent(event);
    emit hoverOut();
}

}
