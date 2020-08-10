/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QPauseAnimation>

#include <functional>

namespace FreeHeroes::Gui {

class OneTimeCallAnimation : public QPauseAnimation {
    bool m_called = false;
    std::function<void(void)> m_callback;
public:
    OneTimeCallAnimation(std::function<void(void)> callback, int delay, QObject * parent)
        : QPauseAnimation(delay, parent), m_callback(callback) {}

    void updateCurrentTime(int) override {
        if (m_called)
            return;
        m_called = true;
        m_callback();
    }
};

}
