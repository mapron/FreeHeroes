/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include <iostream>

#include "CoreApplication.hpp"
#include "Application.hpp"

#include "MapToolWindow.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;
    using namespace Mernel;

    Core::CoreApplication fhCoreApp;
    fhCoreApp.setLoadUserMods(true);

    Gui::Application fhApp(&fhCoreApp,
                           { Gui::Application::Option::QtTranslations,
                             Gui::Application::Option::GraphicsLibrary,
                             Gui::Application::Option::MusicBox,
                             Gui::Application::Option::CursorLibrary,
                             Gui::Application::Option::LibraryModels,
                             Gui::Application::Option::AppStyle },
                           "MapToolUI");
    QApplication     app(argc, argv);

    fhApp.load();

    MapToolWindow dlg(
        fhCoreApp.getDatabaseContainer(),
        fhCoreApp.getRandomGeneratorFactory(),
        fhApp.getGraphicsLibrary(),
        fhApp.getModelsProvider(),
        fhApp.getAppSettings());

    dlg.show();

    return app.exec();
}
