/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include "MernelPlatform/PropertyTree.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <string_view>
#include <vector>
#include <set>
#include <unordered_map>

namespace FreeHeroes {

struct KnownResource {
    enum class ResourceType
    {
        Unknown,
        Sprite,
        Sound,
        Video,
    };

    enum class Section
    {
        Unknown,
        Common,
        Adventure,
        Battle,
        BattleSiege,
        Campaign,
        Puzzle,
        Town,
        Portrait,
        UI,
    };
    enum class Type
    {
        Unknown,
        Hero,
        Creature,
        Dwelling,
        Artifact,
        Spell,
        Terrain,
        Building,
        Castle,
        Obstacle,
        Button,
        Menu,
        DialogBack,
    };
    using SectionSet = std::set<Section>;

    ResourceType m_resourceType = ResourceType::Unknown;
    Section      m_section      = Section::Unknown; // [2]
    Type         m_type         = Type::Unknown;    // [3]

    std::string m_versionMajor;
    std::string m_versionMinor;

    std::string m_legacyId; // [0]
    std::string m_newId;    // [1]
    std::string m_faction;  // [4]
    std::string m_destinationSubfolder;

    void makeSubfolder();
    void load(const Mernel::PropertyTreeList& row);
};

class LEGACYCONVERTERUTIL_EXPORT KnownResources {
public:
    using ResourceType = KnownResource::ResourceType;

    KnownResources(const Mernel::std_path& configRoot);
    const KnownResource* find(ResourceType resourceType, const std::string& legacyId) const;
    Mernel::PropertyTree findPP(ResourceType resourceType, const std::string& legacyId) const;

private:
    struct Data {
        std::vector<KnownResource>                            m_resources;
        std::unordered_map<std::string, const KnownResource*> m_index;

        std::unordered_map<std::string, Mernel::PropertyTree> m_ppData;
    };
    std::map<ResourceType, Data> m_data;
};

}
