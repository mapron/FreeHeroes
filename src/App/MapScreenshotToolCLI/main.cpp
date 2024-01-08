/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <QGuiApplication>

#include <iostream>

#include "MernelPlatform/CommandLineUtils.hpp"
#include "MernelPlatform/Logger.hpp"

#include "CoreApplication.hpp"
#include "Application.hpp"

#include "FHMap.hpp"
#include "MapConverter.hpp"

#include "SpriteMap.hpp"
#include "FHMapToSpriteMap.hpp"

#include "IRandomGenerator.hpp"
#include "IGameDatabase.hpp"

#include "IAppSettings.hpp"

#include "SpriteMapPainter.hpp"
#include "ViewSettings.hpp"

#include <QImage>
#include <QPainter>

namespace FreeHeroes {

class ScreenshotTool {
    const Core::IGameDatabaseContainer*  m_gameDatabaseContainer;
    const Core::IRandomGeneratorFactory* m_rngFactory;
    const Gui::IGraphicsLibrary*         m_graphicsLibrary;
    Gui::IAppSettings*                   m_appSettings;

public:
    struct Impl {
        FHMap        m_map;
        SpriteMap    m_spriteMap;
        ViewSettings m_viewSettings;

        int m_depth = 0;
    };
    std::unique_ptr<Impl> m_impl = std::make_unique<Impl>();

    ScreenshotTool(
        const Core::IGameDatabaseContainer*  gameDatabaseContainer,
        const Core::IRandomGeneratorFactory* rngFactory,
        const Gui::IGraphicsLibrary*         graphicsLibrary,
        Gui::IAppSettings*                   appSettings)
        : m_gameDatabaseContainer(gameDatabaseContainer)
        , m_rngFactory(rngFactory)
        , m_graphicsLibrary(graphicsLibrary)
        , m_appSettings(appSettings)
    {
        Mernel::PropertyTree jdata = m_appSettings->loadCustomJson("mapEditor");
        if (jdata.isMap())
            m_impl->m_viewSettings.fromJson(jdata);
    }

    struct ScreenshotTask {
        std::string m_filename;
        bool        m_strict = false;

        bool        m_dryRun = false;
        std::string m_outputSurface;
        std::string m_outputUnderground;

        std::string m_minimapSurface;
        std::string m_minimapUnderground;

        int m_minimapSize = 144;
        int m_maxSize     = 32000;
    };

    bool load(const std::string& filename,
              bool               strict)
    {
        std::ostringstream os;
        auto               fullpath = Mernel::string2path(filename);
        auto               ext      = fullpath.extension();
        const bool         isH3M    = Mernel::path2string(ext) == ".h3m";

        MapConverter::Settings sett;
        if (isH3M)
            sett.m_inputs = { .m_h3m = { .m_binary = fullpath } };
        else
            sett.m_inputs = { .m_fhMap = fullpath };

        try {
            m_impl->m_viewSettings.m_renderSettings.m_strict = strict;
            MapConverter converter(os,
                                   m_gameDatabaseContainer,
                                   m_rngFactory,
                                   sett);

            if (isH3M)
                converter.run(MapConverter::Task::LoadH3M);
            else
                converter.run(MapConverter::Task::LoadFH);

            assert(converter.m_mapFH.m_database);
            m_impl->m_map = std::move(converter.m_mapFH);

            MapRenderer renderer(m_impl->m_viewSettings.m_renderSettings);
            m_impl->m_spriteMap = renderer.render(m_impl->m_map, m_graphicsLibrary);

            m_impl->m_depth = 0;
            return true;
        }
        catch (std::exception& ex) {
            Mernel::Logger(Mernel::Logger::Err) << ex.what();
            return false;
        }
    }

