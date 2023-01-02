/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ResourceLibrary.hpp"

#include "MernelPlatform/StringUtils.hpp"
#include "MernelPlatform/Profiler.hpp"
#include "MernelPlatform/Logger.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <list>

#include <cassert>

namespace FreeHeroes::Core {
using namespace Mernel;

namespace {
const std::string extension1 = ".txt";
const std::string extension2 = ".fhindex";
const std::string fullSuffix = extension2 + extension1;

const std::string typeDatabase    = "database";
const std::string typeTranslation = "translation";

enum class ParseMode
{
    Undefined,
    Media,
    Database,
    Translation
};

std::vector<std::string> getStringsFromViews(const std::vector<std::string_view>& arr)
{
    std::vector<std::string> result;
    result.reserve(arr.size());
    for (auto& s : arr)
        result.push_back(std::string{ s });
    return result;
}

const std::map<ResourceMedia::Type, std::string> mediaTypeMapping{
    { ResourceMedia::Type::Sprite, "sprites" },
    { ResourceMedia::Type::Sound, "sounds" },
    { ResourceMedia::Type::Music, "music" },
    { ResourceMedia::Type::Video, "video" },
    { ResourceMedia::Type::Other, "other" },
};

class Graph {
    std::map<std::string, ResourceLibrary::DepList> nodes;
    std::map<std::string, bool>                     visited;
    std::map<std::string, bool>                     onStack;
    std::vector<std::string>                        sortResult;

    void dfs(const std::string& node, bool optional)
    {
        if (nodes.count(node) == 0) {
            if (optional)
                return;
            throw std::runtime_error("Invalid dependency found:" + node);
        }
        visited[node] = true;
        onStack[node] = true;

        for (const ResourceLibrary::Dep& neighbour : nodes[node]) {
            if (visited[neighbour.id] && onStack[neighbour.id])
                throw std::runtime_error("Loop detected in resource dependencies");

            if (!visited[neighbour.id])
                dfs(neighbour.id, neighbour.require == ResourceLibrary::DepsRequire::Soft);
        }

        onStack[node] = false;
        sortResult.push_back(node);
    }

public:
    Graph() = default;
    void add(std::string id, ResourceLibrary::DepList deps)
    {
        nodes[std::move(id)] = std::move(deps);
    }

    void sort()
    {
        for (auto& nodeP : nodes) {
            if (!visited[nodeP.first])
                dfs(nodeP.first, false);
        }
    }
    const std::vector<std::string>& getSortResult() const { return sortResult; }
};

class FastCsvTable {
    const char* begin;
    const char* end;
    const char* curr;

public:
    FastCsvTable(const char* begin, size_t length)
    {
        this->begin = begin;
        this->curr  = begin;
        this->end   = begin + length;
    }
    bool scanLine()
    {
        if (curr >= end)
            return false;

        const char* peek = curr;
        size_t      tabs = 0;
        while (peek < end) {
            if (*peek == '\t')
                tabs++;
            if (*peek == '\r' || *peek == '\n')
                break;
            peek++;
        }
        const char* i       = curr;
        const char* lineEnd = peek;
        line                = std::string_view(curr, peek - curr);
        if (*peek == '\r')
            ++peek;
        if (*peek == '\n')
            ++peek;

        curr = peek;
        row.resize(tabs + 1);
        size_t      index = 0;
        const char* prevI = i;
        while (i < lineEnd) {
            if (*i == '\t') {
                row[index] = i == prevI ? std::string_view() : std::string_view(prevI, i - prevI);
                prevI      = i + 1;
                index++;
            }
            i++;
        }
        row[index] = i == prevI ? std::string_view() : std::string_view(prevI, i - prevI);

        return true;
    }

