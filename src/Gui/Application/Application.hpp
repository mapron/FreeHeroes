/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiApplicationExport.hpp"

#include "FsUtils.hpp"

#include <memory>
#include <set>

namespace FreeHeroes {
namespace Core {
class IResourceLibrary;
class IGameDatabase;
class IRandomGeneratorFactory;
class IRandomGenerator;
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
    Application();
    ~Application();

    enum class Option
    {
        QtTranslations,
        Translations,
        ResourceLibrary,
        GameDatabase,
        RNG,
        GraphicsLibrary,
        MusicBox,
        CursorLibrary,
        LibraryModels,
        AppStyle
    };

    // clang-format off
    void load(const std::string & moduleName = "", std::set<Option> options = std::set<Option>{
                                                                      Option::QtTranslations,
                                                                      Option::Translations,
                                                                      Option::ResourceLibrary,
                                                                      Option::GameDatabase,
                                                                      Option::RNG,
                                                                      Option::GraphicsLibrary,
                                                                      Option::MusicBox,
                                                                      Option::CursorLibrary,
                                                                      Option::LibraryModels,
                                                                      Option::AppStyle
                                                                    });
    // clang-format on

    Core::IResourceLibrary& getResourceLibrary() const
    {
        return *m_resourceLibrary;
    }
    Core::IGameDatabase& getGameDatabase() const
    {
        return *m_gameDatabase;
    }
    Core::IRandomGeneratorFactory& getRandomGeneratorFactory() const
    {
        return *m_randomGeneratorFactory;
    }
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
    Gui::IAppSettings& getAppSettings();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    std::shared_ptr<Core::IResourceLibrary>        m_resourceLibrary;
    std::shared_ptr<Core::IGameDatabase>           m_gameDatabase;
    std::shared_ptr<Core::IRandomGeneratorFactory> m_randomGeneratorFactory;
    std::shared_ptr<Core::IRandomGenerator>        m_randomGeneratorUi;

    std::shared_ptr<Sound::IMusicBox> m_musicBox;

    std::shared_ptr<IGraphicsLibrary>      m_graphicsLibrary;
    std::shared_ptr<ICursorLibrary>        m_cursorLibrary;
    std::shared_ptr<LibraryModelsProvider> m_modelsProvider;
};

}

}
