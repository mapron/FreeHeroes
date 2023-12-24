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
                                   "strict", // if save have errors it's over
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
    const bool        strict            = parser.getArg("strict") == "1";

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
        bool hasError = false;
        for (const auto& it : std_fs::recursive_directory_iterator(Mernel::string2path(inputBatch))) {
            if (!it.is_regular_file())
                continue;

            const auto path = it.path();
            const auto ext  = pathToLower(path.extension());
            if (ext != ".h3m")
                continue;

            std::cerr << "\nLoading: " << path << "\n";
            if (!dlg.load(Mernel::path2string(path), strict, true))
                return 1;

            std::cerr << "\nSaving: " << path << "\n";
            if (!hasError)
                dlg.saveScreenshots("DRYRUN", "DRYRUN");
        }
        return hasError;
    } else {
        if (!input.empty()) {
            if (!dlg.load(input, strict, true))
                return 1;
        }

        if (dlg.saveScreenshots(outputSurface, outputUnderground)) // true - saved in silent mode
            return 0;
    }

    dlg.show();

    return app.exec();
}
