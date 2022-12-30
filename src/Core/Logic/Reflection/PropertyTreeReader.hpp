/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "EnumTraits.hpp"
#include "MetaInfo.hpp"

#include "PropertyTree.hpp"

#include "IGameDatabase.hpp"

#include <set>

namespace FreeHeroes::Core::Reflection {

class PropertyTreeReader {
public:
    PropertyTreeReader(const IGameDatabase* gameDatabase = nullptr)
        : m_gameDatabase(gameDatabase)
    {}

    template<class T>
    void jsonToValueUsingMeta(const PropertyTree& json, T& value)
    {
        const auto& jsonMap = json.getMap();

        auto visitor = [&value, &jsonMap, this](auto&& field) {
            if (jsonMap.contains(field.name())) {
                auto writer = field.makeValueWriter(value);
                this->jsonToValue(jsonMap.at(field.name()), writer.getRef());
            } else {
                field.reset(value);
            }
        };
        std::apply([&visitor](auto&&... field) { ((visitor(field)), ...); }, MetaInfo::s_fields<T>);
    }

    template<HasFieldsForRead T>
    void jsonToValue(const PropertyTree& json, T& value)
    {
        jsonToValueUsingMeta(json, value);
    }

    template<GameDatabaseObject T>
    void jsonToValue(const PropertyTree& json, const T*& value)
    {
        const std::string id = json.getScalar().toString();
        using ObjectType     = std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<decltype(value)>>>>;
        value                = m_gameDatabase->container<ObjectType>()->find(id);
    }

    void jsonToValue(const PropertyTree& json, PropertyTreeScalarHeld auto& value)
    {
        json.getScalar().convertTo(value);
    }

    template<IsEnum Enum>
    void jsonToValue(const PropertyTree& json, Enum& value)
    {
        const auto str = json.isScalar() ? json.getScalar().toString() : "";
        value          = EnumTraits::stringToEnum<Enum>({ str.c_str(), str.size() });
    }

    template<HasCustomTransformRead T>
    void jsonToValue(const PropertyTree& json, T& value)
    {
        PropertyTree tmp;
        if (MetaInfo::transformTreeRead<T>(json, tmp))
            jsonToValueUsingMeta(tmp, value);
        else
            jsonToValueUsingMeta(json, value);
    }

    template<HasFromStringRead T>
    void jsonToValue(const PropertyTree& json, T& value)
    {
        if (!json.isScalar() || !json.getScalar().isString())
            return jsonToValueUsingMeta(json, value);

        std::string tmp = json.getScalar().toString();
        value.fromString(std::move(tmp));
    }

    template<NonAssociative Container>
    void jsonToValue(const PropertyTree& json, Container& container)
    {
        if (!json.isList())
            return;
        container.clear();
        if constexpr (IsStdVector<Container>) {
            container.reserve(json.getList().size());
        }
        auto inserter = std::inserter(container, container.end());
        for (const PropertyTree& child : json.getList()) {
            typename Container::value_type value;
            jsonToValue(child, value);
            *inserter = std::move(value);
        }
    }

    template<IsStdArray Container>
    void jsonToValue(const PropertyTree& json, Container& container)
    {
        if (!json.isList())
            return;
        assert(container.size() <= json.getList().size());
        size_t index = 0;
        for (const PropertyTree& child : json.getList()) {
            typename Container::value_type value;
            jsonToValue(child, value);
            container[index++] = std::move(value);
        }
    }

    template<IsStdOptional Container>
    void jsonToValue(const PropertyTree& json, Container& container)
    {
        if (!json.isList())
            return;
        assert(json.getList().size() <= 1);
        for (const PropertyTree& child : json.getList()) {
            typename Container::value_type value;
            jsonToValue(child, value);
            container = std::move(value);
            //break;
        }
    }

    template<IsMap Container>
    void jsonToValue(const PropertyTree& json, Container& container)
    {
        if (!json.isList())
            return;

        for (const PropertyTree& child : json.getList()) {
            typename Container::mapped_type value;
            typename Container::key_type    key;
            jsonToValue(child["key"], key);
            jsonToValue(child["value"], value);
            container[key] = std::move(value);
        }
    }

    template<IsStringMap Container>
    void jsonToValue(const PropertyTree& json, Container& container)
    {
        if (!json.isMap())
            return;

        for (const auto& [keyString, childJson] : json.getMap()) {
            typename Container::key_type    key;
            typename Container::mapped_type value;
            const PropertyTreeScalar        keyScalar(keyString);
            jsonToValue(childJson, value);
            jsonToValue(keyScalar, key);
            container[key] = value;
        }
    }

private:
    const IGameDatabase* const m_gameDatabase = nullptr;
};

}
