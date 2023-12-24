/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ViewSettings.hpp"
#include "ViewSettingsReflection.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

namespace FreeHeroes {

void ViewSettings::toJson(Mernel::PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void ViewSettings::fromJson(const Mernel::PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

}
