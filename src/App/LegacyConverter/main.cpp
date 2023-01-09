/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "CoreApplication.hpp"
#include "Application.hpp"

#include "ConverterDialog.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;

    Core::CoreApplication fhCoreApp;

    Gui::Application fhApp(&fhCoreApp, { Gui::Application::Option::QtTranslations }, "LegacyConverter");
    QApplication     app(argc, argv);

    fhApp.load();

    ConverterDialog dlg(fhCoreApp.getDatabaseContainer());
    dlg.show();

    return app.exec();
}
