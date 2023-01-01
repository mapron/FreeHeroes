/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <map>
#include <functional>

namespace FreeHeroes {
namespace Core {
class IGameDatabaseContainer;
}

class LEGACYCONVERTERUTIL_EXPORT GameExtract {
public:
    struct Settings {
        Core::std_path m_heroesRoot;
        Core::std_path m_archiveExtractRoot;
        Core::std_path m_mainExtractRoot;
        Core::std_path m_knownResourcesFile;
        bool           m_forceExtract = false;
    };

    enum class SourceType
    {
        Archive,
        MusicCopy,
        DefCopy
    };

    struct DetectedPath {
        SourceType     m_type = SourceType::Archive;
        Core::std_path m_path;

        bool m_isSod  = false;
        bool m_isHota = false;
    };
    using DetectedPathList = std::vector<DetectedPath>;

    struct DetectedSources {
        std::map<SourceType, DetectedPathList> m_sources;

        Core::std_path m_heroesRoot;

        bool m_hasSod  = false; // has one of known SoD files
        bool m_hasHota = false; // has one of known HotA files

        bool m_hasAB = false; // has one of known Armageddon Blade files
        bool m_hasHD = false; // has one of known HD Mod files

        bool isSuccess() const { return m_hasSod || m_hasHota; }
    };

    using ProgressCallBack = std::function<bool(int progress, int total)>;
    using ErrorCallBack    = std::function<void(const std::string& error)>;
    using MessageCallBack  = std::function<void(const std::string& msg, bool important)>;

    GameExtract(const Core::IGameDatabaseContainer* databaseContainer, Settings settings);
    ~GameExtract();

    DetectedSources probe() const;

    void run(const DetectedSources& sources) const;

    void setProgressCallback(ProgressCallBack callback) { m_onProgress = callback; }
    void setErrorCallback(ErrorCallBack callback) { m_onError = callback; }
    void setMessageCallback(MessageCallBack callback) { m_onMessage = callback; }

    void sendProgress(int progress, int total) const
    {
        if (!m_onProgress)
            return;
        m_onProgress(progress, total);
    }

    void sendMessage(const std::string& msg, bool important = false) const
    {
        if (!m_onMessage)
            return;
        m_onMessage(msg, important);
    }
    void sendError(const std::string& msg) const
    {
        if (!m_onError)
            return;
        m_onError(msg);
    }

private:
    const Core::IGameDatabaseContainer* const m_databaseContainer;
    const Settings                            m_settings;
    ProgressCallBack                          m_onProgress;
    ErrorCallBack                             m_onError;
    MessageCallBack                           m_onMessage;
};

}
