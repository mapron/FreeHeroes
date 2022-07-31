/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "JsonRTTRDeserialize.hpp"

#include "LibraryIdResolver.hpp"
#include "LibraryReflection.hpp"

#include "Logger.hpp"

#include "PropertyTree.hpp"

#define FH_DEBUG_JSON_DESERIALIZE 1
#ifdef NDEBUG
#undef FH_DEBUG_JSON_DESERIALIZE
#endif

namespace FreeHeroes::Core::Reflection {

using namespace rttr;

#ifdef FH_DEBUG_JSON_DESERIALIZE
std::string jsonToStr(const PropertyTree& jsonObject)
{
    std::ostringstream os;
    os << jsonObject;
    return os.str();
};
#else
std::string jsonToStr(const PropertyTree&)
{
    return "";
};
#endif

variant jsonToReflectionVariant(LibraryIdResolver& idResolver, const type& expectedType, const PropertyTree& scalarValue, bool isOptional = false)
{
    variant     v;
    std::string str;
    assert(scalarValue.isScalar() || scalarValue.isNull());
    if (scalarValue.isScalar()) {
        const PropertyTreeScalar& scalar = scalarValue.getScalar();
        if (scalar.isBool())
            v = scalar.toBool();
        else if (scalar.isInt())
            v = scalar.toInt();
        else if (scalar.isDouble())
            v = scalar.toDouble();
        else if (scalar.isString())
            v = str = scalar.toString();
        else
            assert(scalar.isNull());
    }

    if (LibraryIdResolver::hasResolver(expectedType))
        v = idResolver.resolve(isOptional, str, expectedType);

    if (!v.convert(expectedType))
        throw std::runtime_error("Failed to convert value to type " + std::string{ expectedType.get_name() });
    return v;
}

void jsonArrayToReflectionView(LibraryIdResolver& idResolver, const PropertyTreeList& jsonArray, variant_sequential_view& view)
{
    view.set_size(jsonArray.size());
    const type valueType    = view.get_rank_type(1);
    const bool hasTransform = valueType.get_metadata("transform").is_valid();

    for (size_t i = 0; i < jsonArray.size(); ++i) {
        const PropertyTree& jsonRowValue = jsonArray[i];

        variant elementRef = view.get_value(i);
        if (jsonRowValue.isList()) {
            auto subView = elementRef.create_sequential_view();
            jsonArrayToReflectionView(idResolver, jsonRowValue.getList(), subView);
        } else if (jsonRowValue.isMap() || hasTransform) {
            deserializeFromJson(idResolver, elementRef, jsonRowValue);
        } else {
            elementRef = jsonToReflectionVariant(idResolver, valueType, jsonRowValue.getScalar());
            view.set_value(i, elementRef);
        }
    }
}

void jsonArrayToMapSet(LibraryIdResolver& idResolver, variant_associative_view& view, const PropertyTreeList& jsonObject)
{
    for (size_t i = 0; i < jsonObject.size(); ++i) {
        auto& jsonRow = jsonObject[i];
        if (jsonRow.isMap()) // a key-value associative view
        {
            auto& jsonRowMap = jsonRow.getMap();
            auto  key_itr    = jsonRowMap.find("key");
            auto  value_itr  = jsonRowMap.find("value");

            if (key_itr != jsonRowMap.cend() && value_itr != jsonRowMap.cend()) {
                const PropertyTree& keyJson        = key_itr->second;
                const PropertyTree& valueJson      = value_itr->second;
                variant             extractedKey   = jsonToReflectionVariant(idResolver, view.get_key_type(), keyJson);
                variant             extractedValue = jsonToReflectionVariant(idResolver, view.get_value_type(), valueJson);

                view.insert(extractedKey, extractedValue);
            }
        } else // a key-only associative view
        {
            const PropertyTree& keyJson      = jsonRow;
            variant             extractedKey = jsonToReflectionVariant(idResolver, view.get_key_type(), keyJson);
            view.insert(extractedKey);
        }
    }
}

void deserializeFromJson(LibraryIdResolver& idResolver, instance objOrig, const PropertyTree& jsonObject)
{
    instance                          obj           = objOrig.get_type().get_raw_type().is_wrapper() ? objOrig.get_wrapped_instance() : objOrig;
    const auto                        prop_list     = obj.get_derived_type().get_properties();
    const Reflection::IJsonTransform* transformRoot = nullptr;
    {
        variant metaTransform = obj.get_type().get_metadata("transform");
        if (metaTransform.can_convert<const Reflection::IJsonTransform*>())
            metaTransform.convert(transformRoot);
        metaTransform = obj.get_type().get_raw_type().get_metadata("transform");
        if (metaTransform.can_convert<const Reflection::IJsonTransform*>())
            metaTransform.convert(transformRoot);
    }
    [[maybe_unused]] const std::string jsonObjectStr = jsonToStr(jsonObject);

    const PropertyTreeMap* root = nullptr;
    PropertyTree           transformedRoot;
    if (transformRoot && transformRoot->needTransform(jsonObject)) {
        if (!transformRoot->transform(jsonObject, transformedRoot))
            throw std::runtime_error("Failed to transform value:" + jsonToStr(*root)); // @todo: maybe that's terribly wrong.
        root = &transformedRoot.getMap();
    } else {
        root = &jsonObject.getMap();
    }

    [[maybe_unused]] const std::string typeName{ obj.get_type().get_name() };

    [[maybe_unused]] const std::string transformedRootStr = jsonToStr(transformedRoot);
    for (auto prop : prop_list) {
        auto ret = root->find(prop.get_name().data());
        if (ret == root->cend())
            continue;

        const type valueType   = prop.get_type();
        auto       wrappedType = valueType.is_wrapper() ? valueType.get_wrapped_type() : valueType;
        variant    propVar     = prop.get_value(obj);

        const PropertyTree*                jsonValue    = &ret->second;
        [[maybe_unused]] const std::string jsonValueStr = jsonToStr(*jsonValue);
        PropertyTree                       transformed;
        const Reflection::IJsonTransform*  transformProperty = nullptr;
        {
            variant metaTransformPropType = wrappedType.get_metadata("transform");
            if (metaTransformPropType.can_convert<const Reflection::IJsonTransform*>())
                metaTransformPropType.convert(transformProperty);
        }
        {
            variant metaTransformProp = prop.get_metadata("transform");
            if (!transformProperty && metaTransformProp.can_convert<const Reflection::IJsonTransform*>())
                metaTransformProp.convert(transformProperty);
        }
        if (transformProperty && transformProperty->needTransform(*jsonValue)) {
            if (!transformProperty->transform(*jsonValue, transformed))
                throw std::runtime_error("Failed to transform value:" + jsonToStr(*jsonValue)); // @todo: maybe that's terribly wrong.
            jsonValue = &transformed;
        }
        [[maybe_unused]] const std::string jsonValueStrTrans = jsonToStr(*jsonValue);
        [[maybe_unused]] const std::string propName{ prop.get_name() };

        if (jsonValue->isList()) {
            if (wrappedType.is_sequential_container()) {
                auto view = propVar.create_sequential_view();
                jsonArrayToReflectionView(idResolver, jsonValue->getList(), view);
            } else if (wrappedType.is_associative_container()) {
                auto view = propVar.create_associative_view();

                jsonArrayToMapSet(idResolver, view, jsonValue->getList());
            } else {
                assert(!"Set json array to variant scalar? no way!");
            }

        } else if (jsonValue->isMap()) {
            deserializeFromJson(idResolver, propVar, *jsonValue);
            if (prop.get_metadata("reassign").to_bool())
                prop.set_value(obj, propVar);
        } else if (jsonValue->isNull()) {
            // do nothing
        } else {
            variant extractedValue = jsonToReflectionVariant(idResolver, valueType, *jsonValue, prop.get_metadata("optional").to_bool());
            prop.set_value(obj, extractedValue);
        }
    }
}

}
