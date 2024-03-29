/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QMainWindow>

#include <memory>

#include "MapUtilGuiExport.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabaseContainer;
class IRandomGeneratorFactory;
}
namespace Gui {
class IGraphicsLibrary;
class LibraryModelsProvider;
class IAppSettings;
}

class MAPUTILGUI_EXPORT MapEditorWidget : public QMainWindow {
public:
    MapEditorWidget(
        const Core::IGameDatabaseContainer*  gameDatabaseContainer,
        const Core::IRandomGeneratorFactory* rngFactory,
        const Gui::IGraphicsLibrary*         graphicsLibrary,
        const Gui::LibraryModelsProvider*    modelsProvider,
        Gui::IAppSettings*                   appSettings,

        QWidget* parent = nullptr);

    ~MapEditorWidget();

    void loadUserSettings();
    void saveUserSettings();

    bool load(const std::string& filename,
              bool               strict,
              bool               silent = false);
    void save(const std::string& filename, bool isH3M);
    void loadDialog();
    void saveH3MDialog();
    void saveFHDialog();
    void saveScreenshot();

    void updateMap();
    void derandomize();

private:
    void showCurrentItem();
    void updateAll();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    const Core::IGameDatabaseContainer* const  m_gameDatabaseContainer;
    const Core::IRandomGeneratorFactory* const m_rngFactory;

    const Gui::IGraphicsLibrary* const      m_graphicsLibrary;
    const Gui::LibraryModelsProvider* const m_modelsProvider;

    Gui::IAppSettings* const m_appSettings;
};

}
