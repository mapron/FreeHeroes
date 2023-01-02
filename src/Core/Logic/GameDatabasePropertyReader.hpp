/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"

namespace FreeHeroes::Core {

class PropertyTreeReaderDatabase : public Mernel::Reflection::PropertyTreeReaderBase<PropertyTreeReaderDatabase> {
public:
    PropertyTreeReaderDatabase(const FreeHeroes::Core::IGameDatabase* gameDatabase)
        : m_gameDatabase(gameDatabase)
    {}

    template<GameDatabaseObject T>
    void jsonToValueImpl(const Mernel::PropertyTree& json, const T*& value)
    {
        const std::string id = json.getScalar().toString();
        using ObjectType     = std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<decltype(value)>>>>;
        value                = m_gameDatabase->container<ObjectType>()->find(id);
    }

private:
    const FreeHeroes::Core::IGameDatabase* const m_gameDatabase = nullptr;
};

}
