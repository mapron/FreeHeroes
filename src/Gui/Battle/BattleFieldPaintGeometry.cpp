/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleFieldPaintGeometry.hpp"

#include "BattleField.hpp"

#include "Logger.hpp"

#include <QtMath>

namespace FreeHeroes::Gui {
using namespace Core;

const BattleFieldPaintGeometry defaultGeometry {
    22,
    16,
    26,
};

QPolygonF BattleFieldPaintGeometry::getHexPolygon(const QPointF centerOffset, qreal scale)  const noexcept {
    const QVector<QPointF> pointsHex {
        {-hexW1 * scale, -hexH1 * scale},
        {0             , -hexH2 * scale},
        { hexW1 * scale, -hexH1 * scale},
        { hexW1 * scale,  hexH1 * scale},
        {0             ,  hexH2 * scale},
        {-hexW1 * scale,  hexH1 * scale},
    };
    const QPolygonF poly { pointsHex };
    return poly.translated(centerOffset);
}

QPointF BattleFieldPaintGeometry::hexCenterFromCoord(const BattlePosition pos)  const noexcept {

    QPointF result{0, 0};
    if (pos.y % 2 == 0) {
        result -= QPointF{-hexW1, 0.};
    }
    result += QPointF{ hexW1 * 2 * pos.x, (hexH1 + hexH2) * pos.y};
    return result;
}

QPointF BattleFieldPaintGeometry::hexCenterFromExtCoord(const BattlePositionExtended pos) const noexcept
{
    return (hexCenterFromCoord(pos.leftPos()) + hexCenterFromCoord(pos.rightPos())) / 2;
}

QPolygonF BattleFieldPaintGeometry::getHexPolygon(const BattlePosition pos, qreal scale)  const noexcept {
    return getHexPolygon(hexCenterFromCoord(pos), scale);
}

BattlePosition BattleFieldPaintGeometry::coordFromPos(const QPointF pos)  const noexcept {
    const int aproxX = pos.x() / (hexW1 * 2);
    const int aproxY = pos.y() / (hexH1 + hexH2);

    auto poly1 = getHexPolygon(BattlePosition{aproxX, aproxY});
    if (poly1.containsPoint(pos, Qt::OddEvenFill)) {
        return {aproxX, aproxY};
    }
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (getHexPolygon(BattlePosition{aproxX + dx, aproxY + dy}).containsPoint(pos, Qt::OddEvenFill)) {
                return {aproxX + dx, aproxY + dy};
            }
        }
    }

    return {-1, -1};
}

/*



 */


BattleAttackDirection BattleFieldPaintGeometry::attackDirectionFromRelativePoint(const QPointF posInCell, bool allowTopBottom)  const noexcept{
    auto directonInRad = qAtan2(posInCell.x(), posInCell.y());
    int rotationDegree = static_cast<int>(directonInRad * 180 / M_PI);
    rotationDegree = -rotationDegree;

    while (rotationDegree < 0)
        rotationDegree += 360;
    //Logger(Logger::Info) << "pos:" << posInCell.x() << "," << posInCell.y() << " -> rad=" << directonInRad << ", deg=" << rotationDegree;
    if (allowTopBottom) {
        if (rotationDegree >= 165 && rotationDegree < 195)
            return BattleAttackDirection::B;
        if ((rotationDegree >= 0 && rotationDegree < 15) || (rotationDegree >= 345 && rotationDegree < 360))
            return BattleAttackDirection::T;
    }
    rotationDegree /= 60;

    static const std::vector<BattleAttackDirection> dirs {BattleAttackDirection::TR, BattleAttackDirection::R, BattleAttackDirection::BR,
                BattleAttackDirection::BL, BattleAttackDirection::L, BattleAttackDirection::TL};
    const BattleAttackDirection direction = dirs[rotationDegree];
    return direction;
}

}
