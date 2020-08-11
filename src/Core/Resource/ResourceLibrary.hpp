/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IResourceLibrary.hpp"

#include "CoreResourceExport.hpp"

#include <map>
#include <unordered_map>
#include <memory>

#include <deque>

namespace FreeHeroes::Core {


class CORERESOURCE_EXPORT ResourceLibrary : public IResourceLibrary {
public:
    ResourceLibrary() = default;
    ~ResourceLibrary() = default;
    ResourceLibrary(const ResourceLibrary&) = delete;
    ResourceLibrary& operator=(const ResourceLibrary&) = delete;
    ResourceLibrary(ResourceLibrary&&) noexcept = delete;
    ResourceLibrary& operator=(ResourceLibrary&&) noexcept = delete;

    explicit ResourceLibrary(std::string id);

    enum class DepsRequire { Hard, Soft };

    struct Dep {
        std::string id;
        DepsRequire require = DepsRequire::Hard;
    };
    using DepList = std::vector<Dep>;

    struct ResourceLibraryPathList {
        struct Record {
            std_path path;
            std::string id;
            DepList deps;
        };
        std::vector<Record> records;
        void append(const ResourceLibraryPathList & another) {
            records.insert(records.end(), another.records.cbegin(), another.records.cend());
        }
        CORERESOURCE_EXPORT void topoSort();
    };

    static ResourceLibraryPathList searchIndexInFolderRecursive(const std_path & folder);
    static std::shared_ptr<ResourceLibrary> makeMergedLibrary(const ResourceLibraryPathList & sortedPaths);

    void setIndexFolder(const std_path & folder);
    void addDep(std::string dep, DepsRequire require = DepsRequire::Hard);

    bool loadIndex(bool onlyMeta = false);
    bool saveIndex();

public:
    bool mediaExists(ResourceMedia::Type type, const std::string & id) const noexcept override;
    const ResourceMedia & getMedia(ResourceMedia::Type type, const std::string & id) const override;

    void registerResource(ResourceMedia desc) override;

    std::vector<std::string> getTranslationContextChildren(const std::string & localeId, const std::string & contextId) const override;
    std::vector<std::string> getTranslation(const std::string & localeId,  const std::string & contextId,  const std::string & id) const override;
    void registerResource(ResourceTranslation desc) override;

    std::vector<std::string> getDatabaseIds() const override;
    const ResourceDatabase & getDatabase(const std::string & id) const override;
    void registerResource(ResourceDatabase desc) override;




protected:
    void mergeWith(ResourceLibrary & newData);
    std::string_view deepCopy(const std::string_view & data, bool ensureSlash = false);
    std::vector<std::string_view> deepCopy(const std::vector<std::string_view> & data);
    std::string_view makeNewString(std::string data);
   // std::string_view makeNewFullPath(std::string_view root, std::string_view subdir, std::string_view mainFilename);
    std::string_view ensureTrailingSlashOrEmpty(std::string_view path);
private:

    // @todo: waiting for C++2a heterogeneous lookup.
    using IdMappingMedia       = std::unordered_map<std::string_view, ResourceMedia>;
    using TypeMappingMedia     = std::map<ResourceMedia::Type, IdMappingMedia>;

    using IdMappingTrans       = std::unordered_map<std::string_view, ResourceTranslation>;
    using ContextMappingTrans  = std::unordered_map<std::string_view, IdMappingTrans>;
    using LocaleMappingTrans   = std::unordered_map<std::string_view, ContextMappingTrans>;

    using IdMappingDatabase    = std::unordered_map<std::string_view, ResourceDatabase>;

    const std::string m_id = "";
    DepList m_dependencies;

    TypeMappingMedia m_media;
    LocaleMappingTrans m_translations;
    IdMappingDatabase m_databases;
    std_path m_indexFolder;

    struct MemoryStorage {
        std::deque<std::string> m_strStorage;
        std::vector<char> m_buffer;
        std::string m_rootDir;
    };
    std::deque<MemoryStorage> m_storages{ 1, MemoryStorage()};
    MemoryStorage & m_mainStorage = m_storages[0];

};

}
