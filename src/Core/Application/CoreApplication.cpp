/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CoreApplication.hpp"

// Core
#include "ResourceLibrary.hpp"
#include "GameDatabase.hpp"
#include "MernelPlatform/Profiler.hpp"
#include "RandomGenerator.hpp"
#include "MernelPlatform/Logger_details.hpp"

namespace FreeHeroes::Core {
using namespace Mernel;
namespace {

constexpr const char* s_FHFolderName = "FreeHeroes";
}

CoreApplication::CoreApplication(std::set<Option> options, const std::string& appName)
    : m_options(std::move(options))
    , m_locations(s_FHFolderName, appName)
{
}

CoreApplication::~CoreApplication() = default;

void CoreApplication::initLogger(int debugLevel) const
{
    Logger::SetLoggerBackend(std::make_unique<LoggerBackendFiles>(
        debugLevel,
        true,  /*duplicateInStderr*/
        true,  /*outputLoglevel   */
        false, /*outputTimestamp  */
        true,  /*outputTimeoffsets*/
        10,
        50000,
        m_locations.getAppdataDir() / "Logs"));
}

bool CoreApplication::load()
{
    Logger(Logger::Info) << "CoreApplication::load - start";

    ProfilerScope scopeAll("CoreApplication::load");
    if (m_options.contains(Option::ResourceLibraryApp) || m_options.contains(Option::ResourceLibraryLocalData)) {
        ResourceLibrary::ResourceLibraryPathList pathList;
        {
            ResourceLibrary::ResourceLibraryPathList listCfg;
            ProfilerScope                            scope("ResourceLibrary search");
            if (m_options.contains(Option::ResourceLibraryLocalData))
                pathList = ResourceLibrary::searchIndexInFolderRecursive(m_locations.getAppdataDir() / "Resources");
            if (m_options.contains(Option::ResourceLibraryApp))
                listCfg = ResourceLibrary::searchIndexInFolderRecursive(m_locations.getBinDir() / "gameResources");
            pathList.append(listCfg);
            pathList.topoSort();
        }
        {
            ProfilerScope scope("ResourceLibrary load");
            m_resourceLibrary = ResourceLibrary::makeMergedLibrary(pathList);
            auto dbIds        = m_resourceLibrary->getDatabaseIds();
        }
    }
    if (m_options.contains(Option::RNG)) {
        m_randomGeneratorFactory = std::make_shared<RandomGeneratorFactory>();
    }

    if (m_options.contains(Option::GameDatabase)) {
        ProfilerScope scope("GameDatabase load");
        try {
            m_gameDatabases[GameVersion::SOD]  = std::make_shared<Core::GameDatabase>(g_database_SOD, m_resourceLibrary.get());
            m_gameDatabases[GameVersion::HOTA] = std::make_shared<Core::GameDatabase>(g_database_HOTA, m_resourceLibrary.get());
        }
        catch (std::exception& ex) {
            Logger(Logger::Err) << ex.what();
            return false;
        }
    }

    Logger(Logger::Info) << "CoreApplication::load - end";
    return true;
}

const Core::IGameDatabase* CoreApplication::getDatabase(GameVersion version) const
{
    return m_gameDatabases.at(version).get();
}

const char* CoreApplication::getAppFolder()
{
    return s_FHFolderName;
}

}
