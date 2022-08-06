#pragma once

#include "EnumTraits.hpp"
#include "MetaInfo.hpp"

#include "PropertyTree.hpp"

#include "IGameDatabase.hpp"

#include <set>

namespace FreeHeroes::Core::Reflection {

class PropertyTreeWriter {
public:
    template<HasFields T>
    PropertyTree valueToJson(const T& value)
    {
        PropertyTree result;
        result.convertToMap();
        auto& jsonMap = result.getMap();

        auto visitor = [&value, &jsonMap, this](auto&& field) {
            jsonMap[field.name()] = this->valueToJson(field.get(value));
        };

        std::apply([&visitor](auto&&... field) { ((visitor(field)), ...); }, MetaInfo::s_fields<T>);

        return result;
    }

    PropertyTree valueToJson(const PropertyTreeScalarHeld auto& value)
    {
        return PropertyTreeScalar(value);
    }

    template<GameDatabaseObject T>
    PropertyTree valueToJson(const T* value)
    {
        PropertyTreeScalar result(value ? value->id : std::string());
        return result;
    }

    PropertyTree valueToJson(const IsEnum auto& value)
    {
        const auto str = EnumTraits::enumToString(value);
        return PropertyTreeScalar(std::string(str.begin(), str.end()));
    }

    template<NonAssociative Container>
    PropertyTree valueToJson(const Container& container)
    {
        PropertyTree result;
        result.convertToList();
        for (const auto& value : container) {
            PropertyTree child = valueToJson(value);
            result.append(std::move(child));
        }
        return result;
    }

    template<IsMap Container>
    PropertyTree valueToJson(const Container& container)
    {
        PropertyTree result;
        result.convertToList();
        for (const auto& [key, value] : container) {
            PropertyTree childKey   = valueToJson(key);
            PropertyTree childValue = valueToJson(value);
            PropertyTree pair;
            pair["key"]   = std::move(childKey);
            pair["value"] = std::move(childValue);
            result.append(std::move(pair));
        }
        return result;
    }

    template<IsStringMap Container>
    PropertyTree valueToJson(const Container& container)
    {
        PropertyTree result;
        result.convertToMap();
        for (const auto& [key, value] : container) {
            PropertyTree childKey   = valueToJson(key);
            PropertyTree childValue = valueToJson(value);
            assert(childKey.isScalar() && childKey.getScalar().isString());
            result[childKey.getScalar().toString()] = std::move(childValue);
        }
        return result;
    }
};

}
