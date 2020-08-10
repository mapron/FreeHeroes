/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "Application.hpp"

#include "ConverterDialog.hpp"


int main(int argc, char * argv[])
{
    using namespace FreeHeroes;
    using namespace Conversion;

    Gui::Application fhApp;
    QApplication app(argc, argv);

    fhApp.load("LegacyConverter", {Gui::Application::Option::QtTranslations});

    ConverterDialog dlg;
    dlg.show();

    return app.exec();
}

