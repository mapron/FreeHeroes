/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleStackSpriteItem.hpp"

#include <QPainter>

namespace FreeHeroes::Gui {

namespace {

const int pulseDurationMs = 1500;

QVector<QPointF> getOpaqueRegionBorder(const QImage & img, QPointF offset) {
    auto getImageOpaque = [&img](int w, int h) {
        return (w >= 0 && h >= 0 && w < img.width() && h < img.height()) ? img.pixelColor(w , h ).alpha() == 255 : false;
    };
    QVector<QPointF> points;
    for (int h = -1; h < img.height() + 1; ++h) {
        for (int w = -1; w < img.width() + 1; ++w) {
            if (getImageOpaque(w, h))
                continue;
            const bool hasOpaqueNeighbour =
                       getImageOpaque(w - 1, h - 1)
                    || getImageOpaque(w + 0, h - 1)
                    || getImageOpaque(w + 1, h - 1)
                    || getImageOpaque(w - 1, h + 0)
                    || getImageOpaque(w + 1, h + 0)
                    || getImageOpaque(w - 1, h + 1)
                    || getImageOpaque(w + 0, h + 1)
                    || getImageOpaque(w + 1, h + 1);
            if (hasOpaqueNeighbour)
                points << QPointF(w, h) + offset;
        }
    }
    return points;
}

int blendTwo(int start, int end, double r) {
    return std::clamp(static_cast<int>(start * r + end * (1 - r)), 0, 255);
}

QColor blendTwo(QColor start, QColor end, double r) {

    return QColor(blendTwo(start.red()  , end.red(), r),
                  blendTwo(start.green(), end.green(), r),
                  blendTwo(start.blue() , end.blue(), r),
                  blendTwo(start.alpha(), end.alpha(), r));
}

QColor linearPulse(QColor base, int darkenValue, int ms, int msMax) {
    const QColor end(std::max(base.red() - darkenValue, 0),
               std::max(base.green() - darkenValue, 0),
               std::max(base.blue() - darkenValue, 0),
               base.alpha());
    const auto frac = double(ms % msMax);
    const auto half = double(msMax / 2.);
    const double r = frac <= half ? frac / half : (msMax - frac) / half;
    QColor res = blendTwo(base, end, r);
    return res;
}

}

BattleStackSpriteItem::BattleStackSpriteItem(const SpritePtr& frames, qreal hexWidth, QGraphicsItem* parent)
    : SpriteItemObj(parent)
    , m_hexWidth(hexWidth)
{
    setSprite(frames);
}

void BattleStackSpriteItem::setStartDirectionRight(bool right)
{
    m_startDirectionRight = right;
    setTempDirectionRight(right);
}

void BattleStackSpriteItem::setTempDirectionRight(bool right)
{
    setMirrorHor(!right);
}

void BattleStackSpriteItem::setHighlight(BattleStackSpriteItem::Highlight highlight, bool state)
{
    if (state)
        m_highlights[highlight] = state;
    else
        m_highlights.remove(highlight);
    update();
}

void BattleStackSpriteItem::setIsLarge(bool isLarge)
{
    m_isLarge = isLarge;
    update();
}

void BattleStackSpriteItem::setCounter(int count)
{
    m_count = count;
    update();
}

void BattleStackSpriteItem::setCounterExtra(int count)
{
    m_countExtra = count;
    update();
}

void BattleStackSpriteItem::setCounterVisible(bool visible)
{
    m_counterVisible = visible;
    update();
}

void BattleStackSpriteItem::setCounterMode(BattleStackSpriteItem::CounterMode counterMode)
{
    m_counterMode = counterMode;
    update();
}

void BattleStackSpriteItem::setCounterCompact(bool compact)
{
    m_counterCompact = compact;
    update();
}


void BattleStackSpriteItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    // debug cross
//    {
//        painter->setPen(Qt::SolidLine);
//        painter->drawLine(m_boundingOrigin, QPointF(m_boundingSize.width(), m_boundingSize.height()) + m_boundingOrigin);
//        painter->drawLine(QPointF(m_boundingSize.width(), 0) + m_boundingOrigin, QPointF(0, m_boundingSize.height()) + m_boundingOrigin);
//    }

    auto t = painter->transform();
    auto t2 = t;
    //const qreal scaleDirect = m_currentSequence->params.scaleFactorPercent / 100.;
    const qreal scaleInverse = 100. / m_currentSequence->params.scaleFactorPercent;
    t2.scale(scaleInverse, scaleInverse);
    if (!getTmpDirectionRight()) {

        t2.scale(-1, 1);

    }
    painter->setTransform(t2);
    auto offset = m_boundingOrigin + m_pixmapPadding;
    painter->drawPixmap(offset, m_pixmap);

