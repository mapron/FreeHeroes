/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include <iostream>

#include "CoreApplication.hpp"
#include "MernelPlatform/CommandLineUtils.hpp"
#include "Application.hpp"

#include "MapEditorWidget.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;
    using namespace Mernel;

    AbstractCommandLine parser({
                                   "input",
                                   "input-batch-test",
                                   "output-png-surface",
                                   "output-png-underground",
                               },
                               {});
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const std::string input             = parser.getArg("input");
    const std::string inputBatch        = parser.getArg("input-batch-test");
    const std::string outputSurface     = parser.getArg("output-png-surface");
    const std::string outputUnderground = parser.getArg("output-png-underground");

    Core::CoreApplication fhCoreApp;
    fhCoreApp.setLoadUserMods(true);

    Gui::Application fhApp(&fhCoreApp);
    QApplication     app(argc, argv);

    fhApp.load();

    MapEditorWidget dlg(
        fhCoreApp.getDatabaseContainer(),
        fhCoreApp.getRandomGeneratorFactory(),
        fhApp.getGraphicsLibrary(),
        fhApp.getModelsProvider(),
        fhApp.getAppSettings());

    if (!inputBatch.empty()) {
        for (const auto& it : std_fs::recursive_directory_iterator(Mernel::string2path(inputBatch))) {
            if (!it.is_regular_file())
                continue;

            const auto path = it.path();
            const auto ext  = pathToLower(path.extension());
            if (ext != ".h3m")
                continue;

            std::cerr << "\nLoading: " << path << "\n";
            dlg.load(Mernel::path2string(path));
            std::cerr << "\nSaving: " << path << "\n";
            try {
                dlg.saveScreenshots("DRYRUN", "DRYRUN");
            }
            catch (std::exception& e) {
                std::cerr << "error:" << e.what() << "\n";
            }
        }
    } else {
        if (!input.empty())
            dlg.load(input);

        if (dlg.saveScreenshots(outputSurface, outputUnderground))
            return 0;
    }

    dlg.show();

    return app.exec();
}
