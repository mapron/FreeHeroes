/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace FreeHeroes::Core {

struct ResourceMedia {
    enum class Type { None, Sprite, Sound, Music, Video, Other };
    Type type = Type::None;
    std::string_view id;
    std::string_view subdir; // empty or  ends with '/'
    std::string_view mainFilename;
    // non-serializable.
    std::string_view root;     // ends with '/'
    std_path getFullPath() const { return string2path( std::string{root} + std::string{subdir} + std::string{mainFilename} ); }
};

struct ResourceTranslation {
    std::string_view localeId;
    std::string_view contextId;
    std::string_view id;
    std::vector<std::string_view> values;
};

struct ResourceDatabase {
    std::string_view id;
    std::string_view baseId;
    std::string_view subdir;    // empty or  ends with '/'
    std::vector<std::string_view> files;

    // non-serializable.
    std::string_view root;      // ends with '/'
    std::vector<std::string> filesFullPaths;
    std::vector<std::string> filesFullPathsWithDeps;
};


class IResourceLibrary {
public:
    virtual ~IResourceLibrary() = default;

    virtual bool mediaExists(ResourceMedia::Type type, const std::string & id) const noexcept = 0;
    virtual const ResourceMedia & getMedia(ResourceMedia::Type type, const std::string & id) const = 0;

    virtual void registerResource(ResourceMedia desc) = 0;

    virtual std::vector<std::string> getTranslationContextChildren(const std::string & localeId, const std::string & contextId) const = 0;
    virtual std::vector<std::string> getTranslation(const std::string & localeId, const std::string & contextId,  const std::string & id) const = 0;
    virtual void registerResource(ResourceTranslation desc) = 0;

    virtual std::vector<std::string> getDatabaseIds() const = 0;
    virtual const ResourceDatabase & getDatabase(const std::string & id) const = 0;
    virtual void registerResource(ResourceDatabase desc) = 0;
};

}
