/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "SetupWindow.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;

    QApplication app(argc, argv);

    SetupWindow w;
    w.show();

    return app.exec();
}
