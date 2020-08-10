/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleView.hpp"

#include <QWheelEvent>
#include <QKeyEvent>

namespace FreeHeroes::Gui {

BattleView::BattleView(QWidget *parent) :
    QGraphicsView(parent)
{
     setDragMode(QGraphicsView::NoDrag);
     setInteractive(true);
}

void BattleView::wheelEvent(QWheelEvent *)
{
}

void BattleView::keyPressEvent(QKeyEvent *event)
{
    QGraphicsView::keyPressEvent(event);
}

}
