/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "CoreApplication.hpp"
#include "Application.hpp"

#include "SoundTestWidget.hpp"

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;

    Core::CoreApplication fhCoreApp({ Core::CoreApplication::Option::ResourceLibraryApp, Core::CoreApplication::Option::ResourceLibraryLocalData, Core::CoreApplication::Option::RNG });
    Gui::Application      fhApp(&fhCoreApp, { Gui::Application::Option::MusicBox });
    QApplication          app(argc, argv);

    fhApp.load();
    auto*           musicBox = fhApp.getMusicBox();
    SoundTestWidget dlg(musicBox);
    dlg.show();

    return app.exec();
}
