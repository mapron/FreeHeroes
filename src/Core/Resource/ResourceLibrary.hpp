/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IResourceLibrary.hpp"

#include <map>
#include <unordered_map>
#include <memory>

namespace FreeHeroes::Core {

class ResourceLibrary : public IResourceLibrary {
public:
    using IdMappingMedia   = std::unordered_map<std::string, Mernel::std_path>;
    using TypeMappingMedia = std::map<ResourceType, IdMappingMedia>;

    ResourceLibrary(TypeMappingMedia mapping)
        : m_media(std::move(mapping))
    {
    }

    bool             contains(ResourceType type, const std::string& id) const noexcept override;
    bool             fileExists(ResourceType type, const std::string& id) const noexcept override;
    Mernel::std_path get(ResourceType type, const std::string& id) const noexcept override;

private:
    const TypeMappingMedia m_media;
};

}
