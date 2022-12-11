/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "LauncherWindow.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;

    QApplication app(argc, argv);

    LauncherWindow::Mode mode = LauncherWindow::Mode::General;
    if (app.arguments().contains("--shortcut"))
        mode = LauncherWindow::Mode::Shortcut;
    if (app.arguments().contains("--post-install"))
        mode = LauncherWindow::Mode::Shortcut;

    LauncherWindow w(mode);
    w.show();

    return app.exec();
}
