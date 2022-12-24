/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGraphicsLibrary.hpp"
#include "ICursorLibrary.hpp"

#include "GuiResourceExport.hpp"

#include <QMap>
#include <QCursor>

namespace FreeHeroes::Gui {

class GUIRESOURCE_EXPORT CursorLibrary : public ICursorLibrary {
    QMap<BattleDirection, QCursor> attackCursors;
    QMap<Type, QCursor>            otherCursors;
    QList<QCursor>                 cast;

public:
    CursorLibrary(const IGraphicsLibrary* graphicsLibrary);

    QCursor        getAttackCursor(BattleDirection direction) const override { return attackCursors.value(direction); }
    QCursor        getOther(Type type) const override { return otherCursors.value(type); }
    QList<QCursor> getCast() const override { return cast; }
};

}
