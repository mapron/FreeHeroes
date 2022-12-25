/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "TickTimer.hpp"

#include <QElapsedTimer>
#include <QTimer>

namespace FreeHeroes::Gui {

TickTimer::TickTimer(QObject* parent)
    : QObject(parent)
{
    QElapsedTimer elapsed;
    qint64        last = 0;

    QTimer* animationTimer = new QTimer(this);

    connect(animationTimer, &QTimer::timeout, this, [this, elapsed, last]() mutable {
        if (!elapsed.isValid()) {
            elapsed.start();
            last = 0;
        }
        const auto elapsedMs = elapsed.elapsed();
        emit       tick(elapsedMs - last);
        last = elapsedMs;
    });
    animationTimer->start(15); // we really don't care for the real timer resolution. if we get at least 50 fps, that's perfect.
}

}
