/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <iostream>

#ifndef DISABLE_QT
#include <QGuiApplication>
#endif

#ifndef DISABLE_QT
#include "EnvDetect.hpp"
#endif

#include "CoreApplication.hpp"
#include "MernelPlatform/CommandLineUtils.hpp"

#include "GameExtract.hpp"

using namespace FreeHeroes;
using namespace Mernel;

int main(int argc, char** argv)
{
    AbstractCommandLine parser({ "appResourcePath", "heroesRoot", "archiveExtractRoot", "mainExtractRoot", "force" });

    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Converter invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }

    std::cout << "Loading game database...\n";

    Core::CoreApplication fhCoreApp;
    fhCoreApp.initLogger();
    if (!fhCoreApp.load())
        return 1;

#ifndef DISABLE_QT
    QGuiApplication guiApp(argc, argv);
#endif

    std::cout << "Start game extractor module...\n";

    auto settings = GameExtract::Settings{
        .m_appResourcePath    = Mernel::string2path(parser.getArg("appResourcePath")),
        .m_heroesRoot         = Mernel::string2path(parser.getArg("heroesRoot")),
        .m_archiveExtractRoot = Mernel::string2path(parser.getArg("archiveExtractRoot")),
        .m_mainExtractRoot    = Mernel::string2path(parser.getArg("mainExtractRoot")),
        .m_forceExtract       = (parser.getArg("force") == "1"),
    };
    if (settings.m_appResourcePath.empty()) {
        settings.m_appResourcePath = fhCoreApp.getAppResourcesPath();
    }
    if (settings.m_heroesRoot.empty()) {
#ifndef DISABLE_QT
        settings.m_heroesRoot = findHeroes3Installation(); // @todo: check for WIN32 ?
#else
        std::cout << "--heroesRoot is required\n"
                  << parser.getHelp();
        return 1;
#endif
    }
    if (settings.m_archiveExtractRoot.empty()) {
        settings.m_archiveExtractRoot = fhCoreApp.getUserResourcesPath() / "Archives";
    }
    if (settings.m_mainExtractRoot.empty()) {
        settings.m_mainExtractRoot = fhCoreApp.getUserResourcesPath() / "Imported";
    }

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
