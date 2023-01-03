/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameDatabaseContainer.hpp"

#include "GameDatabase.hpp"

#include "JsonHelper.hpp"
#include "MernelPlatform/Logger.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"

#include "IResourceLibrary.hpp"

namespace FreeHeroes::Core {
using namespace Mernel;
struct Resource {
    Mernel::PropertyTree m_jsonData;
    std::string          m_filename;
};

struct GameDatabaseContainer::Impl {
    const IResourceLibrary* const m_resourceLibrary = nullptr;

    Impl(const IResourceLibrary* resourceLibrary)
        : m_resourceLibrary(resourceLibrary)
    {}

    struct DbSegmentRecord {
        bool         m_isValid = false;
        PropertyTree m_data;
    };

    std::map<std::string, DbSegmentRecord> m_dbSegmentFiles;

    struct DbIndexRecord {
        bool         m_isValid = false;
        PropertyTree m_data;
    };

    std::map<std::string, DbIndexRecord> m_dbIndexFiles;

    struct DbObjectRecord {
        bool                          m_isValid = false;
        PropertyTree                  m_data;
        std::shared_ptr<GameDatabase> m_db;
    };

    std::map<IGameDatabaseContainer::DbOrder, DbObjectRecord> m_dbObjects;

    // @todo: well, that's a bit of leak. Probably need a cleanup strategy for those.
    // hash(PropertyTree) ???
    std::list<std::shared_ptr<const IGameDatabase>> m_patchedStorage;

    bool loadDbSegmentFile(const std::string& dbSegmentId) noexcept
    {
        const bool       isInCache = m_dbSegmentFiles.contains(dbSegmentId);
        DbSegmentRecord& rec       = m_dbSegmentFiles[dbSegmentId];
        if (isInCache)
            return rec.m_isValid;

        try {
            auto path = m_resourceLibrary->get(ResourceType::DbSegment, dbSegmentId);
            if (path.empty()) {
                Logger(Logger::Err) << "Non-existent DB index id '" << dbSegmentId << "': ";
                return false;
            }
            const auto fileBuffer = readFileIntoBufferThrow(path);
            const auto jsonData   = readJsonFromBufferThrow(fileBuffer);

            rec.m_data.convertToMap();
            const int totalRecordsFound = addJsonObjectToIndex(rec.m_data, jsonData);
            Logger(Logger::Info) << "Database segment JSON parsing finished '" << dbSegmentId << "', total records:" << totalRecordsFound;
        }
        catch (std::exception& ex) {
            Logger(Logger::Err) << "error while loading database segment '" << dbSegmentId << "': " << ex.what();
            return false;
        }

        rec.m_isValid = true;
        return true;
    }

    bool loadDbIndexFile(const std::string& dbIndexId) noexcept
    {
        const bool     isInCache = m_dbIndexFiles.contains(dbIndexId);
        DbIndexRecord& rec       = m_dbIndexFiles[dbIndexId];
        if (isInCache)
            return rec.m_isValid;

        try {
            auto path = m_resourceLibrary->get(ResourceType::DbIndex, dbIndexId);
            if (path.empty()) {
                Logger(Logger::Err) << "Non-existent DB index id '" << dbIndexId << "': ";
                return false;
            }
            const auto fileBuffer = readFileIntoBufferThrow(path);
            const auto jsonData   = readJsonFromBufferThrow(fileBuffer);
            for (const auto& item : jsonData["segments"].getList()) {
                const std::string segmentId = item.getScalar().toString();
                if (!loadDbSegmentFile(segmentId))
                    return false;

                auto& segmentJson = m_dbSegmentFiles[segmentId].m_data;
                PropertyTree::mergePatch(rec.m_data, segmentJson);
            }
        }
        catch (std::exception& ex) {
            Logger(Logger::Err) << "error while loading database index '" << dbIndexId << "': " << ex.what();
            return false;
        }

        rec.m_isValid = true;
        return true;
    }

    const IGameDatabase* getDb(const IGameDatabaseContainer::DbOrder& key) noexcept
    {
        const bool      isInCache = m_dbObjects.contains(key);
        DbObjectRecord& rec       = m_dbObjects[key];
        if (isInCache)
            return rec.m_isValid ? rec.m_db.get() : nullptr;

        try {
            for (const std::string& dbIndexId : key) {
                if (!loadDbIndexFile(dbIndexId))
                    return nullptr;
                auto& indexJson = m_dbIndexFiles[dbIndexId].m_data;

                PropertyTree::mergePatch(rec.m_data, indexJson);
            }
            rec.m_db = std::make_shared<GameDatabase>(rec.m_data);
        }
        catch (std::exception& ex) {
            Logger(Logger::Err) << "error while loading database seqence '" << key << "': " << ex.what();
            return nullptr;
        }
        rec.m_isValid = true;
        return rec.m_db.get();
    }

    const IGameDatabase* getDbWithPatch(const IGameDatabaseContainer::DbOrder& key, const PropertyTree& patch) noexcept
    {
        try {
            PropertyTree data;
            for (const std::string& dbIndexId : key) {
                if (!loadDbIndexFile(dbIndexId))
                    return nullptr;
                auto& indexJson = m_dbIndexFiles[dbIndexId].m_data;

                PropertyTree::mergePatch(data, indexJson);
            }
            PropertyTree::mergePatch(data, patch);
            m_patchedStorage.push_back(std::make_shared<GameDatabase>(data));
            return m_patchedStorage.back().get();
        }
        catch (std::exception& ex) {
            Logger(Logger::Err) << "error while loading database seqence '" << key << "': " << ex.what();
            return nullptr;
        }
    }
};

GameDatabaseContainer::GameDatabaseContainer(const IResourceLibrary* resourceLibrary)
    : m_impl(std::make_unique<Impl>(resourceLibrary))
{
}

GameDatabaseContainer::~GameDatabaseContainer() = default;

const IGameDatabase* GameDatabaseContainer::getDatabase(const DbOrder& dbIndexFilesList) const noexcept
{
    return m_impl->getDb(dbIndexFilesList);
}

const IGameDatabase* GameDatabaseContainer::getDatabase(const DbOrder& dbIndexFilesList, const PropertyTree& customSegmentData) const noexcept
{
    return m_impl->getDbWithPatch(dbIndexFilesList, customSegmentData);
}

}
