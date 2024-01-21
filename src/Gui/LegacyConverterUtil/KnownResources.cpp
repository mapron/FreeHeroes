/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "KnownResources.hpp"

#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"

#include "MernelPlatform/StringUtils.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"

namespace Mernel::Reflection {

ENUM_REFLECTION_STRINGIFY(
    FreeHeroes::KnownResource::ResourceType,
    Unknown,
    Unknown,

    Sprite,
    Sound,
    Video)

ENUM_REFLECTION_STRINGIFY(
    FreeHeroes::KnownResource::Section,
    Unknown,
    Unknown,

    Common,
    Adventure,
    Battle,
    BattleSeige,
    Campaign,
    Puzzle,
    Town,
    Portrait,
    UI)

ENUM_REFLECTION_STRINGIFY(
    FreeHeroes::KnownResource::Type,
    Unknown,
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
    DialogBack)

}

namespace FreeHeroes {

namespace {
struct ConfigFile {
    KnownResource::ResourceType m_type = KnownResource::ResourceType::Unknown;
    std::string                 m_versionMajor;
    std::string                 m_versionMinor;
    std::string                 m_name;
};

const std::vector<ConfigFile> g_configs{
    ConfigFile{ .m_type = KnownResource::ResourceType::Sprite, .m_versionMajor = "sod", .m_versionMinor = "roe", .m_name = "sprites_sod_roe" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sprite, .m_versionMajor = "sod", .m_versionMinor = "ab", .m_name = "sprites_sod_ab" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sprite, .m_versionMajor = "sod", .m_versionMinor = "sod", .m_name = "sprites_sod_sod" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sprite, .m_versionMajor = "hota", .m_versionMinor = "161", .m_name = "sprites_hota_161" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sprite, .m_versionMajor = "hota", .m_versionMinor = "170", .m_name = "sprites_hota_170" },

    ConfigFile{ .m_type = KnownResource::ResourceType::Sound, .m_versionMajor = "sod", .m_versionMinor = "roe", .m_name = "sound_sod_roe" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sound, .m_versionMajor = "sod", .m_versionMinor = "ab", .m_name = "sound_sod_ab" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sound, .m_versionMajor = "sod", .m_versionMinor = "sod", .m_name = "sound_sod_sod" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sound, .m_versionMajor = "hota", .m_versionMinor = "161", .m_name = "sound_hota_161" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Sound, .m_versionMajor = "hota", .m_versionMinor = "170", .m_name = "sound_hota_170" },

    ConfigFile{ .m_type = KnownResource::ResourceType::Video, .m_versionMajor = "sod", .m_versionMinor = "roe", .m_name = "videos_sod_roe" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Video, .m_versionMajor = "sod", .m_versionMinor = "ab", .m_name = "videos_sod_ab" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Video, .m_versionMajor = "sod", .m_versionMinor = "sod", .m_name = "videos_sod_sod" },
    ConfigFile{ .m_type = KnownResource::ResourceType::Video, .m_versionMajor = "hota", .m_versionMinor = "170", .m_name = "videos_hota_170" },
};

std::string resourceTypeToString(KnownResource::ResourceType value)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(value);
    return std::string(str.begin(), str.end());
}
[[maybe_unused]] KnownResource::ResourceType stringToResourceType(const std::string& str)
{
    return Mernel::Reflection::EnumTraits::stringToEnum<KnownResource::ResourceType>({ str.c_str(), str.size() });
}

std::string sectionToString(KnownResource::Section value)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(value);
    return std::string(str.begin(), str.end());
}
KnownResource::Section stringToSection(const std::string& str)
{
    return Mernel::Reflection::EnumTraits::stringToEnum<KnownResource::Section>({ str.c_str(), str.size() });
}

std::string typeToString(KnownResource::Type value)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(value);
    return std::string(str.begin(), str.end());
}
KnownResource::Type stringToType(const std::string& str)
{
    return Mernel::Reflection::EnumTraits::stringToEnum<KnownResource::Type>({ str.c_str(), str.size() });
}

}

void KnownResource::makeSubfolder()
{
    std::vector<std::string> parts;
    if (m_resourceType != ResourceType::Unknown) {
        parts.push_back(resourceTypeToString(m_resourceType));
    }
    if (m_section != Section::Unknown) {
        parts.push_back(sectionToString(m_section));
    }
    if (m_type != Type::Unknown) {
        parts.push_back(typeToString(m_type));
    }
    if (!m_faction.empty()) {
        parts.push_back(m_faction);
    }
    if (parts.empty()) {
        m_destinationSubfolder = "Unknown";
    } else {
        m_destinationSubfolder = Mernel::joinString(parts, "/");
    }
}

void KnownResource::load(const Mernel::PropertyTreeList& row)
{
    row[0].getScalar().convertTo(m_legacyId);
    row[1].getScalar().convertTo(m_newId);

    m_section = stringToSection(row[2].getScalar().toString());
    m_type    = stringToType(row[3].getScalar().toString());

    row[4].getScalar().convertTo(m_faction);
}

const KnownResource* KnownResources::find(ResourceType resourceType, const std::string& legacyId) const
{
    const Data& data = m_data.at(resourceType);
    auto        it   = data.m_index.find(legacyId);
    return it == data.m_index.cend() ? nullptr : it->second;
}

Mernel::PropertyTree KnownResources::findPP(ResourceType resourceType, const std::string& legacyId) const
{
    const Data& data = m_data.at(resourceType);
    auto        it   = data.m_ppData.find(legacyId);
    return it == data.m_ppData.cend() ? Mernel::PropertyTree{} : it->second;
}

KnownResources::KnownResources(const Mernel::std_path& configRoot)
{
    m_data[ResourceType::Unknown] = {};
    m_data[ResourceType::Sprite]  = {};
    m_data[ResourceType::Sound]   = {};
    m_data[ResourceType::Video]   = {};

    m_data[ResourceType::Sprite].m_resources.reserve(10000);

    for (const auto& config : g_configs) {
        Data& data = m_data[config.m_type];

        std::string buffer   = Mernel::readFileIntoBuffer(configRoot / (config.m_name + ".json"));
        auto        jsonData = Mernel::readJsonFromBuffer(buffer);

        for (const auto& row : jsonData.getList()) {
            const auto    rowArr = row.getList();
            KnownResource res;
            res.m_resourceType = config.m_type;
            res.load(row.getList());
            if (data.m_index.contains(res.m_legacyId))
                throw std::runtime_error("Duplicate id in known config:" + res.m_legacyId);

            res.makeSubfolder();
            res.m_versionMajor = config.m_versionMajor;
            res.m_versionMinor = config.m_versionMinor;
            data.m_resources.push_back(std::move(res));
        }
    }
    for (auto& [type, data] : m_data) {
        for (const auto& res : data.m_resources) {
            data.m_index[res.m_legacyId] = &res;
        }
    }

    {
        Data&       data     = m_data[ResourceType::Sprite];
        std::string buffer   = Mernel::readFileIntoBuffer(configRoot / "postProcess.json");
        auto        jsonData = Mernel::readJsonFromBuffer(buffer);

        for (const auto& [key, row] : jsonData.getMap()) {
            data.m_ppData[key] = Mernel::PropertyTree(row.getMap());
        }
    }
}

}
