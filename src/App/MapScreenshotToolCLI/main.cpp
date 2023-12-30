/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
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
                                   "output-png-mini-surface",
                                   "output-png-mini-underground",
                                   "logging-level",
                                   "minimap-size",
                                   "max-size",
                                   "strict", // if save have errors it's over
                               },
                               {});
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }
    MapEditorWidget::ScreenshotTask task;

    const std::string inputBatch = parser.getArg("input-batch-test");
    task.m_filename              = parser.getArg("input");
    task.m_outputSurface         = parser.getArg("output-png-surface");
    task.m_outputUnderground     = parser.getArg("output-png-underground");
    task.m_minimapSurface        = parser.getArg("output-png-mini-surface");
    task.m_minimapUnderground    = parser.getArg("output-png-mini-underground");
    task.m_strict                = parser.getArg("strict") == "1";

    if (inputBatch.empty() && task.m_filename.empty()) {
        std::cerr << "Either --input or --input-batch-test is required\n";
        return 1;
    }

    const std::string loggingLevelStr = parser.getArg("logging-level");
    const int         loggingLevel    = loggingLevelStr.empty() ? 4 : std::strtoull(loggingLevelStr.c_str(), nullptr, 10);

    const std::string minimapSizeStr = parser.getArg("minimap-size");
    if (!minimapSizeStr.empty())
        task.m_minimapSize = std::strtoull(minimapSizeStr.c_str(), nullptr, 10);
    const std::string maxSizeStr = parser.getArg("max-size");
    if (!maxSizeStr.empty())
        task.m_maxSize = std::strtoull(maxSizeStr.c_str(), nullptr, 10);

    Core::CoreApplication fhCoreApp;
    fhCoreApp.setLoadUserMods(true);

    Gui::Application fhApp(&fhCoreApp);
    QApplication     app(argc, argv);

    fhApp.load(loggingLevel);

    MapEditorWidget dlg(
        fhCoreApp.getDatabaseContainer(),
        fhCoreApp.getRandomGeneratorFactory(),
        fhApp.getGraphicsLibrary(),
        fhApp.getModelsProvider(),
        fhApp.getAppSettings());

    std::vector<MapEditorWidget::ScreenshotTask> tasks;
    std::vector<MapEditorWidget::ScreenshotTask> tasksError;
    if (inputBatch.empty()) {
        tasks.push_back(task);
    } else {
        task.m_dryRun = true;

        for (const auto& it : std_fs::recursive_directory_iterator(Mernel::string2path(inputBatch))) {
            if (!it.is_regular_file())
                continue;

            const auto path = it.path();
            const auto ext  = pathToLower(path.extension());
            if (ext != ".h3m")
                continue;

            task.m_filename = Mernel::path2string(path);
            tasks.push_back(task);
        }
    }

    for (const auto& task1 : tasks) {
        if (!dlg.saveScreenshots(task1)) {
            tasksError.push_back(task1);
        }
    }

    return tasksError.empty() ? 0 : 1;
}
