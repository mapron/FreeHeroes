/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QSignalBlocker>

#include <vector>

class QSignalBlocker;

namespace FreeHeroes::Gui {

class SignalBlocker
{
    std::vector<QSignalBlocker> blockers;
public:
    SignalBlocker (const std::vector<QObject*> & objects) {
        for (QObject* o : objects)
            blockers.emplace_back(o);
    }
};

}
