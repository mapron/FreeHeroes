/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#include "CoreApplication.hpp"
#include "CommandLineUtils.hpp"

#include "MapConverter.hpp"

using namespace FreeHeroes;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({ "task", "json-input", "h3m-input", "json-output", "h3m-output" });
    parser.markRequired({ "task" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const std::string taskStr                   = parser.getArg("task");
    const std::string jsonInputFile             = parser.getArg("json-input");
    const std::string jsonOutputFile            = parser.getArg("json-output");
    const std::string h3mInputFile              = parser.getArg("h3m-input");
    const std::string h3mOutputFile             = parser.getArg("h3m-output");
    const std::string h3mUncompressedInputFile  = h3mInputFile.empty() ? "" : h3mInputFile + ".uncompressed";
    const std::string h3mUncompressedOutputFile = h3mOutputFile.empty() ? "" : h3mOutputFile + ".uncompressed";

    Core::CoreApplication fhCoreApp;
    fhCoreApp.initLogger();
    fhCoreApp.load();

    MapConverter converter(std::cerr,
                           fhCoreApp.getDatabaseContainer(),
                           fhCoreApp.getRandomGeneratorFactory(),
                           MapConverter::Settings{
                               .m_jsonInput{ jsonInputFile },
                               .m_jsonOutput{ jsonOutputFile },
                               .m_h3mInput{ h3mInputFile },
                               .m_h3mOutput{ h3mOutputFile },
                               .m_h3mUncompressedInput{ h3mUncompressedInputFile },
                               .m_h3mUncompressedOutput{ h3mUncompressedOutputFile },
                           });

    const MapConverter::Task task = stringToTask(taskStr);
    if (task == MapConverter::Task::Invalid) {
        std::cerr << "Unknown task: " << taskStr << "\n";
        return 1;
    }
    if (!converter.run(task))
        return 1;

    return 0;
}
