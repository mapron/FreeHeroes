/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace FreeHeroes::Core {

enum class ResourceType
{
    Invalid,
    Sprite,
    Sound,
    Music,
    Video,
    DbSegment,
    DbIndex,
};

class IResourceLibrary {
public:
    virtual ~IResourceLibrary() = default;

    virtual bool             contains(ResourceType type, const std::string& id) const noexcept   = 0;
    virtual bool             fileExists(ResourceType type, const std::string& id) const noexcept = 0;
    virtual Mernel::std_path get(ResourceType type, const std::string& id) const noexcept        = 0;

    using ConstPtr = std::shared_ptr<const IResourceLibrary>;
};

class IResourceLibraryFactory {
public:
    using ModOrder = std::vector<std::string>;

    virtual ~IResourceLibraryFactory() = default;

    virtual IResourceLibrary::ConstPtr create(const ModOrder& loadOrder) const noexcept = 0;
};

}
