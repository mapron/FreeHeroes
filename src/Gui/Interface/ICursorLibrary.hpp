/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QCursor>

namespace FreeHeroes::Gui {

class ICursorLibrary {
public:
    virtual ~ICursorLibrary() = default;

    enum class Type { Stop, Walk, Fly, RangeAttack, RangeAttackBroken, HeroView, Question, PlainArrow };
    enum class BattleDirection { None = -1, TR = 0, R, BR, BL, L, TL, Top, Bottom };

    virtual QCursor getAttackCursor(BattleDirection direction) const = 0;
    virtual QCursor getOther(Type type) const = 0;
    virtual QList<QCursor> getCast() const = 0;

};

}
