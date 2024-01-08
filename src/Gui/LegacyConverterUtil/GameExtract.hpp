/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <map>
#include <functional>
#include <mutex>

namespace Mernel {
class TaskQueue;
}
namespace FreeHeroes {
namespace Core {
class IGameDatabaseContainer;
}
class KnownResources;
class LEGACYCONVERTERUTIL_EXPORT GameExtract {
public:
    struct Settings {
        Mernel::std_path m_appResourcePath; // required path containing knownResources.json
        Mernel::std_path m_heroesRoot;
        Mernel::std_path m_archiveExtractRoot;
        Mernel::std_path m_mainExtractRoot;
        bool             m_forceExtract      = false;
        bool             m_skipIfFolderExist = false;
        bool             m_needLocalization  = true;
    };

    enum class SourceType
    {
        Archive,
        MusicCopy,
        DefCopy
    };

    struct DetectedPath {
        SourceType       m_type = SourceType::Archive;
        Mernel::std_path m_path;

        bool m_isSod  = false;
        bool m_isHota = false;
        bool m_isHD   = false;
    };
    using DetectedPathList = std::vector<DetectedPath>;

    struct DetectedSources {
        std::map<SourceType, DetectedPathList> m_sources;

        Mernel::std_path m_heroesRoot;

        Mernel::std_path m_ffmpegPath;

        bool m_hasSod  = false; // has one of known SoD files
        bool m_hasHota = false; // has one of known HotA files

        bool m_hasAB = false; // has one of known Armageddon Blade files
        bool m_hasHD = false; // has one of known HD Mod files

        bool isSuccess() const { return m_hasSod || m_hasHota; }
    };

    using ProgressCallBack = std::function<void(int progress, int total)>;
    using ErrorCallBack    = std::function<void(const std::string& error)>;
    using MessageCallBack  = std::function<void(const std::string& msg, bool important)>;

    GameExtract(const Core::IGameDatabaseContainer* databaseContainer, Settings settings);
    ~GameExtract();

    DetectedSources probe() const;

    void run(const DetectedSources& sources) const;

    void setProgressCallback(ProgressCallBack callback) { m_onProgress = callback; }
    void setErrorCallback(ErrorCallBack callback) { m_onError = callback; }
    void setMessageCallback(MessageCallBack callback) { m_onMessage = callback; }

    void sendMessage(const std::string& msg, bool important = false) const
    {
        if (!m_onMessage)
            return;
        std::lock_guard lock(m_messageMutex);
        m_onMessage(msg, important);
    }
    void sendError(const std::string& msg) const
    {
        if (!m_onError)
            return;
        std::lock_guard lock(m_messageMutex);
        m_onError(msg);
    }
    class ConcatProcessor;

    void processFile(Mernel::TaskQueue&      taskQueue,
                     KnownResources&         knownResources,
                     const std::string&      basename,
                     const std::string&      extWithDot,
                     const Mernel::std_path& srcPath,
                     Mernel::std_path        extractFolder,
                     bool                    hasFfmpeg,
                     ConcatProcessor&        concatProcessor,
                     bool                    isSod,
                     bool                    isHota) const;

private:
    const Core::IGameDatabaseContainer* const m_databaseContainer;
    const Settings                            m_settings;
    ProgressCallBack                          m_onProgress;
    ErrorCallBack                             m_onError;
    MessageCallBack                           m_onMessage;
    mutable std::mutex                        m_messageMutex;
};

}
