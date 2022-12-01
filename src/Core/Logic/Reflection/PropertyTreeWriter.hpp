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
    void valueToJson(const T& value, PropertyTree& result)
    {
        result = {};
        result.convertToMap();
        auto& jsonMap = result.getMap();

        auto visitor = [&value, &jsonMap, this](auto&& field) {
            this->valueToJson(field.get(value), jsonMap[field.name()]);
        };

        std::apply([&visitor](auto&&... field) { ((visitor(field)), ...); }, MetaInfo::s_fields<T>);
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
