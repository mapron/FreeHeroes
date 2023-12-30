/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiApplicationExport.hpp"

#include <memory>
#include <string>
#include <set>

namespace FreeHeroes {
namespace Core {
class IGameDatabase;
class IRandomGenerator;
class CoreApplication;
}
namespace Sound {
class IMusicBox;
}

namespace Gui {
class IGraphicsLibrary;
class ICursorLibrary;
class IMusicBox;
class IAppSettings;
class LibraryModelsProvider;

class GUIAPPLICATION_EXPORT Application {
public:
    enum class Option
    {
        QtTranslations,
        GraphicsLibrary,
        MusicBox,
        CursorLibrary,
        LibraryModels,
        AppStyle
    };

public:
    Application(Core::CoreApplication* coreApp,
                std::set<Option>       options = std::set<Option>{
                    Option::QtTranslations,
                    Option::GraphicsLibrary,
                    Option::MusicBox,
                    Option::CursorLibrary,
                    Option::LibraryModels,
                    Option::AppStyle },
                const std::string& tsExtraModule = "");
    Application(Core::CoreApplication* coreApp,
                const std::string&     tsExtraModule)
        : Application(coreApp,
                      std::set<Option>{ Option::QtTranslations,
                                        Option::GraphicsLibrary,
                                        Option::MusicBox,
                                        Option::CursorLibrary,
                                        Option::LibraryModels,
                                        Option::AppStyle },
                      tsExtraModule)
    {}
    ~Application();

    bool load(int logLevel = -1);

    const IGraphicsLibrary* getGraphicsLibrary() const
    {
        return m_graphicsLibrary.get();
    }
    Sound::IMusicBox* getMusicBox() const
    {
        return m_musicBox.get();
    }
    const ICursorLibrary* getCursorLibrary() const
    {
        return m_cursorLibrary.get();
    }
    const LibraryModelsProvider* getModelsProvider() const
    {
        return m_modelsProvider.get();
    }
    const Core::IGameDatabase* getGameDatabase() const
    {
        return m_gameDatabaseUi;
    }
    Gui::IAppSettings* getAppSettings();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    const std::set<Option> m_options;

    const Core::IGameDatabase* m_gameDatabaseUi = nullptr;

    std::shared_ptr<Sound::IMusicBox> m_musicBox;

    std::shared_ptr<IGraphicsLibrary>      m_graphicsLibrary;
    std::shared_ptr<ICursorLibrary>        m_cursorLibrary;
    std::shared_ptr<LibraryModelsProvider> m_modelsProvider;
};

}

}