    std::vector<std::string_view> row;
    std::string_view              line;
};

}

ResourceLibrary::ResourceLibrary(std::string id)
    : m_id(id)
{
}

ResourceLibrary::ResourceLibraryPathList ResourceLibrary::searchIndexInFolderRecursive(const std_path& folder)
{
    ResourceLibraryPathList result;
    if (!std_fs::exists(folder))
        return result;

    std::list<std_path> children;
    for (const auto& it : std_fs::directory_iterator(folder)) {
        if (!it.is_regular_file()) {
            if (it.is_directory())
                children.push_back(it.path());
            continue;
        }
        auto f    = it.path().filename();
        auto ext1 = f.extension();
        if (ext1 != extension1)
            continue;
        auto f2 = f.stem();
        if (f2.extension() != extension2)
            continue;
        std::string id   = path2string(f2.stem());
        auto        root = it.path().parent_path();

        ResourceLibrary tmp(id);
        tmp.setIndexFolder(root);
        if (tmp.loadIndex(true)) {
            result.records.push_back(ResourceLibraryPathList::Record{ root, tmp.m_id, tmp.m_dependencies });
        } else {
            std::cerr << "Warning: failed to load index from:" << path2string(root) << "/" << id << "\n";
        }
    }
    if (result.records.empty()) {
        for (const std_path& path : children) {
            auto subRecords = searchIndexInFolderRecursive(path);
            for (const auto& rec : subRecords.records)
                result.records.push_back(rec);
        }
    }
    return result;
}

std::shared_ptr<ResourceLibrary> ResourceLibrary::makeMergedLibrary(const ResourceLibraryPathList& pathList)
{
    Mernel::ProfilerScope scope("makeMergedLibrary");
    auto                  result = std::make_shared<ResourceLibrary>();
    {
        for (const auto& rec : pathList.records) {
            ResourceLibrary tmp(rec.id);
            tmp.setIndexFolder(rec.path);
            Logger(Logger::Info) << "Loading resource index:" << path2string(rec.path / rec.id);
            if (!tmp.loadIndex())
                throw std::runtime_error("Failed to load index:" + path2string(rec.path / rec.id));
            result->mergeWith(tmp);
        }
    }
    {
        Graph g;
        {
            for (const auto& dbPair : result->m_databases) {
                DepList depList;
                if (!dbPair.second.baseId.empty())
                    depList.push_back(Dep{ std::string{ dbPair.second.baseId } });
                g.add(std::string{ dbPair.first }, depList);
            }
            g.sort();
        }
        for (const auto& dbId : g.getSortResult()) {
            ResourceDatabase& db = result->m_databases[dbId];
            if (!db.baseId.empty()) {
                ResourceDatabase& depDb = result->m_databases[db.baseId];
                for (auto& f : depDb.filesFullPathsWithDeps) {
                    db.filesFullPathsWithDeps.push_back(f);
                }
            }
            for (auto& f : db.filesFullPaths) {
                db.filesFullPathsWithDeps.push_back(f);
            }
        }
    }
    return result;
}

void ResourceLibrary::setIndexFolder(const std_path& folder)
{
    m_indexFolder           = folder;
    m_mainStorage.m_rootDir = path2string(m_indexFolder) + "/";
}

void ResourceLibrary::addDep(std::string dep, ResourceLibrary::DepsRequire require)
{
    m_dependencies.push_back({ dep, require });
}

bool ResourceLibrary::loadIndex(bool onlyMeta)
{
    // using txt format is 10 times faster in debug mode as json/cbor/etc.
    // For release mode it is unnoticeable, but I'd like to have fast startup times in debug - event +0.5s is frustrating.
    // also, i think it's a bit more readable and editable that json.
    if (m_id.empty())
        return false;

    std::ifstream ifs(m_indexFolder / (m_id + fullSuffix), std::ios_base::in | std::ios_base::binary);
    if (!ifs)
        return false;

    ifs.seekg(0, std::ios::end);

    m_mainStorage.m_buffer.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(m_mainStorage.m_buffer.data(), m_mainStorage.m_buffer.size());

    FastCsvTable csvTable(m_mainStorage.m_buffer.data(), m_mainStorage.m_buffer.size());

    m_dependencies.clear();

    {
        while (csvTable.scanLine()) {
            if (csvTable.line.empty() || csvTable.line == "$")
                break;
            //tokens = splitLine(line, '\t', true);
            Dep dep;
            dep.id = csvTable.row[0];
            if (csvTable.row[1] == std::string_view("soft"))
                dep.require = DepsRequire::Soft;
            m_dependencies.push_back(dep);
        }
    }

    if (onlyMeta)
        return true; // @todo: we still reading whole file in the memory

    IdMappingMedia*     currentMapping    = nullptr;
    IdMappingTrans*     currentLocMapping = nullptr;
    ResourceMedia::Type currentType       = ResourceMedia::Type::None;
    ParseMode           parseMode         = ParseMode::Undefined;

    while (csvTable.scanLine()) {
        if (csvTable.line.empty())
            break;

        const bool isHeader = !csvTable.row[0].empty();

        if (isHeader) {
            const std::string_view& typeName = csvTable.row[0];
            parseMode                        = ParseMode::Undefined;
            if (typeName == typeDatabase) {
                parseMode = ParseMode::Database;
            } else if (typeName == typeTranslation) {
                parseMode = ParseMode::Translation;
                std::string_view localeId{ csvTable.row[1] };
                std::string_view contextId{ csvTable.row[2] };
                currentLocMapping = &m_translations[localeId][contextId];
            }
            auto it = std::find_if(mediaTypeMapping.cbegin(), mediaTypeMapping.cend(), [&typeName](const auto& pair) { return pair.second == typeName; });
            if (it != mediaTypeMapping.cend()) {
                currentType    = it->first;
                currentMapping = &m_media[it->first];
                parseMode      = ParseMode::Media;
            }
            continue;
        }
        if (parseMode == ParseMode::Undefined)
            throw std::runtime_error("Encountered undefined section in the index file");

        //tokens.erase(tokens.begin());
        const std::string_view id{ csvTable.row[1] };
        //tokens.erase(tokens.begin());

        if (parseMode == ParseMode::Media) {
            ResourceMedia& res = (*currentMapping)[id];
            res.type           = currentType;
            res.id             = id;
            res.subdir         = ensureTrailingSlashOrEmpty(csvTable.row[2]);
            res.mainFilename   = csvTable.row[3];
            res.root           = m_mainStorage.m_rootDir;
        } else if (parseMode == ParseMode::Translation) {
            auto& values = (*currentLocMapping)[id].values;
            for (size_t i = 2; i < csvTable.row.size(); i++) {
                values.push_back(csvTable.row[i]);
            }
        } else if (parseMode == ParseMode::Database) {
            ResourceDatabase& res = m_databases[id];
            res.id                = id;
            res.baseId            = csvTable.row[2];
            res.subdir            = ensureTrailingSlashOrEmpty(csvTable.row[3]);
            res.root              = m_mainStorage.m_rootDir;
            for (size_t i = 4; i < csvTable.row.size(); i++) {
                res.files.push_back(csvTable.row[i]);
            }
            for (auto& f : res.files)
                res.filesFullPaths.push_back(std::string{ res.root } + std::string{ res.subdir } + std::string{ f });
        }
    }

    return true;
}

bool ResourceLibrary::saveIndex()
{
    if (m_id.empty())
        return false;

    std::ofstream ofs(m_indexFolder / (m_id + fullSuffix), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if (!ofs)
        return false;

    for (auto& dep : m_dependencies) {
        ofs << dep.id << '\t';
        ofs << (dep.require == DepsRequire::Hard ? "hard" : "soft") << '\t';
        //ofs << (dep.merge == DepsMerge::Extend ? "extend" : (dep.merge == DepsMerge::Append ? "append" : "replace")) << '\t';
        ofs << "\n";
    }
    ofs << "\n";
    const std::string replaceStart = path2string(m_indexFolder);
    for (const auto& pair1 : m_media) {
        const ResourceMedia::Type type = pair1.first;
        const std::string&        name = mediaTypeMapping.at(type);
        ofs << name << "\n";
        for (const auto& pair2 : pair1.second) {
            const ResourceMedia& desc = pair2.second;
            ofs << '\t' << desc.id << '\t' << desc.subdir << '\t' << desc.mainFilename << "\n";
        }
    }
    for (const auto& pair1 : m_translations) {
        const auto& localeId = pair1.first;
        for (const auto& pair2 : pair1.second) {
            const auto& contextId = pair2.first;
            ofs << typeTranslation << '\t' << localeId << '\t' << contextId << "\n";
            for (const auto& pair3 : pair2.second) {
                ofs << '\t' << pair3.first;
                for (auto& val : pair3.second.values)
                    ofs << '\t' << val;
                ofs << "\n";
            }
        }
    }
    if (!m_databases.empty()) {
        ofs << typeDatabase << "\n";
    }
    for (const auto& pair1 : m_databases) {
        const ResourceDatabase& desc = pair1.second;
        ofs << '\t' << desc.id << '\t' << desc.baseId << '\t' << desc.subdir;
        for (auto& f : desc.files) {
            ofs << '\t' << f;
        }
        ofs << "\n";
    }
    return true;
}

bool ResourceLibrary::mediaExists(ResourceMedia::Type type, const std::string& id) const noexcept
{
    auto itType = m_media.find(type);
    if (itType == m_media.cend())
        return false;

    auto& idMapping = itType->second;
    auto  it        = idMapping.find(id);
    if (it == idMapping.cend())
        return false;

    const auto&     desc = it->second;
    std::error_code ec;
    return std_fs::exists(std_path(desc.getFullPath()), ec);
}

const ResourceMedia& ResourceLibrary::getMedia(ResourceMedia::Type type, const std::string& id) const
{
    if (!m_media.at(type).contains(id))
        Logger(Logger::Err) << "Invalid id=" << id;
    return m_media.at(type).at(id);
}

void ResourceLibrary::registerResource(ResourceMedia desc)
{
    ResourceMedia newRec{ desc.type, deepCopy(desc.id), deepCopy(desc.subdir, true), deepCopy(desc.mainFilename), m_mainStorage.m_rootDir };

    m_media[desc.type][newRec.id] = newRec;
}

std::vector<std::string> ResourceLibrary::getTranslationContextChildren(const std::string& localeId, const std::string& contextId) const
{
    auto it = m_translations.find(localeId);
    if (it == m_translations.cend())
        return {};
    auto it2 = it->second.find(contextId);
    if (it2 == it->second.cend())
        return {};
    std::vector<std::string> result;
    result.reserve(it2->second.size());
    for (auto& p : it2->second)
        result.push_back(std::string{ p.first });
    return result;
}

std::vector<std::string> ResourceLibrary::getTranslation(const std::string& localeId, const std::string& contextId, const std::string& id) const
{
    auto it = m_translations.find(localeId);
    if (it == m_translations.cend())
        return {};
    auto it2 = it->second.find(contextId);
    if (it2 == it->second.cend())
        return {};
    auto it3 = it2->second.find(id);
    if (it3 == it2->second.cend())
        return {};
    return getStringsFromViews(it3->second.values);
}

void ResourceLibrary::registerResource(ResourceTranslation desc)
{
    ResourceTranslation newRec{ deepCopy(desc.localeId), deepCopy(desc.contextId), deepCopy(desc.id), deepCopy(desc.values) };
    ;
    m_translations[newRec.localeId][newRec.contextId][newRec.id] = newRec;
}

std::vector<std::string> ResourceLibrary::getDatabaseIds() const
{
    std::vector<std::string> result;
    for (auto& p : m_databases)
        result.push_back(std::string{ p.first });
    return result;
}

const ResourceDatabase& ResourceLibrary::getDatabase(const std::string& id) const
{
    if (!m_databases.contains(id))
        Logger(Logger::Err) << "Invalid id=" << id;
    return m_databases.at(id);
}

void ResourceLibrary::registerResource(ResourceDatabase desc)
{
    ResourceDatabase newRec{ deepCopy(desc.id), deepCopy(desc.baseId), deepCopy(desc.subdir, true), deepCopy(desc.files), m_mainStorage.m_rootDir, {}, {} };
    newRec.filesFullPaths.reserve(desc.files.size());
    for (auto& f : newRec.files)
        newRec.filesFullPaths.push_back(std::string{ newRec.root } + std::string{ newRec.subdir } + std::string{ f });
    m_databases[newRec.id] = newRec;
}

void ResourceLibrary::mergeWith(ResourceLibrary& newData)
{
    m_storages.insert(m_storages.end(),
                      std::make_move_iterator(newData.m_storages.begin()),
                      std::make_move_iterator(newData.m_storages.end()));

    newData.m_storages.erase(newData.m_storages.begin(), newData.m_storages.end());

    {
        for (const auto& pair1 : newData.m_media) {
            const ResourceMedia::Type type      = pair1.first;
            auto&                     idMapping = m_media[type];
            for (const auto& pair2 : pair1.second) {
                auto& resourceDesc = idMapping[pair2.first];
                resourceDesc       = pair2.second;
            }
        }
    }
    {
        for (const auto& pair1 : newData.m_translations) {
            const auto& key1     = pair1.first;
            auto&       mapping1 = m_translations[key1];
            for (const auto& pair2 : pair1.second) {
                const auto& key2     = pair2.first;
                auto&       mapping2 = mapping1[key2];
                for (const auto& pair3 : pair2.second) {
                    const auto&                key3     = pair3.first;
                    ResourceTranslation&       mapping3 = mapping2[key3];
                    const ResourceTranslation& value    = pair3.second;
                    if (value.values.size() > 1 && value.values[0] == "+") {
                        if (mapping3.values.size() > 0)
                            mapping3.values.insert(mapping3.values.end(), value.values.cbegin() + 1, value.values.cend());
                    } else {
                        mapping3 = value;
                    }
                }
            }
        }
    }
    for (const auto& db : newData.m_databases) {
        m_databases[db.first] = db.second;
    }
}

std::string_view ResourceLibrary::deepCopy(const std::string_view& data, bool ensureSlash)
{
    const bool appendSlash = ensureSlash && !data.empty() && data[data.size() - 1] != '/';
    return makeNewString(std::string{ data } + (appendSlash ? "/" : ""));
}

std::vector<std::string_view> ResourceLibrary::deepCopy(const std::vector<std::string_view>& data)
{
    std::vector<std::string_view> result;
    result.reserve(data.size());
    for (auto& s : data)
        result.push_back(deepCopy(s));
    return result;
}

std::string_view ResourceLibrary::makeNewString(std::string data)
{
    m_mainStorage.m_strStorage.push_back(std::move(data));
    std::string& newRecord = m_mainStorage.m_strStorage.back();
    return std::string_view(newRecord.data(), newRecord.size());
}

std::string_view ResourceLibrary::ensureTrailingSlashOrEmpty(std::string_view path)
{
    if (path.empty() || path[path.size() - 1] == '/')
        return path;
    return makeNewString(std::string{ path } + '/');
}

void ResourceLibrary::ResourceLibraryPathList::topoSort()
{
    Graph g;
    for (const auto& rec : records) {
        g.add(rec.id, rec.deps);
    }
    g.sort();
    std::vector<Record> recordsNew;
    for (const auto& id : g.getSortResult()) {
        auto it = std::find_if(records.cbegin(), records.cend(), [id](const auto& rec) { return rec.id == id; });
        assert(it != records.cend());
        recordsNew.push_back(*it);
    }
    this->records = recordsNew;
}

}
