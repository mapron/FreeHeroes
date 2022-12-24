/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include <iostream>

#include "CoreApplication.hpp"
#include "CommandLineUtils.hpp"
#include "Application.hpp"

#include "MapEditorWidget.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;

    AbstractCommandLine parser({
                                   "input-fhMap",
                               },
                               {});
    parser.markRequired({ "input-fhMap" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const std::string input = parser.getArg("input-fhMap");

    Core::CoreApplication fhCoreApp({ Core::CoreApplication::Option::ResourceLibraryApp,
                                      Core::CoreApplication::Option::ResourceLibraryLocalData,
                                      Core::CoreApplication::Option::GameDatabase,
                                      Core::CoreApplication::Option::RNG });
    Gui::Application      fhApp(&fhCoreApp);
    QApplication          app(argc, argv);

    fhApp.load();

    MapEditorWidget dlg(
        fhCoreApp.getDatabaseContainer(),
        fhCoreApp.getRandomGeneratorFactory(),
        fhApp.getGraphicsLibrary(),
        fhApp.getModelsProvider());
    dlg.show();

    return app.exec();
}
