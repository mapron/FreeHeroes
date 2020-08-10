/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "JsonRTTRSerialize.hpp"

#include "LibraryIdResolver.hpp"

#include "Logger.hpp"

#include <json.hpp>

namespace FreeHeroes::Core::Reflection {

using namespace nlohmann;
using namespace rttr;

void serializeToJson(const instance& obj, json& data);

void reflectionVariantToJson(const variant& var, json& data);

bool scalarToJson(const type& t, const variant& var, json& data)
{
    if (t.is_arithmetic())
    {
        if (t == type::get<bool>())          data = (var.to_bool());
        else if (t == type::get<char>())     data = (var.to_bool());
        else if (t == type::get<int8_t>())   data = (var.to_int8());
        else if (t == type::get<int16_t>())  data = (var.to_int16());
        else if (t == type::get<int32_t>())  data = (var.to_int32());
        else if (t == type::get<int64_t>())  data = (var.to_int64());
        else if (t == type::get<uint8_t>())  data = (var.to_uint8());
        else if (t == type::get<uint16_t>()) data = (var.to_uint16());
        else if (t == type::get<uint32_t>()) data = (var.to_uint32());
        else if (t == type::get<uint64_t>()) data = (var.to_uint64());
        else if (t == type::get<float>())    data = (var.to_double());
        else if (t == type::get<double>())   data = (var.to_double());

        return true;
    }
    else if (t.is_enumeration())
    {
        bool ok = false;
        auto result = var.to_string(&ok);
        if (ok)
        {
            data = (var.to_string());
        }
        else
        {
            ok = false;
            auto value = var.to_uint64(&ok);
            if (ok)
                data = (value);
            else
                data = json(json::value_t::null);
        }

        return true;
    }
    else if (t == type::get<std::string>())
    {
        data = (var.to_string());
        return true;
    }
    else if (LibraryIdResolver::hasResolver(t))
    {
        data = (var.to_string()); // converter function will do all magic.
        return true;
    }

    return false;
}

static void reflectionSequenceToJson(const variant_sequential_view& view, json& data)
{
    for (const auto& item : view)
    {
        json element;
        if (item.is_sequential_container())
        {
            reflectionSequenceToJson(item.create_sequential_view(), element);
        }
        else
        {
            variant wrappedVar = item.extract_wrapped_value();
            type valueType = wrappedVar.get_type();
            if (!scalarToJson(valueType, wrappedVar, element))
                serializeToJson(wrappedVar, element);

        }
        data.push_back(std::move(element));
    }
}

static void reflectionMapToJson(const variant_associative_view& view, json& data)
{
    if (view.is_key_only_type())
    {
        for (auto& item : view)
        {
            json element;
            reflectionVariantToJson(item.first, element);
            data.push_back(element);
        }
    }
    else
    {
        for (auto& item : view)
        {
            json keyValue;
            reflectionVariantToJson(item.first, keyValue);

            json valueValue;
            reflectionVariantToJson(item.second, valueValue);
            assert(keyValue.is_string());
            data.push_back(json{{"key", keyValue}, {"value", valueValue}});
        }
    }
}

void reflectionVariantToJson(const variant& var, json& data)
{
    auto valueType = var.get_type();
    auto wrappedType = valueType.is_wrapper() ? valueType.get_wrapped_type() : valueType;
    bool isWrapper = wrappedType != valueType;

    if (scalarToJson(isWrapper ? wrappedType : valueType,
                     isWrapper ? var.extract_wrapped_value() : var,
                     data))
    {
    }
    else if (var.is_sequential_container())
    {
        reflectionSequenceToJson(var.create_sequential_view(), data);
    }
    else if (var.is_associative_container())
    {
        reflectionMapToJson(var.create_associative_view(), data);
    }
    else
    {
        auto childProps = isWrapper ? wrappedType.get_properties() : valueType.get_properties();
        if (!childProps.empty())
            serializeToJson(var, data);

    }
}

void serializeToJson(const instance& obj2, json& data)
{
    instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;

    auto propList = obj.get_derived_type().get_properties();
    for (auto prop : propList)
    {
        if (prop.get_metadata("NO_SERIALIZE"))
            continue;

        variant prop_value = prop.get_value(obj);
        if (!prop_value)
            continue; // cannot serialize, because we cannot retrieve the value

        const auto name = prop.get_name();
        json value;
        reflectionVariantToJson(prop_value, value);

        data[name.to_string()] = std::move(value);
    }
}

json serializeToJson(instance obj)
{
    if (!obj.is_valid())
       return json();

    json j;
    serializeToJson(obj, j);
    return j;
}

}
