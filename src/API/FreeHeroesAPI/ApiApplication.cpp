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
#include "MapConverter.hpp"
#include "GraphicsLibrary.hpp"

#include "GameExtract.hpp"
#include "SpriteMap.hpp"
#include "ViewSettings.hpp"
#include "FHMapToSpriteMap.hpp"
#include "SpriteMapPainterPixmap.hpp"
#include "Painter.hpp"

#include <string>

namespace FreeHeroes {
using namespace Mernel;

const uint32_t g_mapAnimationInterval = 160;

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

    Mernel::std_path                        m_appResourcePath;
    Mernel::std_path                        m_userResourcePath;
    Core::IResourceLibraryFactory::ModOrder m_modOrder = { "sod_res", "hota_res" };

    std::shared_ptr<const Core::IResourceLibrary>       m_resourceLibrary;
    std::shared_ptr<Core::IRandomGeneratorFactory>      m_randomGeneratorFactory;
    std::shared_ptr<const Core::IGameDatabaseContainer> m_gameDatabaseContainer;
    std::shared_ptr<Gui::IGraphicsLibrary>              m_graphicsLibrary;

    FHMap m_map;

    ScopeTimer   m_timer;
    SpriteMap    m_spriteMap;
    ViewSettings m_viewSettings;
    RenderWindow m_renderWindow;

    std::vector<uint8_t> m_rgba;
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

    Logger(Logger::Info) << "init - start";

    m_impl->m_appResourcePath  = string2path(appResourcePath);
    m_impl->m_userResourcePath = string2path(userResourcePath);

    Core::ResourceLibraryFactory factory;
    {
        ProfilerScope scope("ResourceLibrary search");
        factory.scanForMods(m_impl->m_appResourcePath);
        factory.scanForMods(m_impl->m_userResourcePath);
    }
    {
        ProfilerScope scope("ResourceLibrary load");
        factory.scanModSubfolders();
        m_impl->m_resourceLibrary = factory.create(m_impl->m_modOrder);
    }

    m_impl->m_randomGeneratorFactory = std::make_shared<Core::RandomGeneratorFactory>();

    {
        ProfilerScope scope("GameDatabaseContainer load");
        m_impl->m_gameDatabaseContainer = std::make_shared<Core::GameDatabaseContainer>(m_impl->m_resourceLibrary.get());
    }
    {
        ProfilerScope scope("GraphicsLibrary");
        m_impl->m_graphicsLibrary = std::make_shared<Gui::GraphicsLibrary>(m_impl->m_resourceLibrary.get());
    }

    Logger(Logger::Info) << "init - end";
}

void ApiApplication::reinit() noexcept(false)
{
    Logger(Logger::Info) << "reinit - start";

    Core::ResourceLibraryFactory factory;
    {
        ProfilerScope scope("ResourceLibrary search");
        factory.scanForMods(m_impl->m_appResourcePath);
        factory.scanForMods(m_impl->m_userResourcePath);
    }
    {
        ProfilerScope scope("ResourceLibrary load");
        factory.scanModSubfolders();
        m_impl->m_resourceLibrary = factory.create(m_impl->m_modOrder);
    }

    m_impl->m_randomGeneratorFactory = std::make_shared<Core::RandomGeneratorFactory>();

    {
        ProfilerScope scope("GameDatabaseContainer load");
        m_impl->m_gameDatabaseContainer = std::make_shared<Core::GameDatabaseContainer>(m_impl->m_resourceLibrary.get());
    }
    {
        ProfilerScope scope("GraphicsLibrary");
        m_impl->m_graphicsLibrary = std::make_shared<Gui::GraphicsLibrary>(m_impl->m_resourceLibrary.get());
    }
    {
        m_impl->m_map       = {};
        m_impl->m_spriteMap = {};
    }

    Logger(Logger::Info) << "reinit - end";
}

