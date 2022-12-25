/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QObject>

#include "GuiWidgetsExport.hpp"

namespace FreeHeroes::Gui {

class GUIWIDGETS_EXPORT TickTimer : public QObject {
    Q_OBJECT
public:
    TickTimer(QObject* parent);

signals:
    void tick(uint32_t msElapsed);
};

}
