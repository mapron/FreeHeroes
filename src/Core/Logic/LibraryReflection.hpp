/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <json_fwd.hpp>

namespace FreeHeroes::Core::Reflection {

void libraryReflectionStub();

class IJsonTransform
{
public:
    virtual ~IJsonTransform() = default;
    [[nodiscard]] virtual bool needTransform(const nlohmann::json & in) const noexcept = 0;
    virtual bool transform(const nlohmann::json & in, nlohmann::json & out) const = 0;
};

template<class T>
const IJsonTransform * getJsonTransform();

}
