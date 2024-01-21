/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameExtract.hpp"

#ifndef DISABLE_QT
#include "FsUtilsQt.hpp"
#include "LocalizationConverter.hpp"
#endif

#include "ConversionHandler.hpp"

#include "Archive.hpp"
#include "KnownResources.hpp"
#include "SpriteFile.hpp"
#include "IGameDatabase.hpp"
#include "Pixmap.hpp"
#include "Painter.hpp"

#include "MernelExecution/ParallelExecutor.hpp"
#include "MernelExecution/TaskQueueWatcher.hpp"
#include "MernelExecution/TaskQueue.hpp"

#include "MernelPlatform/Logger.hpp"
#include "MernelPlatform/Profiler.hpp"
#include "MernelPlatform/StringUtils.hpp"

#include <sstream>
#include <thread>

#ifndef DISABLE_QT
#include <QProcess> // ffmpeg
#endif

namespace FreeHeroes {
using namespace Mernel;

namespace {

struct ArchiveWrapper {
    std::ostringstream                 m_converterLog;
    std::unique_ptr<ConversionHandler> m_converter;

    Mernel::std_path m_dat;
    Mernel::std_path m_folder;
    std::string      m_datFilename;
    std::string      m_folderName;
    std::string      m_resourceModName;

    bool m_doExtract = false;
    bool m_doSkip    = false;
    bool m_required  = false;

    bool m_isSod  = false;
    bool m_isHota = false;
    bool m_isHD   = false;
};
#ifndef DISABLE_QT
bool executeFFMpeg(QStringList args, QString* programOut = nullptr)
{
    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", args);
    if (!ffmpeg.waitForStarted())
        return false;
    if (programOut)
        *programOut = ffmpeg.program();

    ffmpeg.closeWriteChannel();

    if (!ffmpeg.waitForFinished())
        return false;

    if (ffmpeg.exitStatus() == QProcess::CrashExit)
        return false;

    // output:
    // Duration: 00:00:00.15
    // .tens of milliseconds, .15 = 150-159 milliseconds.

    const int rc = ffmpeg.exitCode();
    if (rc != 0) {
        Logger(Logger::Err) << "Failed to execute ffmpeg: " << (args.join(' ').toStdString()) << "\n"
                            << ffmpeg.readAllStandardError().toStdString();
    }
    return rc == 0;
}
#endif

Mernel::std_path findPathChild(const Mernel::std_path& parent, const std::string& lowerCaseName)
{
    if (parent.empty())
        return {};
    for (auto&& it : Mernel::std_fs::directory_iterator(parent)) {
        if (it.is_regular_file() || it.is_directory()) {
            auto name = Mernel::pathToLower(it.path().filename());
            if (name == lowerCaseName) {
                std::error_code ec;
                if (it.is_regular_file()) {
                    auto size = Mernel::std_fs::file_size(it.path(), ec);
                    if (size == 0 || size == uintmax_t(-1))
                        continue;
                }

                return it.path();
            }
        }
    }
    return {};
}

}

class GameExtract::ConcatProcessor {
public:
    struct ConcatData {
        std::map<std_path, SpriteFile> m_sprites;
        bool                           m_makeFrames  = false;
        bool                           m_singleGroup = false;
        //std_path m_someSpritePath;
    };
    std::map<std::string, ConcatData> m_data;
    std::mutex                        m_mutex;

    void addSprite(const SpriteFile& sprite, const std_path& path, const std::string& concatId, bool makeFrames, bool singleGroup)
    {
        std::unique_lock lock(m_mutex);

        auto& data           = m_data[concatId];
        data.m_makeFrames    = makeFrames;
        data.m_singleGroup   = singleGroup;
        data.m_sprites[path] = sprite;
    }

