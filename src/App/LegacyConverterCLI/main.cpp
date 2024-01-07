/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#include "CoreApplication.hpp"
#include "MernelPlatform/CommandLineUtils.hpp"

#include "ConversionHandler.hpp"

using namespace FreeHeroes;
using namespace Mernel;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({
                                   "input-dat",
                                   "input-folder",
                                   "input-def",
                                   "input-json",
                                   "output-dat",
                                   "output-folder",
                                   "output-def",
                                   "output-json",
                                   "force",
                                   "uncompress",
                                   "cleanupFolder",
                                   "prettyJson",
                                   "mergePng",
                                   "transparentKeyColor",
                               },
                               { "tasks" });

    parser.markRequired({ "tasks" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Converter invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const std::vector<std::string> tasksStr            = parser.getMultiArg("tasks");
    const bool                     forceWrite          = parser.getArg("force") == "1";
    const bool                     uncompress          = parser.getArg("uncompress") == "1";
    const bool                     cleanupFolder       = parser.getArg("cleanupFolder") == "1";
    const bool                     prettyJson          = parser.getArg("prettyJson") == "1";
    const bool                     mergePng            = parser.getArg("mergePng") == "1";
    const bool                     transparentKeyColor = parser.getArg("transparentKeyColor") == "1";

    Core::CoreApplication fhCoreApp;
    fhCoreApp.setLoadAppBinMods(false);
    fhCoreApp.initLogger();
    if (!fhCoreApp.load())
        return 1;

    auto makePaths = [&parser](const std::string& prefix) -> ConversionHandler::PathsSet {
        const auto dat     = Mernel::string2path(parser.getArg(prefix + "dat"));
        const auto def     = Mernel::string2path(parser.getArg(prefix + "def"));
        const auto pngJson = Mernel::string2path(parser.getArg(prefix + "json"));
        const auto folder  = Mernel::string2path(parser.getArg(prefix + "folder"));

        return ConversionHandler::PathsSet{
            .m_datFile     = dat,
            .m_defFile     = def,
            .m_pngJsonFile = pngJson,
            .m_folder      = folder,
        };
    };

    ConversionHandler converter(std::cerr,
                                ConversionHandler::Settings{
                                    .m_inputs              = makePaths("input-"),
                                    .m_outputs             = makePaths("output-"),
                                    .m_forceWrite          = forceWrite,
                                    .m_cleanupFolder       = cleanupFolder,
                                    .m_uncompressArchive   = uncompress,
                                    .m_prettyJson          = prettyJson,
                                    .m_mergePng            = mergePng,
                                    .m_transparentKeyColor = transparentKeyColor,
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
