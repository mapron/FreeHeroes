/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#include "CoreApplication.hpp"
#include "MernelPlatform/CommandLineUtils.hpp"

#include "MapConverter.hpp"

using namespace FreeHeroes;
using namespace Mernel;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({
                                   "input-fhTpl",
                                   "output-fhMap",
                                   "output-fhTpl",
                                   "output-h3m",
                                   "logging-level",
                                   "logs-extra",
                                   "seed",
                                   "rng-settings-file",
                                   "stage-stop-after",
                                   "stage-show-debug",
                                   "heat-stop-after",
                                   "tile-filter",
                               },
                               { "tasks" });
    parser.markRequired({ "tasks" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const auto tasks = parser.getMultiArg("tasks");

    const std::string seedStr          = parser.getArg("seed");
    const std::string stopAfterHeatStr = parser.getArg("heat-stop-after");

    MapConverter::TemplateSettings templateSettings;
    templateSettings.m_seed            = std::strtoull(seedStr.c_str(), nullptr, 10);
    templateSettings.m_extraLogging    = parser.getArg("logs-extra") == "1";
    templateSettings.m_stopAfterHeat   = stopAfterHeatStr.empty() ? 1000 : std::strtol(stopAfterHeatStr.c_str(), nullptr, 10);
    templateSettings.m_stopAfterStage  = parser.getArg("stage-stop-after");
    templateSettings.m_showDebugStage  = parser.getArg("stage-show-debug");
    templateSettings.m_tileFilter      = parser.getArg("tile-filter");
    templateSettings.m_rngUserSettings = Mernel::string2path(parser.getArg("rng-settings-file"));

    const std::string loggingLevelStr = parser.getArg("logging-level");
    const int         loggingLevel    = loggingLevelStr.empty() ? 4 : std::strtoull(loggingLevelStr.c_str(), nullptr, 10);

    Core::CoreApplication fhCoreApp;
    fhCoreApp.initLogger(loggingLevel);
    if (!fhCoreApp.load())
        return 1;

    auto makePaths = [&parser](const std::string& prefix) -> MapConverter::PathsSet {
        const std::string fhMap      = parser.getArg(prefix + "fhMap");
        const std::string fhTemplate = parser.getArg(prefix + "fhTpl");
        const std::string h3m        = parser.getArg(prefix + "h3m");

        MapConverter::PathsSet result;
        result.m_fhMap      = fhMap;
        result.m_fhTemplate = fhTemplate;
        result.m_h3m        = { .m_binary = h3m };

        return result;
    };

    MapConverter::Settings settings{
        .m_inputs  = makePaths("input-"),
        .m_outputs = makePaths("output-"),
    };

    MapConverter converter(std::cerr,
                           fhCoreApp.getDatabaseContainer(),
                           fhCoreApp.getRandomGeneratorFactory(),
                           settings);
    converter.setTemplateSettings(templateSettings);

    for (const std::string& taskStr : tasks) {
        const MapConverter::Task task = stringToTask(taskStr);
        if (task == MapConverter::Task::Invalid) {
            std::cerr << "Unknown task: " << taskStr << "\n";
            return 1;
        }
        try {
            converter.run(task);
        }
        catch (MapConverter::RoundTripException&) {
            return 1;
        }
        catch (std::exception& ex) {
            std::cerr << ex.what() << "\n";
            return 1;
        }
    }

    return 0;
}