    void create()
    {
        for (const auto& [id, concatData] : m_data) {
            std::vector<Pixmap> pixmaps;
            pixmaps.reserve(concatData.m_sprites.size());
            for (const auto& [path, sprite] : concatData.m_sprites) {
                pixmaps.push_back(*sprite.m_bitmaps[0].m_pixmap.get());
            }
            const auto root = concatData.m_sprites.begin()->first.parent_path();
            SpriteFile outFile;
            if (concatData.m_makeFrames) {
                outFile.m_embeddedBitmapData = false;
                std::vector<BitmapFile> bmps;
                for (auto& pix : pixmaps) {
                    BitmapFile bmp;
                    bmp.m_pixmap = std::make_shared<Pixmap>(pix);
                    bmps.push_back(std::move(bmp));
                }
                outFile.fromPixmapList(bmps, concatData.m_singleGroup);
            } else {
                PixmapSize size(0, 0);
                for (auto& pix : pixmaps) {
                    size.m_width = std::max(size.m_width, pix.m_size.m_width);
                    size.m_height += pix.m_size.m_height;
                }
                Pixmap      result(size);
                PixmapPoint offset(0, 0);
                for (auto& pix : pixmaps) {
                    Painter p(&result);
                    p.drawPixmap(offset, pix);
                    offset += PixmapPoint(0, pix.m_size.m_height);
                }
                BitmapFile bmp;
                bmp.m_pixmap = std::make_shared<Pixmap>(std::move(result));

                outFile.fromPixmap(std::move(bmp));
            }
            outFile.saveGuiSprite(root / (id + ".fhsprite.json"), {});
        }
    }
};

GameExtract::GameExtract(const Core::IGameDatabaseContainer* databaseContainer, Settings settings)
    : m_databaseContainer(databaseContainer)
    , m_settings(std::move(settings))
{
    if (m_settings.m_appResourcePath.empty())
        throw std::runtime_error("appResourcePath is required");
}

GameExtract::~GameExtract() = default;

GameExtract::DetectedSources GameExtract::probe() const
{
    DetectedSources result;
    result.m_heroesRoot = m_settings.m_heroesRoot;
    std::error_code ec;
    if (result.m_heroesRoot.empty() || !Mernel::std_fs::exists(result.m_heroesRoot, ec))
        return result;

    sendMessage("Probing path: " + Mernel::path2string(result.m_heroesRoot));

    Mernel::std_path dataFolder  = findPathChild(result.m_heroesRoot, "data"),
                     mp3Folder   = findPathChild(result.m_heroesRoot, "mp3"),
                     hdModFolder = findPathChild(result.m_heroesRoot, "_hd3_data");

    if (dataFolder.empty())
        return result;

    for (const char* name : { "h3ab_spr.lod",
                              "h3ab_ahd.vid",
                              "h3ab_ahd.snd",
                              "h3ab_bmp.lod" }) {
        auto path = findPathChild(dataFolder, name);
        if (path.empty())
            continue;

        result.m_hasAB = true;
        result.m_sources[SourceType::Archive].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = path, .m_isSod = true });
    }

    for (const char* name : { "h3bitmap.lod",
                              "h3sprite.lod",
                              "video.vid",
                              "heroes3.snd" }) {
        auto path = findPathChild(dataFolder, name);
        if (path.empty())
            continue;
        result.m_hasSod = true;
        result.m_sources[SourceType::Archive].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = path, .m_isSod = true });
    }

    for (const char* name : { "hota.snd",
                              "hota.lod",
                              "hota_lng.lod",
                              "hota.vid" }) {
        auto path = findPathChild(dataFolder, name);
        if (path.empty())
            continue;
        result.m_hasHota = true;
        result.m_sources[SourceType::Archive].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = path, .m_isHota = true });
    }
    for (const char* name : { "hota.dat" }) {
        auto path = findPathChild(result.m_heroesRoot, name);
        if (path.empty())
            continue;
        result.m_hasHota = true;
        result.m_sources[SourceType::Archive].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = path, .m_isHota = true });
    }

    {
        // D:\Games\Heroes3_HotA\_HD3_Data\Common\Fix.Cosmetic
        Mernel::std_path hdFolder = findPathChild(result.m_heroesRoot, "_hd3_data");
        if (!hdFolder.empty() && Mernel::std_fs::exists(hdFolder / "Common")) {
            result.m_hasHD  = true;
            auto commonPath = hdFolder / "Common";
            auto fixPath    = commonPath / "Fix.Cosmetic";
            result.m_sources[SourceType::DefCopy].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = commonPath, .m_isHD = true });
            if (Mernel::std_fs::exists(commonPath)) {
                result.m_sources[SourceType::DefCopy].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = fixPath, .m_isHD = true });
            }
        }
    }

    {
        Mernel::std_path mpFolder = findPathChild(result.m_heroesRoot, "mp3");
        if (!mpFolder.empty())
            result.m_sources[SourceType::MusicCopy].push_back(DetectedPath{ .m_type = SourceType::MusicCopy, .m_path = mpFolder, .m_isSod = true });
    }
    {
#ifndef DISABLE_QT
        QString ffmpegBinary;
        if (executeFFMpeg({ "-version" }, &ffmpegBinary)) {
            result.m_ffmpegPath = Gui::QString2stdPath(ffmpegBinary);
        }
#endif
    }

    return result;
}

