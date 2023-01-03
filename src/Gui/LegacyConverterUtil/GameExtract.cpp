/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameExtract.hpp"

#include "EnvDetect.hpp"

#include "ConversionHandler.hpp"
#include "ThreadPoolExecutor.hpp"
#include "MernelPlatform/AppLocations.hpp"

#include "Archive.hpp"
#include "SpriteFile.hpp"
#include "KnownResources.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <sstream>

#include <QEventLoop>
#include <QImage>
#include <QPainter>

namespace FreeHeroes {
using namespace Mernel;

struct ArchiveWrapper {
    std::ostringstream                 m_converterLog;
    std::unique_ptr<ConversionHandler> m_converter;

    Mernel::std_path m_dat;
    Mernel::std_path m_folder;
    std::string      m_datFilename;
    std::string      m_folderName;
    std::string      m_resourceModName;

    bool m_doExtract = false;
    bool m_required  = false;

    bool m_isSod  = false;
    bool m_isHota = false;
    bool m_isHD   = false;
};

namespace {

Mernel::std_path findPathChild(const Mernel::std_path& parent, const std::string& lowerCaseName)
{
    if (parent.empty())
        return {};
    for (auto&& it : Mernel::std_fs::directory_iterator(parent)) {
        if (it.is_regular_file() || it.is_directory()) {
            auto name = Mernel::path2string(it.path().filename());
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
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

class ConcatProcessor {
public:
    struct ConcatData {
        std::map<std_path, SpriteFile> m_sprites;
        bool                           m_makeFrames = false;
        //std_path m_someSpritePath;
    };
    std::map<std::string, ConcatData> m_data;
    std::mutex                        m_mutex;

    void addSprite(const SpriteFile& sprite, const std_path& path, const std::string& concatId, bool makeFrames)
    {
        std::unique_lock lock(m_mutex);

        auto& data           = m_data[concatId];
        data.m_makeFrames    = makeFrames;
        data.m_sprites[path] = sprite;
    }

    void create()
    {
        for (const auto& [id, concatData] : m_data) {
            std::vector<QPixmap> pixmaps;
            pixmaps.reserve(concatData.m_sprites.size());
            for (const auto& [path, sprite] : concatData.m_sprites) {
                pixmaps.push_back(*sprite.m_bitmaps[0].m_pixmapQt.get());
            }
            const auto root = concatData.m_sprites.begin()->first.parent_path();
            SpriteFile outFile;
            if (concatData.m_makeFrames) {
                outFile.m_embeddedBitmapData = false;
                std::vector<BitmapFile> bmps;
                for (auto& pix : pixmaps) {
                    BitmapFile bmp;
                    bmp.m_pixmapQt = std::make_shared<QPixmap>(pix);
                    bmps.push_back(std::move(bmp));
                }
                outFile.fromPixmapList(bmps);
            } else {
                QSize size(0, 0);
                for (auto& pix : pixmaps) {
                    size.setWidth(std::max(size.width(), pix.width()));
                    size.setHeight(size.height() + pix.height());
                }
                QImage result(size.width(), size.height(), QImage::Format_RGBA8888);
                QPoint offset(0, 0);
                for (auto& pix : pixmaps) {
                    QPainter p(&result);
                    p.drawPixmap(offset, pix);
                    offset += QPoint(0, pix.height());
                }
                BitmapFile bmp;
                bmp.m_pixmapQt = std::make_shared<QPixmap>(QPixmap::fromImage(result));

                outFile.fromPixmap(std::move(bmp));
            }
            outFile.saveGuiSprite(root / (id + ".fhsprite.json"), {});
        }
    }
};

}

GameExtract::GameExtract(const Core::IGameDatabaseContainer* databaseContainer, Settings settings)
    : m_databaseContainer(databaseContainer)
    , m_settings(std::move(settings))
{
}

GameExtract::~GameExtract() = default;

GameExtract::DetectedSources GameExtract::probe() const
{
    DetectedSources result;
    result.m_heroesRoot = m_settings.m_heroesRoot;
    if (result.m_heroesRoot.empty()) {
        result.m_heroesRoot = findHeroes3Installation();
    }
    std::error_code ec;
    if (result.m_heroesRoot.empty() || !Mernel::std_fs::exists(result.m_heroesRoot, ec))
        return result;

    sendMessage("Probing path:" + Mernel::path2string(result.m_heroesRoot));

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
        result.m_sources[SourceType::Archive].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = path });
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
                              "hota_lng.lod"
                              "hota.vid",
                              "video.vid" }) {
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
        wrapper.m_folderName  = wrapper.m_datFilename;
        std::transform(wrapper.m_folderName.begin(), wrapper.m_folderName.end(), wrapper.m_folderName.begin(), [](unsigned char c) { return std::tolower(c); });
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
            if (m_settings.m_forceExtract) {
                wrapper.m_doExtract = true;
                sendMessage("- " + wrapper.m_folderName + " : exists, but forcing re-EXTRACT from " + wrapper.m_datFilename);
            } else {
                sendMessage("- " + wrapper.m_folderName + " : exists, SKIPPING");
            }
        } else {
            wrapper.m_doExtract = true;
            sendMessage("- " + wrapper.m_folderName + " : not exists, EXTRACTING from " + wrapper.m_datFilename);
        }
        wrapper.m_converter = std::make_unique<ConversionHandler>(wrapper.m_converterLog, ConversionHandler::Settings{
                                                                                              .m_inputs            = { .m_datFile = wrapper.m_dat, .m_folder = wrapper.m_folder },
                                                                                              .m_outputs           = { .m_folder = wrapper.m_folder },
                                                                                              .m_forceWrite        = true,
                                                                                              .m_uncompressArchive = true,
                                                                                          });
    }
    const int total = archiveWrappers.size();
    for (int current = 0; ArchiveWrapper & wrapper : archiveWrappers) {
        try {
            sendProgress(current++, total);
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

    sendMessage("archives loaded in " + std::to_string(timer.elapsed()) + " us.");
    sendMessage("Converting data...", true);

    ThreadPoolExecutor executorMain;
    ConcatProcessor    concatProcessor;

    const auto appRoot = AppLocations("").getBinDir();

    KnownResources knownResources(appRoot / "gameResources" / "knownResources.json", appRoot / "gameResources" / "knownResourcesPostProcess.json");

    for (const ArchiveWrapper& wrapper : archiveWrappers) {
        sendMessage(wrapper.m_folderName + " convert...");
        if (wrapper.m_resourceModName.empty())
            continue;

        const Mernel::std_path extractArchiveRoot = m_settings.m_mainExtractRoot / wrapper.m_resourceModName;

        for (const auto& file : wrapper.m_converter->m_archive->m_records) {
            //sendMessage(file.m_extWithDot + " | " + file.m_extWithDot);
            if (file.m_extWithDot == ".def" || file.m_extWithDot == ".d32" || file.m_extWithDot == ".pcx") {
                auto* known    = knownResources.find(file.m_basename);
                auto  handlers = knownResources.findPP(file.m_basename);
                if (handlers.contains("skip"))
                    continue;
                auto        extractFolder = extractArchiveRoot;
                std::string newId         = file.m_basename;
                if (known) {
                    newId         = known->newId;
                    extractFolder = extractFolder / known->destinationSubfolder;
                }

                auto outputJson = extractFolder / (newId + ".fhsprite.json");
                if (Mernel::std_fs::exists(outputJson))
                    continue;

                const std::vector<std::string> names{ "make_transparent", "unpack" };
                for (const auto& name : names) {
                    if (handlers.contains(name + "_hota")) {
                        if (wrapper.m_isHota)
                            handlers[name] = handlers[name + "_hota"];

                        handlers.getMap().erase(name + "_hota");
                    }
                    if (handlers.contains(name + "_sod")) {
                        if (wrapper.m_isSod)
                            handlers[name] = handlers[name + "_sod"];

                        handlers.getMap().erase(name + "_sod");
                    }
                }

                executorMain.add([this, &wrapper, outputJson, handlers, file, &concatProcessor] {
                    ConversionHandler::Settings sett{
                        .m_inputs = { .m_defFile = wrapper.m_folder / file.fullname() },
                    };
                    std::ostringstream os;
                    ConversionHandler  pixHandler(os, sett);

                    try {
                        pixHandler.run(ConversionHandler::Task::SpriteLoadDef);
                        if (!handlers.contains("animatePalette"))
                            pixHandler.m_sprite->setEmbeddedData(false, true);
                        pixHandler.m_sprite->saveGuiSprite(outputJson, handlers);
                        if (handlers.contains("concat")) {
                            concatProcessor.addSprite(*pixHandler.m_sprite,
                                                      outputJson,
                                                      handlers["concat"]["id"].getScalar().toString(),
                                                      handlers["concat"]["frames"].getScalar().toBool());
                        }
                    }
                    catch (std::exception& ex) {
                        sendError(wrapper.m_datFilename + " ERROR: " + ex.what() + "\n" + os.str());
                        return;
                    }
                });
            }
        }
    }

    {
        const int  totalCC = executorMain.getQueueSize();
        QEventLoop loop;
        QObject::connect(&executorMain, &ThreadPoolExecutor::finished, &loop, &QEventLoop::quit);
        QObject::connect(&executorMain, &ThreadPoolExecutor::progress, [this, totalCC](int done) { sendProgress(done, totalCC); });
        executorMain.start(std::chrono::milliseconds{ 100 });

        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }

    concatProcessor.create();
}

}
