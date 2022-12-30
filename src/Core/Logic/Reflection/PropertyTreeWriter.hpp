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

class PropertyTreeWriter {
public:
    template<class T>
    void valueToJsonUsingMeta(const T& value, PropertyTree& result)
    {
        result = {};
        result.convertToMap();
        auto& jsonMap = result.getMap();

        auto visitor = [&value, &jsonMap, this](auto&& field) {
            const auto& fieldVal = field.get(value);
            if (!field.isDefault(fieldVal))
                this->valueToJson(fieldVal, jsonMap[field.name()]);
        };

        std::apply([&visitor](auto&&... field) { ((visitor(field)), ...); }, MetaInfo::s_fields<T>);
    }

    template<HasFieldsForWrite T>
    void valueToJson(const T& value, PropertyTree& result)
    {
        valueToJsonUsingMeta(value, result);
    }

    void valueToJson(const PropertyTreeScalarHeld auto& value, PropertyTree& result)
    {
        result = PropertyTreeScalar(value);
    }

    template<GameDatabaseObject T>
    void valueToJson(const T* value, PropertyTree& result)
    {
        result = PropertyTreeScalar(value ? value->id : std::string());
    }

    void valueToJson(const IsEnum auto& value, PropertyTree& result)
    {
        const auto str = EnumTraits::enumToString(value);
        result         = PropertyTreeScalar(std::string(str.begin(), str.end()));
    }

    template<HasCustomTransformWrite T>
    void valueToJson(const T& value, PropertyTree& result)
    {
        valueToJsonUsingMeta(value, result);
        PropertyTree tmp;
        if (MetaInfo::transformTreeWrite<T>(result, tmp))
            result = std::move(tmp);
    }

    template<HasToStringWrite T>
    void valueToJson(const T& value, PropertyTree& result)
    {
        result = PropertyTreeScalar(value.toString());
    }

    template<NonAssociative Container>
    void valueToJson(const Container& container, PropertyTree& result)
    {
        result = {};
        result.convertToList();
        result.getList().resize(std::size(container));
        size_t i = 0;
        for (const auto& value : container) {
            PropertyTree& child = result.getList()[i++];
            valueToJson(value, child);
        }
    }

    template<IsStdOptional Container>
    void valueToJson(const Container& container, PropertyTree& result)
    {
        result = {};
        result.convertToList();
        if (!container.has_value())
            return;

        result.getList().resize(1);

        PropertyTree& child = result.getList()[0];
        valueToJson(container.value(), child);
    }

    template<IsMap Container>
    void valueToJson(const Container& container, PropertyTree& result)
    {
        result = {};
        result.convertToList();
        for (const auto& [key, value] : container) {
            PropertyTree pair;
            valueToJson(key, pair["key"]);
            valueToJson(value, pair["value"]);
            result.append(std::move(pair));
        }
    }

    template<IsStringMap Container>
    void valueToJson(const Container& container, PropertyTree& result)
    {
        result = {};
        result.convertToMap();
        for (const auto& [key, value] : container) {
            PropertyTree childKey;
            valueToJson(key, childKey);
            assert(childKey.isScalar() && childKey.getScalar().isString());

            valueToJson(value, result[childKey.getScalar().toString()]);
        }
    }
};

}
