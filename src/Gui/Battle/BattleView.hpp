/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsView>

namespace FreeHeroes::Gui {

class BattleView : public QGraphicsView {
    Q_OBJECT
public:
    explicit BattleView(QWidget* parent = 0);

protected:
    void wheelEvent(QWheelEvent* event);

    void keyPressEvent(QKeyEvent* event);
};

}
