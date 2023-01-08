/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QFrame>

#include <memory>

namespace Ui {
class MapToolWindow;
}

namespace FreeHeroes {

namespace Core {
class IGameDatabaseContainer;
class IRandomGeneratorFactory;
}
namespace Gui {
class IGraphicsLibrary;
class LibraryModelsProvider;
}

class MapEditorWidget;
class MapToolWindow : public QFrame {
public:
    MapToolWindow(
        const Core::IGameDatabaseContainer*  gameDatabaseContainer,
        const Core::IRandomGeneratorFactory* rngFactory,
        const Gui::IGraphicsLibrary*         graphicsLibrary,
        const Gui::LibraryModelsProvider*    modelsProvider,

        QWidget* parent = nullptr);

    void generateMap();
    ~MapToolWindow();

private:
    std::unique_ptr<Ui::MapToolWindow> m_ui;

    const Core::IGameDatabaseContainer* const  m_gameDatabaseContainer;
    const Core::IRandomGeneratorFactory* const m_rngFactory;

    const Gui::IGraphicsLibrary* const      m_graphicsLibrary;
    const Gui::LibraryModelsProvider* const m_modelsProvider;

    MapEditorWidget* m_editor = nullptr;
};

}
