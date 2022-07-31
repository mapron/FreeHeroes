/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <rttr/type>

namespace FreeHeroes {
class PropertyTree;
}

namespace FreeHeroes::Core::Reflection {
class LibraryIdResolver;
void deserializeFromJson(LibraryIdResolver& idResolver, rttr::instance obj, const PropertyTree& jsonObject);

}
