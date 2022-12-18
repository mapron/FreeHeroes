/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "FHMap.hpp"

#include "IRandomGenerator.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

#include "AdventureReflection.hpp"

#include "FHMapReflection.hpp"

namespace FreeHeroes {

void FHMap::toJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void FHMap::fromJson(const PropertyTree& data, const Core::IGameDatabase* database)
{
    Core::Reflection::PropertyTreeReader reader(database);
    *this = {};
    reader.jsonToValue(data, *this);
}

}
