/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"

#include "MernelReflection/PropertyTreeWriter.hpp"

namespace FreeHeroes::Core {

class PropertyTreeWriterDatabase : public Mernel::Reflection::PropertyTreeWriterBase<PropertyTreeWriterDatabase> {
public:
    template<GameDatabaseObject T>
    void valueToJsonImpl(const T* value, Mernel::PropertyTree& result)
    {
        result = Mernel::PropertyTreeScalar(value ? value->id : std::string());
    }
};

}
