/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QFrame>

#include <memory>

namespace Ui {
class LauncherWindow;
}

namespace FreeHeroes {

class LauncherWindow : public QFrame {
public:
    enum class Mode
    {
        General,
        PostInstall,
        Shortcut,
    };

    LauncherWindow(Mode mode);
    ~LauncherWindow();

private:
    std::unique_ptr<Ui::LauncherWindow> m_ui;
};

}
