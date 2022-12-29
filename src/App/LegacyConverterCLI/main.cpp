/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#include "CoreApplication.hpp"
#include "CommandLineUtils.hpp"

#include "ConversionHandler.hpp"

using namespace FreeHeroes;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({
        "task",
        "input-dat",
        "input-folder",
        "input-def",
        "input-png",
        "output-dat",
        "output-folder",
        "output-def",
        "output-png",
        "force",
        "cleanupFolder",
    });
    parser.markRequired({ "task" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Converter invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const std::string taskStr       = parser.getArg("task");
    const bool        forceWrite    = parser.getArg("force") == "1";
    const bool        cleanupFolder = parser.getArg("cleanupFolder") == "1";

    Core::CoreApplication fhCoreApp;
    fhCoreApp.initLogger();
    if (!fhCoreApp.load())
        return 1;

    auto makePaths = [&parser](const std::string& prefix) -> ConversionHandler::PathsSet {
        const std::string dat    = parser.getArg(prefix + "dat");
        const std::string def    = parser.getArg(prefix + "def");
        const std::string png    = parser.getArg(prefix + "png");
        const std::string folder = parser.getArg(prefix + "folder");

        return ConversionHandler::PathsSet{
            .m_datFile = dat,
            .m_defFile = def,
            .m_pngFile = png,
            .m_folder  = folder,
        };
    };

    ConversionHandler converter(std::cerr,
                                fhCoreApp.getDatabaseContainer(),
                                ConversionHandler::Settings{
                                    .m_inputs        = makePaths("input-"),
                                    .m_outputs       = makePaths("output-"),
                                    .m_forceWrite    = forceWrite,
                                    .m_cleanupFolder = cleanupFolder,
                                });

    const ConversionHandler::Task task = stringToTask(taskStr);
    if (task == ConversionHandler::Task::Invalid) {
        std::cerr << "Unknown task: " << taskStr << "\n";
        return 1;
    }
    try {
        converter.run(task);
    }
    catch (std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }

    return 0;
}
