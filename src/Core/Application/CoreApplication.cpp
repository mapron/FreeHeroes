/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CoreApplication.hpp"

// Core
#include "ResourceLibraryFactory.hpp"
#include "GameDatabaseContainer.hpp"
#include "MernelPlatform/Profiler.hpp"
#include "RandomGenerator.hpp"
#include "MernelPlatform/Logger_details.hpp"

namespace FreeHeroes::Core {
using namespace Mernel;
namespace {

constexpr const char* s_FHFolderName = "FreeHeroes";
}

CoreApplication::CoreApplication(const std::string& appName)
    : m_locations(s_FHFolderName, appName)
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

    ResourceLibraryFactory factory;
    {
        ProfilerScope scope("ResourceLibrary search");
        if (m_loadUserMods)
            factory.scanForMods(m_locations.getAppdataDir() / "Resources");

        if (m_loadAppBinMods)
            factory.scanForMods(m_locations.getBinDir() / "gameResources");
    }
    {
        ProfilerScope scope("ResourceLibrary load");
        factory.scanModSubfolders();
        m_resourceLibrary = factory.create(m_customLoadSeqence);
    }

    m_randomGeneratorFactory = std::make_shared<RandomGeneratorFactory>();

    {
        ProfilerScope scope("GameDatabaseContainer load");
        m_gameDatabaseContainer = std::make_shared<GameDatabaseContainer>(m_resourceLibrary.get());
    }

    Logger(Logger::Info) << "CoreApplication::load - end";
    return true;
}

const char* CoreApplication::getAppFolder()
{
    return s_FHFolderName;
}

}