void ApiApplication::convertLoD(const std::string& lodPath) noexcept(false)
{
    const auto settings = GameExtract::Settings{
        .m_appResourcePath    = m_impl->m_appResourcePath,
        .m_archiveExtractRoot = m_impl->m_userResourcePath / "Archives",
        .m_mainExtractRoot    = m_impl->m_userResourcePath / "Imported",
        .m_forceExtract       = false,
        .m_skipIfFolderExist  = true,
        .m_needLocalization   = false,
    };

    GameExtract converter(m_impl->m_gameDatabaseContainer.get(), settings);

    converter.setMessageCallback([](const std::string& msg, bool) {
        Mernel::Logger(Mernel::Logger::Info) << msg;
    });
    converter.setErrorCallback([](const std::string& msg) {
        Mernel::Logger(Mernel::Logger::Err) << msg;
    });
    converter.setProgressCallback([](int progress, int total) {
        return false; // return true to cancel operation.
    });
    GameExtract::DetectedSources probeInfo;
    const auto                   filenameLower = Mernel::pathToLower(Mernel::string2path(lodPath).filename());
    probeInfo.m_hasHota                        = filenameLower == "hota.lod";
    probeInfo.m_hasSod                         = !probeInfo.m_hasHota;

    probeInfo.m_sources[GameExtract::SourceType::Archive].push_back(GameExtract::DetectedPath{ .m_path = Mernel::string2path(lodPath), .m_isSod = probeInfo.m_hasSod, .m_isHota = probeInfo.m_hasHota });
    if (probeInfo.isSuccess())
        converter.run(probeInfo);
}

void ApiApplication::loadMap(const std::string& mapPath) noexcept(false)
{
    std::ostringstream os;
    auto               fullpath = Mernel::string2path(mapPath);
    auto               ext      = fullpath.extension();
    const bool         isH3M    = Mernel::path2string(ext) == ".h3m";

    MapConverter::Settings sett;
    if (isH3M)
        sett.m_inputs = { .m_h3m = { .m_binary = fullpath } };
    else
        sett.m_inputs = { .m_fhMap = fullpath };

    try {
        MapConverter converter(os,
                               m_impl->m_gameDatabaseContainer.get(),
                               m_impl->m_randomGeneratorFactory.get(),
                               sett);

        if (isH3M)
            converter.run(MapConverter::Task::LoadH3M);
        else
            converter.run(MapConverter::Task::LoadFH);

        assert(converter.m_mapFH.m_database);
        m_impl->m_map                = std::move(converter.m_mapFH);
        m_impl->m_mapInfo            = { .m_width  = m_impl->m_map.m_tileMap.m_width,
                                         .m_height = m_impl->m_map.m_tileMap.m_height,
                                         .m_depth  = m_impl->m_map.m_tileMap.m_depth };
        m_impl->m_mapInfo.m_tileSize = 32;
        m_impl->m_mapInfo.m_version  = m_impl->m_map.isSoDMap() ? 1 : (m_impl->m_map.isHotAMap() ? 2 : 0);

        //updateMap();
    }
    catch (std::exception&) {
        m_impl->m_lastOutput += os.str();
        throw;
    }
}

const ApiApplication::MapInfo& ApiApplication::getMapInfo() const noexcept
{
    return m_impl->m_mapInfo;
}

void ApiApplication::derandomize()
{
    auto rng = m_impl->m_randomGeneratorFactory->create();
    rng->makeGoodSeed();
    m_impl->m_map.derandomize(rng.get());
}

void ApiApplication::setRenderWindow(const RenderWindow& renderWindow) noexcept
{
    m_impl->m_lastOutput.clear();
    m_impl->m_renderWindow = renderWindow;
}

