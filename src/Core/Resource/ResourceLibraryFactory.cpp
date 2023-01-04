/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ResourceLibraryFactory.hpp"

#include "ResourceLibrary.hpp"

#include "MernelPlatform/StringUtils.hpp"
#include "MernelPlatform/Logger.hpp"

#include <set>

namespace FreeHeroes::Core {
using namespace Mernel;

namespace {
const std::string_view g_modFolderSuffix{ "fhmod" };

const std::map<std::string, ResourceType> g_knownTypes{
    { ".fhsprite.json", ResourceType::Sprite },
    { ".wav", ResourceType::Sound },
    { ".mp3", ResourceType::Music },
    { ".webp", ResourceType::Video },
    { ".fhdb.json", ResourceType::DbSegment },
    { ".fhdbindex.json", ResourceType::DbIndex },
};

void mergeMappings(ResourceLibrary::TypeMappingMedia& dest, const ResourceLibrary::TypeMappingMedia& source)
{
    for (const auto& [type, idMapping] : source) {
        auto& idMappingDest = dest[type];
        for (const auto& [id, path] : idMapping) {
            idMappingDest[id] = path;
        }
    }
}

}

struct ResourceLibraryFactory::Impl {
    struct Mod {
        std_path                          m_modRoot;
        bool                              m_scanned = false;
        ResourceLibrary::TypeMappingMedia m_modMapping;

        void scanIfNeeded() noexcept
        {
            m_scanned = true;
            for (const auto& it : std_fs::recursive_directory_iterator(m_modRoot)) {
                if (!it.is_regular_file())
                    continue;

                const auto  path    = it.path();
                auto        name    = path.filename();
                std::string fullExt = "";
                while (name.has_extension()) {
                    fullExt = path2string(name.extension()) + fullExt;
                    name    = name.stem();
                }
                const auto id = path2string(name);

                auto typeIt = g_knownTypes.find(fullExt);
                if (typeIt == g_knownTypes.cend())
                    continue;
                const ResourceType type = typeIt->second;

                //Logger(Logger::Warning) << path;
                m_modMapping[type][id] = path;
            }
        }
    };

    std::map<std::string, Mod> m_mods;

    void searchForSpecialModFolders(const std_path& folder)
    {
        std::error_code ec;
        if (!std_fs::exists(folder, ec))
            return;

        for (const auto& it : std_fs::directory_iterator(folder)) {
            if (!it.is_directory())
                continue;

            auto parts = splitLine(path2string(it.path().filename()), '.');
            if (parts.size() != 2 || parts[1] != g_modFolderSuffix) {
                searchForSpecialModFolders(it.path());
                continue;
            }
            const auto id = parts[0];
            if (m_mods.contains(id)) {
                Logger(Logger::Err) << "Found duplicate paths for mod id '" << id << "', both '" << path2string(m_mods[id].m_modRoot) << "' and '" << path2string(it.path()) << "'";
            }
            Logger(Logger::Info) << "Registered mod '" << id << "' at '" << it.path() << "'";
            m_mods[id] = { .m_modRoot = it.path() };
        }
    }
};

ResourceLibraryFactory::ResourceLibraryFactory()
    : m_impl(std::make_unique<Impl>())
{
}

ResourceLibraryFactory::~ResourceLibraryFactory() = default;

void ResourceLibraryFactory::scanForMods(const Mernel::std_path& root)
{
    m_impl->searchForSpecialModFolders(root);
}

void ResourceLibraryFactory::scanModSubfolders() const noexcept
{
    for (auto&& [id, mod] : m_impl->m_mods)
        mod.scanIfNeeded();
}

IResourceLibrary::ConstPtr ResourceLibraryFactory::create(const ModOrder& loadOrder) const noexcept
{
    scanModSubfolders();
    const std::set<std::string>       orderSet(loadOrder.cbegin(), loadOrder.cend());
    ResourceLibrary::TypeMappingMedia mapping;
    ModOrder                          realOrder;
    for (const auto& id : loadOrder) {
        if (!m_impl->m_mods.contains(id))
            continue;

        mergeMappings(mapping, m_impl->m_mods[id].m_modMapping);
        realOrder.push_back(id);
    }
    for (auto&& [id, mod] : m_impl->m_mods) {
        if (orderSet.contains(id))
            continue;
        mergeMappings(mapping, m_impl->m_mods[id].m_modMapping);
        realOrder.push_back(id);
    }
    Logger(Logger::Notice) << "Resource index is created, requested order: <" << loadOrder << ">, resulting order: <" << realOrder << ">";
    return std::make_shared<ResourceLibrary>(std::move(mapping));
}

}
