/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattleFieldPosition.hpp"

#include <QPolygonF>
#include <QPointF>


namespace FreeHeroes::Gui {

struct BattleFieldPaintGeometry {
    const qreal hexW1{}; // Hex horizontal radius. It is inner circle radius of hexagon.
    const qreal hexH1{}; // Height of vertical side.
    const qreal hexH2{}; // Vertical radius of cell (outer circle radius).

    QPolygonF getHexPolygon(const QPointF centerOffset, qreal scale = 1.0) const noexcept;

    QPointF hexCenterFromCoord(const Core::BattlePosition pos) const noexcept;
    QPointF hexCenterFromExtCoord(const Core::BattlePositionExtended pos) const noexcept;

    QPolygonF getHexPolygon(const Core::BattlePosition pos, qreal scale = 1.0)  const noexcept;

    Core::BattlePosition coordFromPos(const QPointF pos)  const noexcept;

    Core::BattleAttackDirection attackDirectionFromRelativePoint(const QPointF posInCell, bool allowTopBottom)  const noexcept;
};

extern const BattleFieldPaintGeometry defaultGeometry;

}