void ApiApplication::prepareRender()
{
    //throw std::runtime_error("todo");
    const int maxObjectWidth  = 8;
    const int maxObjectHeight = 6;
    auto&     rset            = m_impl->m_viewSettings.m_renderSettings;
    rset.m_useRenderWindow    = true;
    rset.m_z                  = m_impl->m_renderWindow.m_z;
    rset.m_xMin               = m_impl->m_renderWindow.m_x - 1;
    rset.m_xMax               = m_impl->m_renderWindow.m_x + m_impl->m_renderWindow.m_width + maxObjectWidth;

    rset.m_yMin = m_impl->m_renderWindow.m_y - 1;
    rset.m_yMax = m_impl->m_renderWindow.m_y + m_impl->m_renderWindow.m_height + maxObjectHeight;

    rset.m_showEvents = false; // @todo: configurable?
    rset.m_showGrail  = false; // @todo: configurable?

    Logger(Logger::Info) << "Render map, window: z=" << rset.m_z << ", x=" << rset.m_xMin << ".." << rset.m_xMax << ", y=" << rset.m_yMin << ".." << rset.m_yMax;

    MapRenderer renderer(rset);
    assert(m_impl->m_map.m_database);
    m_impl->m_spriteMap = renderer.render(m_impl->m_map, m_impl->m_graphicsLibrary.get());
}

void ApiApplication::paint()
{
    PixmapSize windowSize(m_impl->m_renderWindow.m_width, m_impl->m_renderWindow.m_height);
    windowSize.m_height *= m_impl->m_mapInfo.m_tileSize;
    windowSize.m_width *= m_impl->m_mapInfo.m_tileSize;

    m_impl->m_rgba.resize(windowSize.m_width * windowSize.m_height * 4);

    SpriteMapPainterPixmap spainter(&(m_impl->m_viewSettings.m_paintSettings), m_impl->m_renderWindow.m_z);

    Pixmap  imageMap(windowSize);
    Painter painterMap(&imageMap);
    painterMap.translate(PixmapPoint(-m_impl->m_renderWindow.m_x * m_impl->m_mapInfo.m_tileSize, -m_impl->m_renderWindow.m_y * m_impl->m_mapInfo.m_tileSize));

    const auto tick = static_cast<uint32_t>(m_impl->m_timer.elapsedUS() / 1000 / g_mapAnimationInterval);

    spainter.paint(&painterMap, &(m_impl->m_spriteMap), tick, tick);
    for (int y1 = 0; y1 < windowSize.m_height; ++y1) {
        for (int x1 = 0; x1 < windowSize.m_width; ++x1) {
            uint8_t* bitmapPixel = m_impl->m_rgba.data() + y1 * windowSize.m_width * 4 + x1 * 4;
            auto&    color       = imageMap.get(x1, y1).m_color;
            bitmapPixel[0]       = color.m_r;
            bitmapPixel[1]       = color.m_g;
            bitmapPixel[2]       = color.m_b;
            bitmapPixel[3]       = color.m_a;
        }
    }
}

ApiApplication::Bitmap ApiApplication::getRGBA() const noexcept
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

bool ApiApplicationNoexcept::reinit() noexcept
{
    return m_impl->handle("reinit", [=, this] { m_impl->m_app.reinit(); });
}

bool ApiApplicationNoexcept::convertLoD(const char* lodPath) noexcept
{
    return m_impl->handle("convertLoD", [=, this] { m_impl->m_app.convertLoD(lodPath); });
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

bool ApiApplicationNoexcept::setRenderWindow(const RenderWindow& renderWindow) noexcept
{
    m_impl->m_lastError.clear();
    m_impl->m_lastOutput.clear();
    m_impl->m_app.setRenderWindow(renderWindow);
    return true;
}

bool ApiApplicationNoexcept::prepareRender() noexcept
{
    return m_impl->handle("prepareRender", [=, this] { m_impl->m_app.prepareRender(); });
}

bool ApiApplicationNoexcept::paint() noexcept
{
    return m_impl->handle("paint", [this] { m_impl->m_app.paint(); });
}

ApiApplicationNoexcept::Bitmap ApiApplicationNoexcept::getRGBA() const noexcept
{
    return m_impl->m_app.getRGBA();
}

}
