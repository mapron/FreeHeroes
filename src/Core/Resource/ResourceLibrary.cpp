/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ResourceLibrary.hpp"

namespace FreeHeroes::Core {

bool ResourceLibrary::contains(ResourceType type, const std::string& id) const noexcept
{
    auto itType = m_media.find(type);
    if (itType == m_media.cend())
        return false;

    auto& idMapping = itType->second;
    return idMapping.contains(id);
}

bool ResourceLibrary::fileExists(ResourceType type, const std::string& id) const noexcept
{
    auto itType = m_media.find(type);
    if (itType == m_media.cend())
        return false;

    auto& idMapping = itType->second;
    auto  it        = idMapping.find(id);
    if (it == idMapping.cend())
        return false;

    const Mernel::std_path& desc = it->second;
    std::error_code         ec;
    return Mernel::std_fs::exists(desc, ec);
}

Mernel::std_path ResourceLibrary::get(ResourceType type, const std::string& id) const noexcept
{
    auto itType = m_media.find(type);
    if (itType == m_media.cend())
        return {};

    auto& idMapping = itType->second;
    auto  it        = idMapping.find(id);
    if (it == idMapping.cend())
        return {};

    const Mernel::std_path& desc = it->second;
    return desc;
}

}
