/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QApplication>

#include "Application.hpp"

#include "SoundTestWidget.hpp"


int main(int argc, char * argv[])
{
    using namespace FreeHeroes;

    Gui::Application fhApp;
    QApplication app(argc, argv);

    fhApp.load("", {Gui::Application::Option::ResourceLibrary, Gui::Application::Option::MusicBox});
    auto & musicBox = fhApp.getMusicBox();
    SoundTestWidget dlg(musicBox);
    dlg.show();

    return app.exec();
}

