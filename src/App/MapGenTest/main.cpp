/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "Application.hpp"

#include "MapGenTestWidget.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;

    Gui::Application fhApp;
    QApplication     app(argc, argv);

    fhApp.load("MapGenTest");

    MapGenTestWidget dlg(fhApp.getGraphicsLibrary(), fhApp.getGameDatabase());
    dlg.show();

    return app.exec();
}