void GameExtract::run(const DetectedSources& sources) const
{
    if (!sources.isSuccess())
        return;

    const DetectedPathList& archives = sources.m_sources.at(SourceType::Archive);
    //const DetectedPathList& defCopy  = sources.m_sources.at(SourceType::DefCopy);

    std::vector<ArchiveWrapper> archiveWrappers;
    archiveWrappers.resize(archives.size());

    bool hasHota = false;
    bool hasSod  = false;

    ScopeTimer timer;
    sendMessage("Extracting game archives...", true);
    sendMessage("Prepare extraction of archives, from " + Mernel::path2string(sources.m_heroesRoot)
                + "\n    to " + Mernel::path2string(m_settings.m_archiveExtractRoot));
    for (size_t i = 0; i < archiveWrappers.size(); ++i) {
        auto& wrapper         = archiveWrappers[i];
        wrapper.m_dat         = archives[i].m_path;
        wrapper.m_datFilename = Mernel::path2string(wrapper.m_dat.filename());
        wrapper.m_folderName  = Mernel::strToLower(wrapper.m_datFilename);
        wrapper.m_folderName.replace(wrapper.m_folderName.find('.'), 1, "_");
        wrapper.m_folder = m_settings.m_archiveExtractRoot / wrapper.m_folderName;

        wrapper.m_isSod  = archives[i].m_isSod;
        wrapper.m_isHota = archives[i].m_isHota;
        wrapper.m_isHD   = archives[i].m_isHD;

        hasHota = hasHota || wrapper.m_isHota;
        hasSod  = hasSod || wrapper.m_isSod;

        if (wrapper.m_isSod)
            wrapper.m_resourceModName = "sod_res.fhmod";
        if (wrapper.m_isHota)
            wrapper.m_resourceModName = "hota_res.fhmod";
        if (wrapper.m_isHD)
            wrapper.m_resourceModName = "hd_res.fhmod";

        if (Mernel::std_fs::exists(wrapper.m_folder)) {
            if (m_settings.m_skipIfFolderExist) {
                wrapper.m_doSkip = true;
                sendMessage("- " + wrapper.m_folderName + " : exists, SKIPPING");
            } else {
                wrapper.m_doExtract = true;
                sendMessage("- " + wrapper.m_folderName + " : exists, but forcing re-EXTRACT from " + wrapper.m_datFilename);
            }
        } else {
            wrapper.m_doExtract = true;
            sendMessage("- " + wrapper.m_folderName + " : not exists, EXTRACTING from " + wrapper.m_datFilename);
        }
        wrapper.m_converter = std::make_unique<ConversionHandler>(wrapper.m_converterLog, ConversionHandler::Settings{
                                                                                              .m_inputs            = { .m_datFile = wrapper.m_dat, .m_folder = wrapper.m_folder },
                                                                                              .m_outputs           = { .m_folder = wrapper.m_folder },
                                                                                              .m_forceWrite        = false,
                                                                                              .m_uncompressArchive = true,
                                                                                          });
    }
    const int total     = archiveWrappers.size();
    int       scheduled = 0;
    for (int current = 0; ArchiveWrapper & wrapper : archiveWrappers) {
        if (wrapper.m_doSkip) {
            m_onProgress(current++, total);
            continue;
        }
        scheduled++;
        try {
            m_onProgress(current++, total);
            if (wrapper.m_doExtract) {
                wrapper.m_converter->run(ConversionHandler::Task::ArchiveLoadDat);
                wrapper.m_converter->run(ConversionHandler::Task::ArchiveSaveFolder);
                //sendMessage(wrapper.m_datFilename + " extracted.");
            } else {
                wrapper.m_converter->run(ConversionHandler::Task::ArchiveLoadFolder);
                //sendMessage(wrapper.m_folderName + " loaded.");
            }
        }
        catch (std::exception& ex) {
            m_onError(wrapper.m_datFilename + " ERROR: " + ex.what() + "\n" + wrapper.m_converterLog.str());
            return;
        }
    }
    if (!scheduled)
        return;

    sendMessage("archives loaded in " + std::to_string(timer.elapsedUS()) + " us.");
    sendMessage("Converting data...", true);

    TaskQueue       taskQueue;
    ConcatProcessor concatProcessor;

    KnownResources             knownResources(m_settings.m_appResourcePath / "knownResources");
    std::set<Mernel::std_path> usedTasks;

    for (const ArchiveWrapper& wrapper : archiveWrappers) {
        if (wrapper.m_doSkip)
            continue;
        sendMessage(wrapper.m_folderName + " convert...");
        if (wrapper.m_resourceModName.empty())
            continue;

        const Mernel::std_path extractArchiveRoot = m_settings.m_mainExtractRoot / wrapper.m_resourceModName;

        for (const auto& file : wrapper.m_converter->m_archive->m_records) {
            const auto srcPath = wrapper.m_folder / file.fullname();
            processFile(taskQueue,
                        knownResources,
                        file.m_basename,
                        file.m_extWithDot,
                        srcPath,
                        extractArchiveRoot,
                        usedTasks,
                        !sources.m_ffmpegPath.empty(),
                        concatProcessor,
                        wrapper.m_isSod,
                        wrapper.m_isHota);
        }
    }
    if (sources.m_sources.contains(SourceType::DefCopy)) {
        const DetectedPathList& defCopy = sources.m_sources.at(SourceType::DefCopy);
        for (const auto& src : defCopy) {
            const auto     resourceModName    = "hd_res.fhmod";
            const std_path extractArchiveRoot = m_settings.m_mainExtractRoot / resourceModName;
            const auto     srcFolderPath      = src.m_path;
            for (auto&& it : std_fs::directory_iterator(srcFolderPath)) {
                if (!it.is_regular_file())
                    continue;
                const auto srcPath  = it.path();
                auto       ext      = pathToLower(srcPath.extension());
                auto       basename = pathToLower(srcPath.stem());

                processFile(taskQueue,
                            knownResources,
                            basename,
                            ext,
                            srcPath,
                            extractArchiveRoot,
                            usedTasks,
                            !sources.m_ffmpegPath.empty(),
                            concatProcessor,
                            false,
                            false);
            }
        }
    }

    if (sources.m_sources.contains(SourceType::MusicCopy)) {
        const DetectedPathList& mp3Copy = sources.m_sources.at(SourceType::MusicCopy);
        for (const auto& src : mp3Copy) {
            const auto     resourceModName    = "sod_res.fhmod";
            const std_path extractArchiveRoot = m_settings.m_mainExtractRoot / resourceModName;
            std_fs::create_directories(extractArchiveRoot);
            for (auto&& it : std_fs::directory_iterator(src.m_path)) {
                if (!it.is_regular_file())
                    continue;
                const auto srcPath  = it.path();
                auto       ext      = pathToLower(srcPath.extension());
                auto       basename = pathToLower(srcPath.stem());

                auto            destPath = extractArchiveRoot / (basename + ".mp3");
                std::error_code ec;
                std_fs::copy(srcPath, destPath, std_fs::copy_options::skip_existing, ec);
            }
        }
    }

    {
        ParallelExecutor executor(std::thread::hardware_concurrency());
        TaskQueueWatcher watcher(taskQueue);
        watcher.setProgressHandler(m_onProgress);

        executor.execQueue(taskQueue);

        Mernel::Logger(Mernel::Logger::Info) << "Loop finished, taskQueueSize:" << taskQueue.queueSize();
    }

    concatProcessor.create();

    if (!m_settings.m_needLocalization)
        return;

#ifndef DISABLE_QT
    LocalizationConverter loc(m_databaseContainer->getDatabase(Core::GameVersion::HOTA), m_databaseContainer->getDatabase(Core::GameVersion::SOD));

    if (hasSod)
        loc.extractSOD(m_settings.m_archiveExtractRoot / "h3bitmap_lod", m_settings.m_mainExtractRoot / "sod_res.fhmod" / "sod_tr_update.fhdb.json");
    if (hasHota) {
        loc.extractHOTA(m_settings.m_archiveExtractRoot / "hota_dat", m_settings.m_mainExtractRoot / "hota_res.fhmod" / "hota_tr_update.fhdb.json");
        if (std_fs::exists(m_settings.m_archiveExtractRoot / "hota_lng_lod")) {
            loc.extractSOD(m_settings.m_archiveExtractRoot / "hota_lng_lod", m_settings.m_mainExtractRoot / "hota_res.fhmod" / "hota_trlng_update.fhdb.json");
        }
    }
#endif
}

