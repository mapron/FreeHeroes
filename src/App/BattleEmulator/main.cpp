/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "EmulatorMainWidget.hpp"

#include "Application.hpp"

// Platform
#include "Profiler.hpp"
#include "Logger.hpp"

#include <QApplication>
#include <QMainWindow>

#include <iostream>
#include <set>


int main(int argc, char * argv[])
{

    using namespace FreeHeroes;
    using namespace Core;
    using namespace Gui;

    Application fhApp;

    try {
        QApplication app(argc, argv);
        fhApp.load("BattleEmulator");
        std::unique_ptr<BattleEmulator::EmulatorMainWidget> w;
        {
            Logger(Logger::Info) << "Start UI construct";
            ProfilerScope scope("UI Construction");

            w = std::make_unique<BattleEmulator::EmulatorMainWidget>(fhApp.getGraphicsLibrary(),
                                                 fhApp.getCursorLibrary(),
                                                 fhApp.getGameDatabase(),
                                                 fhApp.getRandomGeneratorFactory(),
                                                 fhApp.getMusicBox(),
                                                 fhApp.getAppSettings(),
                                                 fhApp.getModelsProvider());
            w->show();
            Logger(Logger::Info) << "End of UI construct";
        }

        return app.exec();
    } catch(std::exception & e) {
        Logger(Logger::Err) << e.what();
        return 1;
    }
}
