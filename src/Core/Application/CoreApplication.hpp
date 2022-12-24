/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CoreApplicationExport.hpp"

#include "IGameDatabase.hpp"
#include "AppLocations.hpp"

#include <memory>
#include <map>
#include <set>

namespace FreeHeroes::Core {
class IResourceLibrary;
class IGameDatabase;
class IRandomGeneratorFactory;
class IRandomGenerator;

class COREAPPLICATION_EXPORT CoreApplication : private IGameDatabaseContainer {
public:
    enum class Option
    {
        ResourceLibraryApp,       // include files shipped with application
        ResourceLibraryLocalData, // include user data files (after conversion)
        GameDatabase,
        RNG
    };

public:
    CoreApplication(std::set<Option>   options = std::set<Option>{ Option::ResourceLibraryApp, Option::GameDatabase, Option::RNG },
                    const std::string& appName = "");
    ~CoreApplication();

    void initLogger(int debugLevel = 5) const;

    bool load();

    const IResourceLibrary* getResourceLibrary() const
    {
        return m_resourceLibrary.get();
    }
    const IRandomGeneratorFactory* getRandomGeneratorFactory() const
    {
        return m_randomGeneratorFactory.get();
    }
    const IGameDatabaseContainer* getDatabaseContainer() const
    {
        return this;
    }

    [[nodiscard]] const IGameDatabase* getDatabase(GameVersion version) const override;
    bool                               hasDatabases() const { return !m_gameDatabases.empty(); }

    const AppLocations& getLocations() const { return m_locations; }

    static const char* getAppFolder();

private:
    const std::set<Option>                                m_options;
    std::shared_ptr<IResourceLibrary>                     m_resourceLibrary;
    std::map<GameVersion, std::shared_ptr<IGameDatabase>> m_gameDatabases;
    std::shared_ptr<IRandomGeneratorFactory>              m_randomGeneratorFactory;

    AppLocations m_locations;
};

}
