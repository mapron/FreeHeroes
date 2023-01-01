/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameExtract.hpp"

#include "EnvDetect.hpp"

#include "ConversionHandler.hpp"

#include "Archive.hpp"

#include "Profiler.hpp"

#include <sstream>

namespace FreeHeroes {

struct ArchiveWrapper {
    std::ostringstream                 m_converterLog;
    std::unique_ptr<ConversionHandler> m_converter;

    Core::std_path m_dat;
    Core::std_path m_folder;
    std::string    m_datFilename;
    std::string    m_folderName;

    bool m_doExtract = false;
    bool m_required  = false;
};

namespace {

Core::std_path findPathChild(const Core::std_path& parent, const std::string& lowerCaseName)
{
    if (parent.empty())
        return {};
    for (auto&& it : Core::std_fs::directory_iterator(parent)) {
        if (it.is_regular_file() || it.is_directory()) {
            auto name = Core::path2string(it.path().filename());
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
            if (name == lowerCaseName) {
                std::error_code ec;
                if (it.is_regular_file()) {
                    auto size = Core::std_fs::file_size(it.path(), ec);
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
    if (result.m_heroesRoot.empty() || !Core::std_fs::exists(result.m_heroesRoot, ec))
        return result;

    sendMessage("Probing path:" + Core::path2string(result.m_heroesRoot));

    Core::std_path dataFolder  = findPathChild(result.m_heroesRoot, "data"),
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
        Core::std_path hdFolder = findPathChild(result.m_heroesRoot, "_hd3_data");
        if (!hdFolder.empty() && Core::std_fs::exists(hdFolder / "Common")) {
            result.m_hasHD  = true;
            auto commonPath = hdFolder / "Common";
            auto fixPath    = commonPath / "Fix.Cosmetic";
            result.m_sources[SourceType::DefCopy].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = commonPath });
            if (Core::std_fs::exists(commonPath)) {
                result.m_sources[SourceType::DefCopy].push_back(DetectedPath{ .m_type = SourceType::Archive, .m_path = fixPath });
            }
        }
    }

    {
        Core::std_path mpFolder = findPathChild(result.m_heroesRoot, "mp3");
        if (!mpFolder.empty())
            result.m_sources[SourceType::MusicCopy].push_back(DetectedPath{ .m_type = SourceType::MusicCopy, .m_path = mpFolder });
    }

    return result;
}

void GameExtract::run(const DetectedSources& sources) const
{
    if (!sources.isSuccess())
        return;

    const DetectedPathList& archives = sources.m_sources.at(SourceType::Archive);

    std::vector<ArchiveWrapper> archiveWrappers;
    archiveWrappers.resize(archives.size());

    ScopeTimer timer;
    sendMessage("Extracting game archives...", true);
    sendMessage("Prepare extraction of archives, from " + Core::path2string(sources.m_heroesRoot)
                + "\n    to " + Core::path2string(m_settings.m_archiveExtractRoot));
    for (size_t i = 0; i < archiveWrappers.size(); ++i) {
        auto& wrapper         = archiveWrappers[i];
        wrapper.m_dat         = archives[i].m_path;
        wrapper.m_datFilename = Core::path2string(wrapper.m_dat.filename());
        wrapper.m_folderName  = wrapper.m_datFilename;
        std::transform(wrapper.m_folderName.begin(), wrapper.m_folderName.end(), wrapper.m_folderName.begin(), [](unsigned char c) { return std::tolower(c); });
        wrapper.m_folderName.replace(wrapper.m_folderName.find('.'), 1, "_");
        wrapper.m_folder = m_settings.m_archiveExtractRoot / wrapper.m_folderName;

        if (Core::std_fs::exists(wrapper.m_folder)) {
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

    for (ArchiveWrapper& wrapper : archiveWrappers) {
        sendMessage(wrapper.m_folderName + " convert...");
        for (const auto& file : wrapper.m_converter->m_archive->m_records) {
            //sendMessage(file.m_extWithDot + " | " + file.m_extWithDot);
            if (file.m_extWithDot == ".def") {
                std::ostringstream os;
                auto               extractFolder = m_settings.m_mainExtractRoot / wrapper.m_folderName;
                auto               outputJson    = extractFolder / (file.m_basename + ".json");
                if (Core::std_fs::exists(outputJson))
                    continue;
                ConversionHandler::Settings sett{
                    .m_inputs              = { .m_defFile = wrapper.m_folder / file.fullname() },
                    .m_outputs             = { .m_pngJsonFile = outputJson },
                    .m_prettyJson          = true,
                    .m_mergePng            = true,
                    .m_transparentKeyColor = true,
                };
                ConversionHandler pixHandler(os, sett);

                try {
                    pixHandler.run(ConversionHandler::Task::SpriteLoadDef);
                    pixHandler.run(ConversionHandler::Task::SpriteSaveUI);
                }
                catch (std::exception& ex) {
                    m_onError(wrapper.m_datFilename + " ERROR: " + ex.what() + "\n" + os.str());
                    return;
                }
            }
        }
    }
}

}
