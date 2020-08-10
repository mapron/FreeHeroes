/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <memory>
#include <functional>
#include <string>
#include <map>

namespace rttr {
class variant;
class type;
}

namespace FreeHeroes::Core {
class IGameDatabase;
}

namespace FreeHeroes::Core::Reflection {


class LibraryIdResolver {
public:
    LibraryIdResolver(const IGameDatabase & gameDatabase);
    ~LibraryIdResolver();

    struct ResolutionContext {
        const IGameDatabase * database = nullptr;
    };
    using ResolutionId = std::string;
    using ResolutionResult = std::pair<rttr::variant, bool>;
    using ResolutionCallback = std::function<ResolutionResult(const ResolutionId &, const ResolutionContext&)>;

    static void registerIdResolver(const std::string & typeName, ResolutionCallback callback);

    struct DelayedDeserializeParam {
        LibraryIdResolver * delayedDeserialize = nullptr;
        std::string id;
    };
    static bool hasResolver(const rttr::type & valueType);

    rttr::variant resolve(bool isOptional, const ResolutionId & id, const rttr::type & valueType) const;

private:
    ResolutionContext m_context;

};

}