    bool saveScreenshots(const ScreenshotTask& task)
    {
        Mernel::Logger(Mernel::Logger::Notice) << "Loading " << task.m_filename;
        if (!load(task.m_filename, task.m_strict))
            return false;

        try {
            for (int d : { 0, 1 }) {
                const std::string& outFilename  = d == 0 ? task.m_outputSurface : task.m_outputUnderground;
                const std::string& miniFilename = d == 0 ? task.m_minimapSurface : task.m_minimapUnderground;

                if (d >= m_impl->m_spriteMap.m_depth)
                    break;

                m_impl->m_depth = d;

                m_impl->m_viewSettings.m_paintSettings.m_strict = task.m_strict;
                SpriteMapPainter spainter(&(m_impl->m_viewSettings.m_paintSettings), m_impl->m_depth);

                const auto tileSize = m_impl->m_viewSettings.m_paintSettings.m_tileSize;

                QSize mainSize(m_impl->m_spriteMap.m_width * tileSize, m_impl->m_spriteMap.m_height * tileSize);

                QImage   imageMap(mainSize, QImage::Format_ARGB32);
                QPainter painterMap(&imageMap);

                QImage   imageMini(task.m_minimapSize, task.m_minimapSize, QImage::Format_ARGB32);
                QPainter painterMini(&imageMini);

                spainter.paint(&painterMap, &(m_impl->m_spriteMap), 0, 0);
                spainter.paintMinimap(&painterMini, &(m_impl->m_spriteMap), QSize{ task.m_minimapSize, task.m_minimapSize }, QRectF());

                if (task.m_dryRun)
                    continue;

                if (!outFilename.empty()) {
                    QImage imageMapSaving = imageMap;
                    if (mainSize.width() > task.m_maxSize) {
                        while (mainSize.width() > task.m_maxSize) {
                            mainSize = mainSize / 2.0;
                        }
                        imageMapSaving = imageMapSaving.scaled(mainSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    }
                    Mernel::Logger(Mernel::Logger::Notice) << "Saving map to: " << outFilename;
                    imageMapSaving.save(QString::fromStdString(outFilename));
                }
                if (!miniFilename.empty()) {
                    Mernel::Logger(Mernel::Logger::Notice) << "Saving minimap to: " << miniFilename;
                    imageMini.save(QString::fromStdString(miniFilename));
                }
            }
        }
        catch (std::exception& ex) {
            Mernel::Logger(Mernel::Logger::Err) << ex.what();
            return false;
        }

        return true;
    }
};

}

int main(int argc, char* argv[])
{
    using namespace FreeHeroes;
    using namespace Mernel;

    AbstractCommandLine parser({
                                   "input",
                                   "input-batch-test",
                                   "output-png-surface",
                                   "output-png-underground",
                                   "output-png-mini-surface",
                                   "output-png-mini-underground",
                                   "logging-level",
                                   "minimap-size",
                                   "max-size",
                                   "strict", // if save have errors it's over
                               },
                               {});
    if (!parser.parseArgs(std::cerr, argc, argv)) {
        std::cerr << "Map utils invocation failed, correct usage is:\n";
        std::cerr << parser.getHelp();
        return 1;
    }
    ScreenshotTool::ScreenshotTask task;

    const std::string inputBatch = parser.getArg("input-batch-test");
    task.m_filename              = parser.getArg("input");
    task.m_outputSurface         = parser.getArg("output-png-surface");
    task.m_outputUnderground     = parser.getArg("output-png-underground");
    task.m_minimapSurface        = parser.getArg("output-png-mini-surface");
    task.m_minimapUnderground    = parser.getArg("output-png-mini-underground");
    task.m_strict                = parser.getArg("strict") == "1";

    if (inputBatch.empty() && task.m_filename.empty()) {
        std::cerr << "Either --input or --input-batch-test is required\n";
        return 1;
    }

    const std::string loggingLevelStr = parser.getArg("logging-level");
    const int         loggingLevel    = loggingLevelStr.empty() ? 4 : std::strtoull(loggingLevelStr.c_str(), nullptr, 10);

    const std::string minimapSizeStr = parser.getArg("minimap-size");
    if (!minimapSizeStr.empty())
        task.m_minimapSize = std::strtoull(minimapSizeStr.c_str(), nullptr, 10);
    const std::string maxSizeStr = parser.getArg("max-size");
    if (!maxSizeStr.empty())
        task.m_maxSize = std::strtoull(maxSizeStr.c_str(), nullptr, 10);

    Core::CoreApplication fhCoreApp;
    fhCoreApp.setLoadUserMods(true);

    Gui::Application fhApp(&fhCoreApp);
    QGuiApplication  app(argc, argv);

    fhApp.load(loggingLevel);

    ScreenshotTool dlg(
        fhCoreApp.getDatabaseContainer(),
        fhCoreApp.getRandomGeneratorFactory(),
        fhApp.getGraphicsLibrary(),
        fhApp.getAppSettings());

    std::vector<ScreenshotTool::ScreenshotTask> tasks;
    std::vector<ScreenshotTool::ScreenshotTask> tasksError;
    if (inputBatch.empty()) {
        tasks.push_back(task);
    } else {
        task.m_dryRun = true;

        for (const auto& it : std_fs::recursive_directory_iterator(Mernel::string2path(inputBatch))) {
            if (!it.is_regular_file())
                continue;

            const auto path = it.path();
            const auto ext  = pathToLower(path.extension());
            if (ext != ".h3m")
                continue;

            task.m_filename = Mernel::path2string(path);
            tasks.push_back(task);
        }
    }

    for (const auto& task1 : tasks) {
        if (!dlg.saveScreenshots(task1)) {
            tasksError.push_back(task1);
        }
    }

    return tasksError.empty() ? 0 : 1;
}