void GameExtract::processFile(Mernel::TaskQueue&          taskQueue,
                              KnownResources&             knownResources,
                              const std::string&          basename,
                              const std::string&          extWithDot,
                              const Mernel::std_path&     srcPath,
                              Mernel::std_path            extractFolder,
                              std::set<Mernel::std_path>& usedTasks,
                              bool                        hasFfmpeg,
                              ConcatProcessor&            concatProcessor,

                              bool isSod,
                              bool isHota) const
{
    KnownResources::ResourceType resourceType = KnownResources::ResourceType::Unknown;
    if (extWithDot == ".def" || extWithDot == ".d32" || extWithDot == ".pcx" || extWithDot == ".p32" || extWithDot == ".bmp")
        resourceType = KnownResources::ResourceType::Sprite;
    else if (extWithDot == ".wav")
        resourceType = KnownResources::ResourceType::Sound;
    else if (extWithDot == ".bik" || extWithDot == ".smk")
        resourceType = KnownResources::ResourceType::Video;

    auto* known    = knownResources.find(resourceType, basename);
    auto  handlers = knownResources.findPP(resourceType, basename);
    if (handlers.contains("skip"))
        return;

    std::string newId = basename;
    if (known) {
        newId = known->m_newId;
        if (newId.empty())
            newId = basename;
        extractFolder = extractFolder / known->m_destinationSubfolder;
    } else {
        extractFolder = extractFolder / "Unknown";
    }

    //sendMessage(file.m_extWithDot + " | " + file.m_extWithDot);
    if (resourceType == KnownResources::ResourceType::Sprite) {
        auto outputJson = extractFolder / (newId + ".fhsprite.json");
        if (Mernel::std_fs::exists(outputJson) && !m_settings.m_forceExtract)
            return;
        if (usedTasks.contains(outputJson))
            return;
        usedTasks.insert(outputJson);

        const std::vector<std::string> names{ "fix_colors", "unpack", "pad" };
        for (const auto& name : names) {
            if (handlers.contains(name + "_hota")) {
                if (isHota)
                    handlers[name] = handlers[name + "_hota"];

                handlers.getMap().erase(name + "_hota");
            }
            if (handlers.contains(name + "_sod")) {
                if (isSod)
                    handlers[name] = handlers[name + "_sod"];

                handlers.getMap().erase(name + "_sod");
            }
        }

        taskQueue.addTask([this, outputJson, handlers, srcPath, &concatProcessor] {
            ConversionHandler::Settings sett{
                .m_inputs = { .m_defFile = srcPath },
            };
            std::ostringstream os;
            ConversionHandler  pixHandler(os, sett);

            try {
                pixHandler.run(ConversionHandler::Task::SpriteLoadDef);
                if (!handlers.contains("animatePalette"))
                    pixHandler.m_sprite->setEmbeddedData(false, true);

                pixHandler.m_sprite->saveGuiSprite(outputJson, handlers);
                if (handlers.contains("concat")) {
                    auto& c      = handlers["concat"];
                    auto  single = c.contains("singleGroup") ? c["singleGroup"].getScalar().toBool() : false;
                    concatProcessor.addSprite(*pixHandler.m_sprite,
                                              outputJson,
                                              c["id"].getScalar().toString(),
                                              c["frames"].getScalar().toBool(),
                                              single);
                }
            }
            catch (std::exception& ex) {
                sendError(std::string(" ERROR: ") + ex.what() + "\n" + os.str());
                return;
            }
        });
    }
#ifndef DISABLE_QT
    else if (resourceType == KnownResources::ResourceType::Sound) {
        auto outputWav = extractFolder / (newId + ".wav");
        if (!hasFfmpeg)
            return;
        if (std_fs::exists(outputWav))
            return;
        std_fs::create_directories(outputWav.parent_path());
        taskQueue.addTask([srcPath, outputWav] {
            executeFFMpeg({ "-y", "-i", Gui::stdPath2QString(srcPath), "-c:a", "pcm_s16le", Gui::stdPath2QString(outputWav) });
        });
    } else if (resourceType == KnownResources::ResourceType::Video) {
        auto outputWebp = extractFolder / (newId + ".webp");
        if (!hasFfmpeg)
            return;
        if (std_fs::exists(outputWebp))
            return;
        std_fs::create_directories(outputWebp.parent_path());
        taskQueue.addTask([srcPath, outputWebp] {
            executeFFMpeg({ "-y", "-i", Gui::stdPath2QString(srcPath), "-threads", "1", Gui::stdPath2QString(outputWebp) });
        });
    }
#endif
}
}
