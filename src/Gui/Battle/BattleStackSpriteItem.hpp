/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SpriteItem.hpp"

namespace FreeHeroes::Gui {

class BattleStackSpriteItem : public SpriteItemObj
{
public:
    enum class Highlight { Selected, Hovered, Possible, No };
    enum class CounterMode { Normal, Buff, Debuff, Mixed };

    static constexpr int fromSpriteCenterToHexCenter = 40;
public:

    BattleStackSpriteItem(const SpritePtr& frames, qreal hexWidth, QGraphicsItem* parent = nullptr);

    void setStartDirectionRight(bool right);
    void setTempDirectionRight(bool right);
    void setHighlight(Highlight highlight, bool state);
    void setIsLarge(bool isLarge);

    void setCounter(int count);
    void setCounterExtra(int count);
    void setCounterVisible(bool visible);
    void setCounterMode(CounterMode counterMode);
    void setCounterCompact(bool compact);

    bool getTmpDirectionRight() const { return !m_mirrorHor; }
    bool getHighlight(Highlight highlight) const { return m_highlights.value(highlight, false); }

protected:

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    Highlight getMainHighlight() const;
protected:
    const qreal m_hexWidth;
    QPointF m_mirrorShift;
    QMap<Highlight, bool> m_highlights;
    bool m_startDirectionRight = true;
    bool m_isLarge = false;

    int m_count = 0;
    int m_countExtra = 0;
    bool m_counterVisible = true;
    CounterMode m_counterMode = CounterMode::Normal;
    bool m_counterCompact = false;
};

using BattleStackSpriteItemPtr = BattleStackSpriteItem*;
using BattleStackSpriteItemConstPtr = const BattleStackSpriteItem*;

}
