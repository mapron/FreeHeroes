/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#include <QGuiApplication>

#include "CoreApplication.hpp"
#include "CommandLineUtils.hpp"

#include "ConversionHandler.hpp"

using namespace FreeHeroes;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({
                                   "input-dat",
                                   "input-folder",
                                   "input-def",
                                   "input-png",
                                   "output-dat",
                                   "output-folder",
                                   "output-def",
                                   "output-png",
                                   "force",
                                   "uncompress",
                                   "cleanupFolder",
                                   "prettyJson",
                               },
                               { "tasks" });

    parser.markRequired({ "tasks" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Converter invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const std::vector<std::string> tasksStr      = parser.getMultiArg("tasks");
    const bool                     forceWrite    = parser.getArg("force") == "1";
    const bool                     uncompress    = parser.getArg("uncompress") == "1";
    const bool                     cleanupFolder = parser.getArg("cleanupFolder") == "1";
    const bool                     prettyJson    = parser.getArg("prettyJson") == "1";

    Core::CoreApplication fhCoreApp(std::set<Core::CoreApplication::Option>{});
    fhCoreApp.initLogger();
    if (!fhCoreApp.load())
        return 1;

    QGuiApplication guiApp(argc, argv);

    auto makePaths = [&parser](const std::string& prefix) -> ConversionHandler::PathsSet {
        const auto dat    = Core::string2path(parser.getArg(prefix + "dat"));
        const auto def    = Core::string2path(parser.getArg(prefix + "def"));
        const auto png    = Core::string2path(parser.getArg(prefix + "png"));
        const auto folder = Core::string2path(parser.getArg(prefix + "folder"));

        Core::std_path pngJson;
        if (!png.empty()) {
            const auto        pngFolder    = png.parent_path();
            const std::string resourceName = Core::path2string(png.filename().stem());

            pngJson = pngFolder / (Core::string2path(resourceName).concat(".fh.json"));
        }

        return ConversionHandler::PathsSet{
            .m_datFile     = dat,
            .m_defFile     = def,
            .m_pngFile     = png,
            .m_pngJsonFile = pngJson,
            .m_folder      = folder,
        };
    };

    ConversionHandler converter(std::cerr,
                                ConversionHandler::Settings{
                                    .m_inputs        = makePaths("input-"),
                                    .m_outputs       = makePaths("output-"),
                                    .m_forceWrite    = forceWrite,
                                    .m_cleanupFolder = cleanupFolder,
                                    .m_uncompress    = uncompress,
                                    .m_prettyJson    = prettyJson,
                                });

    for (const std::string& taskStr : tasksStr) {
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
    }

    return 0;
}