    auto mainHightLight = getMainHighlight();

    if (mainHightLight != Highlight::No) {
        auto img = m_pixmap.toImage();
        const auto points = getOpaqueRegionBorder(img, offset); // @todo: cache maybe

        const auto pulseColor = mainHightLight == Highlight::Selected ? QColor(255, 255, 0, 128) : QColor(0, 255, 255, 192);
        painter->setPen(linearPulse(pulseColor, 100, m_frameMsTick, pulseDurationMs));
        painter->drawPoints(points.data(), points.size());
    }

    painter->setTransform(t);


    if (m_counterVisible) {
        const qreal boxWidth = 30;
        const qreal boxHeight = 11;
        painter->setPen(QPen(QColor(255, 231, 132), 1));
        static const QMap<CounterMode, QPair<QColor, QColor>> baseColors {
            {CounterMode::Normal, QPair{QColor(69 , 23 , 115) , QColor(128, 42 , 214)} },
            {CounterMode::Buff  , QPair{QColor(21 , 107, 21 ) , QColor(44 , 222, 44 )} },
            {CounterMode::Debuff, QPair{QColor(107, 21 , 21 ) , QColor(222, 44 , 44 )} },
            {CounterMode::Mixed , QPair{QColor(107, 107, 21 ) , QColor(222, 222, 44 )} },
        };
        static const auto stopsGray = QPair{QColor(60 , 60 , 60) , QColor(110, 110 , 110)};


        const QPointF cellGroundCenter = m_boundingOrigin
                                         + QPointF(m_boundingSize.width(), m_boundingSize.height()) / 2
                                         + QPointF{0, fromSpriteCenterToHexCenter}
                                         ;
        QPointF offset = cellGroundCenter;
        const int horizontalShift =  (m_counterCompact)
                                  ?  (m_isLarge ? +3 : -boxWidth / 2)
                                  :  (m_isLarge ? m_hexWidth : m_hexWidth / 2 + 1);


        if (!m_startDirectionRight) {
            offset -= QPointF{boxWidth, m_counterCompact ? 0 : (boxHeight + 2)};
            offset.rx() -= horizontalShift;
        } else {
            offset.rx() += horizontalShift;
        }
        if (m_counterCompact) {
            offset.ry() +=  boxHeight - 2;
        }

        QRectF rect;
        rect.setTopLeft(offset);
        rect.setSize({boxWidth - 1, boxHeight - 1}); // bottom and right pen pixeSize not included.
        {
            const auto stops = baseColors[m_counterMode];
            QRadialGradient grad(offset + QPointF{boxWidth/2, boxHeight-3}, boxWidth * 0.7);
            grad.setColorAt(1, stops.first);
            grad.setColorAt(0, stops.second);
            painter->setBrush(QBrush(grad));
            painter->drawRect(rect);
        }
        auto oldPen = painter->pen();
        {
            painter->setPen(Qt::white);
            QFont f = painter->font();
            f.setPixelSize(10);
            f.setStyleStrategy(QFont::StyleStrategy(f.styleStrategy() | QFont::NoSubpixelAntialias));
            painter->setFont(f);
            painter->drawText(rect, QString::number(m_count), QTextOption(Qt::AlignCenter | Qt::AlignHCenter));
        }
        painter->setPen(oldPen);
        if (m_countExtra != 0) {
            rect.translate(0, rect.height() );
            {

                QRadialGradient grad(offset + QPointF{boxWidth/2, boxHeight-3}, boxWidth * 0.7);
                grad.setColorAt(1, stopsGray.first);
                grad.setColorAt(0, stopsGray.second);
                painter->setBrush(QBrush(grad));
                painter->drawRect(rect);
            }

            {
                painter->setPen(Qt::white);
                QFont f = painter->font();
                f.setPixelSize(10);
                f.setStyleStrategy(QFont::StyleStrategy(f.styleStrategy() | QFont::NoSubpixelAntialias));
                painter->setFont(f);
                painter->drawText(rect, QString::number(m_countExtra), QTextOption(Qt::AlignCenter | Qt::AlignHCenter));
            }
        }
    }

}

BattleStackSpriteItem::Highlight BattleStackSpriteItem::getMainHighlight() const
{
    if (m_highlights.value(Highlight::Hovered))
        return Highlight::Hovered;
    if (m_highlights.value(Highlight::Selected))
        return Highlight::Selected;
    if (m_highlights.value(Highlight::Possible))
        return Highlight::Possible;
    return Highlight::No;
}

}
