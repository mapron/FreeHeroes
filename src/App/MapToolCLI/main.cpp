/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
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
                                   "input-fhMap",
                                   "input-h3m",
                                   "input-h3c",
                                   "input-h3svg",
                                   "input-h3tpl",
                                   "input-diff-json",
                                   "input-folder",
                                   "output-fhMap",
                                   "output-h3m",
                                   "output-h3c",
                                   "output-h3svg",
                                   "output-h3tpl",
                                   "output-diff-json",
                                   "output-folder",
                                   "dump-uncompressed",
                                   "dump-json",
                                   "logging-level",
                               },
                               { "tasks" });
    parser.markRequired({ "tasks" });
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const auto        tasks            = parser.getMultiArg("tasks");
    const bool        dumpUncompressed = parser.getArg("dump-uncompressed") == "1";
    const bool        dumpJson         = parser.getArg("dump-json") == "1";
    const std::string loggingLevelStr  = parser.getArg("logging-level");

    const int loggingLevel = loggingLevelStr.empty() ? 4 : std::strtoull(loggingLevelStr.c_str(), nullptr, 10);

    Core::CoreApplication fhCoreApp;
    fhCoreApp.initLogger(loggingLevel);
    if (!fhCoreApp.load())
        return 1;

    auto makePaths = [&parser](const std::string& prefix) -> MapConverter::PathsSet {
        const std::string fhMap    = parser.getArg(prefix + "fhMap");
        const std::string folder   = parser.getArg(prefix + "folder");
        const std::string h3m      = parser.getArg(prefix + "h3m");
        const std::string h3c      = parser.getArg(prefix + "h3c");
        const std::string h3sav    = parser.getArg(prefix + "h3svg");
        const std::string h3tpl    = parser.getArg(prefix + "h3tpl");
        const std::string diffjson = parser.getArg(prefix + "diff-json");

        const std::string h3mUncompressed = h3m.empty() ? "" : h3m + ".uncompressed";
        const std::string h3mJson         = h3m.empty() ? "" : h3m + ".json";

        const std::string h3cUncompressed = h3c.empty() ? "" : h3c + ".uncompressed";
        const std::string h3cJson         = h3c.empty() ? "" : h3c + ".json";

        const std::string h3savUncompressed = h3sav.empty() ? "" : h3sav + ".uncompressed";
        const std::string h3savJson         = h3sav.empty() ? "" : h3sav + ".json";

        const std::string h3tplUncompressed = h3tpl.empty() ? "" : h3tpl + ".csv";
        const std::string h3tplJson         = h3tpl.empty() ? "" : h3tpl + ".json";

        MapConverter::PathsSet result;
        result.m_fhMap    = fhMap;
        result.m_h3m      = { .m_binary = h3m, .m_uncompressedBinary = h3mUncompressed, .m_json = h3mJson };
        result.m_h3c      = { .m_binary = h3c, .m_uncompressedBinary = h3cUncompressed, .m_json = h3cJson };
        result.m_h3svg    = { .m_binary = h3sav, .m_uncompressedBinary = h3savUncompressed, .m_json = h3savJson };
        result.m_h3tpl    = { .m_binary = h3tpl, .m_uncompressedBinary = h3tplUncompressed, .m_json = h3tplJson };
        result.m_jsonDiff = diffjson;
        result.m_folder   = folder;
        return result;
    };
    MapConverter::Settings settings{
        .m_inputs                  = makePaths("input-"),
        .m_outputs                 = makePaths("output-"),
        .m_dumpUncompressedBuffers = dumpUncompressed,
        .m_dumpBinaryDataJson      = dumpJson,
    };
    const bool                          batchModeEnabled = tasks.size() == 1 && !settings.m_inputs.m_folder.empty() && settings.m_inputs.m_h3m.m_binary.empty();
    std::vector<MapConverter::Settings> batch;
    if (batchModeEnabled) {
        for (const auto& it : std_fs::recursive_directory_iterator(settings.m_inputs.m_folder)) {
            if (!it.is_regular_file())
                continue;

            const auto path = it.path();
            const auto ext  = pathToLower(path.extension());
            if (ext != ".h3m")
                continue;
            settings.m_inputs.m_h3m.m_binary             = path;
            settings.m_inputs.m_h3m.m_uncompressedBinary = settings.m_inputs.m_h3m.m_binary;
            settings.m_inputs.m_h3m.m_uncompressedBinary.concat(".uncompressed");
            settings.m_inputs.m_h3m.m_json = settings.m_inputs.m_h3m.m_binary;
            settings.m_inputs.m_h3m.m_json.concat(".json");
            batch.push_back(settings);
        }
    } else {
        batch.push_back(settings);
    }

    MapConverter                        converter(std::cerr,
                           fhCoreApp.getDatabaseContainer(),
                           fhCoreApp.getRandomGeneratorFactory(),
                           settings);
    std::vector<MapConverter::Settings> batchFailedTrip;
    std::vector<MapConverter::Settings> batchFatalError;
    std::vector<MapConverter::Settings> batchSucceeded;

    for (const auto& setting : batch) {
        converter.setSettings(setting);
        bool isFailed = false;
        bool isFatal  = false;
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
                isFailed = true;
                continue;
            }
            catch (std::exception& ex) {
                std::cerr << ex.what() << "\n";
                isFatal = true;
                continue;
            }
        }
        if (isFatal)
            batchFatalError.push_back(setting);
        else if (isFailed)
            batchFailedTrip.push_back(setting);
        else
            batchSucceeded.push_back(setting);
    }
    if (batchModeEnabled) {
        std::cerr << "Successful runs:\n";
        for (auto& sett : batchSucceeded)
            std::cerr << sett.m_inputs.m_h3m.m_binary << "\n";
        std::cerr << "\nFailed round-trip runs:\n";
        for (auto& sett : batchFailedTrip)
            std::cerr << sett.m_inputs.m_h3m.m_binary << "\n";
        std::cerr << "\nFatal error runs:\n";
        for (auto& sett : batchFatalError)
            std::cerr << sett.m_inputs.m_h3m.m_binary << "\n";
    }

    return batchFailedTrip.size() + batchFatalError.size() != 0;
}
