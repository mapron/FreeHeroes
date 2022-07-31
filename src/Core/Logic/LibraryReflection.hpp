/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes {
class PropertyTree;
}

namespace FreeHeroes::Core::Reflection {

void libraryReflectionInit();

class IJsonTransform {
public:
    virtual ~IJsonTransform()                                                             = default;
    [[nodiscard]] virtual bool needTransform(const PropertyTree& in) const noexcept       = 0;
    virtual bool               transform(const PropertyTree& in, PropertyTree& out) const = 0;
};

template<class T>
const IJsonTransform* getJsonTransform();

}
