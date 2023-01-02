/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#include <QGuiApplication>

#include "CoreApplication.hpp"
#include "MernelPlatform/CommandLineUtils.hpp"

#include "GameExtract.hpp"

using namespace FreeHeroes;
using namespace Mernel;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({ "heroesRoot", "archiveExtractRoot", "mainExtractRoot", "knownResourcesFile", "force" });
    parser.markRequired({ "archiveExtractRoot", "mainExtractRoot" });

    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Converter invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    const auto settings = GameExtract::Settings{
        .m_heroesRoot         = Mernel::string2path(parser.getArg("heroesRoot")),
        .m_archiveExtractRoot = Mernel::string2path(parser.getArg("archiveExtractRoot")),
        .m_mainExtractRoot    = Mernel::string2path(parser.getArg("mainExtractRoot")),
        .m_knownResourcesFile = Mernel::string2path(parser.getArg("knownResourcesFile")),
        .m_forceExtract       = (parser.getArg("force") == "1"),
    };

    std::cout << "Loading game database...\n";

    Core::CoreApplication fhCoreApp;
    fhCoreApp.initLogger();
    if (!fhCoreApp.load())
        return 1;

    QGuiApplication guiApp(argc, argv);

    std::cout << "Start game extractor module...\n";

    GameExtract converter(fhCoreApp.getDatabaseContainer(), settings);

    try {
        converter.setMessageCallback([](const std::string& msg, bool) {
            std::cout << msg << '\n';
        });
        converter.setErrorCallback([](const std::string& msg) {
            std::cerr << msg << '\n';
        });
        converter.setProgressCallback([](int progress, int total) {
            std::cout << "[" << progress << " / " << total << "]\r";
            return false;
        });
        const auto probeInfo = converter.probe();
        std::cout << "SoD detected:" << probeInfo.m_hasSod << '\n';
        std::cout << "HotA detected:" << probeInfo.m_hasHota << '\n';
        std::cout << "HD mod detected:" << probeInfo.m_hasHD << '\n';
        std::cout << "AB detected:" << probeInfo.m_hasAB << '\n';
        if (probeInfo.isSuccess())
            converter.run(probeInfo);
    }
    catch (std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }

    return 0;
}
