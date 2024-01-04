/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ApiApplication.hpp"

#include "MernelPlatform/Logger_details.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "ResourceLibraryFactory.hpp"
#include "GameDatabaseContainer.hpp"
#include "RandomGenerator.hpp"

#include <string>

// @todo: switch to QGui!
#include <QApplication>

namespace FreeHeroes {
using namespace Mernel;

struct ApiApplication::Impl {
    Impl() = default;
    ~Impl()
    {
        Logger::SetLoggerBackend(nullptr);
    }
    struct LoggerString : public AbstractLoggerBackend {
        LoggerString(std::string& output)
            : AbstractLoggerBackend(Logger::Info, true, false, true, true)
            , m_output(output)
        {}
        void FlushMessageInternal(const std::string& message, int logLevel) const override
        {
            m_output += message;
        }

        std::string& m_output;
    };

    std::string                     m_lastOutput;
    ApiApplicationNoexcept::MapInfo m_mapInfo;

    int                           m_argc = 1; // qt require argc+argv by reference!
    std::vector<std::string>      m_argvData;
    std::vector<char*>            m_argv;
    std::unique_ptr<QApplication> m_qtApp;

    std::shared_ptr<const Core::IResourceLibrary>       m_resourceLibrary;
    std::shared_ptr<Core::IRandomGeneratorFactory>      m_randomGeneratorFactory;
    std::shared_ptr<const Core::IGameDatabaseContainer> m_gameDatabaseContainer;

    std::vector<uint32_t> m_rgba;
};

ApiApplication::ApiApplication() noexcept
    : m_impl(std::make_unique<Impl>())

{
}

const std::string& ApiApplication::getLastOutput() const noexcept
{
    return m_impl->m_lastOutput;
}

void ApiApplication::clearOutput() noexcept
{
    m_impl->m_lastOutput.clear();
}

void ApiApplication::init(const std::string& appResourcePath, const std::string& userResourcePath) noexcept(false)
{
    Logger::SetLoggerBackend(std::make_unique<Impl::LoggerString>(m_impl->m_lastOutput));

    m_impl->m_argvData = { "FreeHeroes", "" };
    m_impl->m_argv     = std::vector<char*>{ m_impl->m_argvData[0].data(), m_impl->m_argvData[1].data() };
    m_impl->m_qtApp    = std::make_unique<QApplication>(m_impl->m_argc, m_impl->m_argv.data());

    Logger(Logger::Info) << "CoreApplication::load - start";

    Core::ResourceLibraryFactory factory;
    {
        ProfilerScope scope("ResourceLibrary search");
        factory.scanForMods(string2path(appResourcePath));
        factory.scanForMods(string2path(userResourcePath));
    }
    {
        ProfilerScope scope("ResourceLibrary load");
        factory.scanModSubfolders();
        m_impl->m_resourceLibrary = factory.create({ "sod_res", "hota_res" });
    }

    m_impl->m_randomGeneratorFactory = std::make_shared<Core::RandomGeneratorFactory>();

    {
        ProfilerScope scope("GameDatabaseContainer load");
        m_impl->m_gameDatabaseContainer = std::make_shared<Core::GameDatabaseContainer>(m_impl->m_resourceLibrary.get());
    }

    Logger(Logger::Info) << "CoreApplication::load - end";
}

void ApiApplication::convertLoD(const std::string& lodPath, const std::string& userResourcePath) noexcept(false)
{
    throw std::runtime_error("todo");
}

void ApiApplication::loadMap(const std::string& mapPath) noexcept(false)
{
    throw std::runtime_error("todo");
}

const ApiApplication::MapInfo& ApiApplication::getMapInfo() const noexcept
{
    return m_impl->m_mapInfo;
}

void ApiApplication::derandomize()
{
    throw std::runtime_error("todo");
}

void ApiApplication::prepareRender()
{
    throw std::runtime_error("todo");
}

void ApiApplication::paint(int x, int y, int z, int width, int height)
{
    throw std::runtime_error("todo");
}

ApiApplication::RGBAArray ApiApplication::getRGBA() const noexcept
{
    return m_impl->m_rgba.size() > 0 ? m_impl->m_rgba.data() : nullptr;
}

ApiApplication::~ApiApplication() noexcept = default;

struct ApiApplicationNoexcept::Impl {
    ApiApplication m_app;
    std::string    m_lastError;
    std::string    m_lastOutput;

    bool handle(const char* scope, auto&& callback)
    {
        bool result = true;
        {
            Mernel::ProfilerScope profile(scope);
            m_app.clearOutput();
            m_lastError.clear();
            try {
                callback();
            }
            catch (std::exception& ex) {
                m_lastError = ex.what();
                result      = false;
            }
            catch (...) {
                m_lastError = "non-std exception caught.";
                result      = false;
            }
        }
        m_lastOutput = m_app.getLastOutput();
        m_app.clearOutput();
        auto profilerStr = ProfilerScope::printToStr();
        ProfilerScope::clearAll();
        if (result) {
            m_lastOutput += "Profiler data:\n";
            m_lastOutput += profilerStr;
        }

        return result;
    }
};

ApiApplicationNoexcept::ApiApplicationNoexcept() noexcept
    : m_impl(std::make_unique<Impl>())
{
}

ApiApplicationNoexcept::~ApiApplicationNoexcept() noexcept = default;

bool ApiApplicationNoexcept::init(const char* appResourcePath, const char* userResourcePath) noexcept
{
    return m_impl->handle("init", [=, this] { m_impl->m_app.init(appResourcePath, userResourcePath); });
}

bool ApiApplicationNoexcept::convertLoD(const char* lodPath, const char* userResourcePath) noexcept
{
    return m_impl->handle("convertLoD", [=, this] { m_impl->m_app.convertLoD(lodPath, userResourcePath); });
}

bool ApiApplicationNoexcept::loadMap(const char* mapPath) noexcept
{
    return m_impl->handle("loadMap", [=, this] { m_impl->m_app.loadMap(mapPath); });
}

const char* ApiApplicationNoexcept::getLastError() const noexcept
{
    return m_impl->m_lastError.c_str();
}

const char* ApiApplicationNoexcept::getLastOutput() const noexcept
{
    return m_impl->m_lastOutput.c_str();
}

const ApiApplicationNoexcept::MapInfo& ApiApplicationNoexcept::getMapInfo() const noexcept
{
    return m_impl->m_app.getMapInfo();
}

bool ApiApplicationNoexcept::derandomize() noexcept
{
    return m_impl->handle("derandomize", [=, this] { m_impl->m_app.derandomize(); });
}

bool ApiApplicationNoexcept::prepareRender() noexcept
{
    return m_impl->handle("prepareRender", [=, this] { m_impl->m_app.prepareRender(); });
}

bool ApiApplicationNoexcept::paint(int x, int y, int z, int width, int height) noexcept
{
    return m_impl->handle("paint", [=, this] { m_impl->m_app.paint(x, y, z, width, height); });
}

ApiApplicationNoexcept::RGBAArray ApiApplicationNoexcept::getRGBA() const noexcept
{
    return m_impl->m_app.getRGBA();
}

}
