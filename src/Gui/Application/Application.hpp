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
        Translations,
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
                    Option::Translations,
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
                                        Option::Translations,
                                        Option::GraphicsLibrary,
                                        Option::MusicBox,
                                        Option::CursorLibrary,
                                        Option::LibraryModels,
                                        Option::AppStyle },
                      tsExtraModule)
    {}
    ~Application();

    void load();

    IGraphicsLibrary& getGraphicsLibrary() const
    {
        return *m_graphicsLibrary;
    }
    Sound::IMusicBox& getMusicBox() const
    {
        return *m_musicBox;
    }
    ICursorLibrary& getCursorLibrary() const
    {
        return *m_cursorLibrary;
    }
    LibraryModelsProvider& getModelsProvider() const
    {
        return *m_modelsProvider;
    }
    const Core::IGameDatabase* getGameDatabase() const
    {
        return m_gameDatabaseUi;
    }
    Gui::IAppSettings& getAppSettings();

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
