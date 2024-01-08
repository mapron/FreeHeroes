/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include "CoreApplicationExport.hpp"

#include "IGameDatabase.hpp"

#include <memory>
#include <map>
#include <set>

namespace FreeHeroes::Core {
class IResourceLibrary;
class IRandomGeneratorFactory;
class IRandomGenerator;

class COREAPPLICATION_EXPORT CoreApplication {
public:
    CoreApplication(const std::string& appName = "");
    ~CoreApplication();

    void initLogger(int debugLevel = 5) const;

    void setLoadAppBinMods(bool load) { m_loadAppBinMods = load; }
    void setLoadUserMods(bool load) { m_loadUserMods = load; }
    void setLoadUserModSequence(std::vector<std::string> order) { m_customLoadSeqence = std::move(order); }

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
        return m_gameDatabaseContainer.get();
    }
    Mernel::std_path getAppDataRoot() const { return m_appDataRoot; }
    Mernel::std_path getBinRoot() const { return m_binRoot; }
    Mernel::std_path getUserResourcesPath() const { return m_userResources; }
    Mernel::std_path getAppResourcesPath() const { return m_appResources; }

    static const char* getAppFolder();

private:
    std::shared_ptr<const IResourceLibrary>       m_resourceLibrary;
    std::shared_ptr<IRandomGeneratorFactory>      m_randomGeneratorFactory;
    std::shared_ptr<const IGameDatabaseContainer> m_gameDatabaseContainer;

    Mernel::std_path m_appDataRoot;
    Mernel::std_path m_binRoot;
    Mernel::std_path m_userResources;
    Mernel::std_path m_appResources;

    bool                     m_loadAppBinMods = true;
    bool                     m_loadUserMods   = false;
    std::vector<std::string> m_customLoadSeqence;
};

}
