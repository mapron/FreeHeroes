/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibraryIdResolver.hpp"

#include "IGameDatabase.hpp"

#include "Logger.hpp"

#include <rttr/type>

#include <deque>
#include <cassert>

namespace FreeHeroes::Core::Reflection {


namespace  {
using ResolversMap = std::map<std::string, LibraryIdResolver::ResolutionCallback>;

static ResolversMap & getCallbacks() {
    /// @note: static global variable!
    static ResolversMap s_callbacks;
    return s_callbacks;
}

}


LibraryIdResolver::LibraryIdResolver(const IGameDatabase & gameDatabase)
{
    m_context.database = &gameDatabase;
}

LibraryIdResolver::~LibraryIdResolver()
{

}

void LibraryIdResolver::registerIdResolver(const std::string& typeName, ResolutionCallback callback)
{
    getCallbacks()[typeName] = std::move(callback);
}

bool LibraryIdResolver::hasResolver(const rttr::type& valueType)
{
    const auto & s_callbacks =  getCallbacks();
    std::string name {valueType.get_name()};
    auto it = s_callbacks.find(name);
    return it != s_callbacks.cend();
}

rttr::variant LibraryIdResolver::resolve(bool isOptional, const ResolutionId & id, const rttr::type& valueType) const
{
    const auto & s_callbacks =  getCallbacks();
    std::string name {valueType.get_name()};
    auto it = s_callbacks.find(name);
    if (it == s_callbacks.cend())
        throw std::runtime_error("Unknown type for resulution:" + name);

    if (!isOptional && id.empty())
        throw std::runtime_error("Empty id for non-optional link");

    const ResolutionCallback & resolutionCallback = it->second;
    auto res = resolutionCallback(id, m_context);
    const bool status = res.second;

    if (!status && !isOptional)
        throw std::runtime_error("Failed dependency resolution for id:" + id);

    return res.first;
}


}
