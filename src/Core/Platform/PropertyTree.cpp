/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "PropertyTree.hpp"

#include <iostream>

namespace FreeHeroes {

namespace {
static const std::string s_empty;

static inline char toHex(uint8_t c)
{
    return (c <= 9) ? '0' + c : 'a' + c - 10;
}

void printJsonString(std::ostream& os, const std::string& value, bool addQuotes, bool escapeString)
{
    if (addQuotes)
        os << '"';
    if (escapeString) {
        // @note: while useful for debugging pupose, pretty-print is not handling utf-8.
        // \r\n => 0x0D 0x0A => \u000d \u000a
        for (char c : value) {
            uint8_t byte = c;
            if (byte < 0x20) {
                os << "\\u00" << toHex(byte / 16) << toHex(byte % 16);
            } else {
                os << c;
            }
        }
    } else {
        os << value;
    }
    if (addQuotes)
        os << '"';
}
}

bool PropertyTreeScalar::toBool() const noexcept
{
    if (const auto* bval = std::get_if<bool>(&m_data); bval)
        return *bval;
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival)
        return *ival;
    if (const auto* dval = std::get_if<double>(&m_data); dval)
        return *dval;
    return false;
}

const std::string& PropertyTreeScalar::toString() const noexcept
{
    if (const auto* sval = std::get_if<std::string>(&m_data); sval)
        return *sval;
    return s_empty;
}

const char* PropertyTreeScalar::toCString() const noexcept
{
    if (const auto* sval = std::get_if<std::string>(&m_data); sval)
        return sval->c_str();
    return nullptr;
}

int64_t PropertyTreeScalar::toInt() const noexcept
{
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival)
        return *ival;
    if (const auto* bval = std::get_if<bool>(&m_data); bval)
        return *bval;
    if (const auto* dval = std::get_if<double>(&m_data); dval)
        return static_cast<int64_t>(*dval);
    return 0;
}

double PropertyTreeScalar::toDouble() const noexcept
{
    if (const auto* dval = std::get_if<double>(&m_data); dval)
        return *dval;
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival)
        return static_cast<double>(*ival);
    return 0.;
}

std::string PropertyTreeScalar::dump() const noexcept
{
    if (const auto* bval = std::get_if<bool>(&m_data); bval)
        return std::string(*bval ? "true" : "false");
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival)
        return std::to_string(*ival);
    if (const auto* dval = std::get_if<double>(&m_data); dval)
        return std::to_string(*dval) + "f";
    if (const auto* sval = std::get_if<std::string>(&m_data); sval)
        return "'" + *sval + "'";
    return "null";
}

void PropertyTreeScalar::print(std::ostream& os, bool addQuotes, bool escapeString) const noexcept
{
    if (const auto* bval = std::get_if<bool>(&m_data); bval) {
        os << (*bval ? "true" : "false");
        return;
    }
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival) {
        os << *ival;
        return;
    }

    if (const auto* dval = std::get_if<double>(&m_data); dval) {
        os << *dval;
        return;
    }
    if (const auto* sval = std::get_if<std::string>(&m_data); sval) {
        printJsonString(os, *sval, addQuotes, escapeString);
        return;
    }
    os << "null";
}

void PropertyTree::append(PropertyTree child)
{
    convertToList();
    PropertyTreeList& l = std::get<PropertyTreeList>(m_data);
    l.push_back(std::move(child));
}

void PropertyTree::insert(const std::string& key, PropertyTree child)
{
    convertToMap();
    PropertyTreeMap& m = std::get<PropertyTreeMap>(m_data);
    m[key]             = std::move(child);
}

void PropertyTree::convertToList() noexcept(false)
{
    if (m_data.index() == 0)
        m_data = PropertyTreeList{};
    assert(isList());
}

void PropertyTree::convertToMap() noexcept(false)
{
    if (m_data.index() == 0)
        m_data = PropertyTreeMap{};
    assert(isMap());
}

void PropertyTree::dump(std::ostream& stream, const DumpParams& params, int level) const noexcept
{
    auto simplePrint = [&params, &stream](const PropertyTree& value) {
        if (value.isNull()) {
            stream << (params.m_isDump ? "NULL" : "null");
            return;
        }
        const auto& scalar = value.getScalar();
        if (params.m_isDump) {
            stream << scalar.dump();
            return;
        }
        scalar.print(stream, params.m_quoteValues, params.m_escapeStrings);
    };

    auto checkFlatList = [&params](const PropertyTreeList& list) -> bool {
        if (!params.m_compactArrays)
            return false;
        for (const auto& child : list) {
            if (child.isList() || child.isMap()) {
                return false;
            }
        }
        return true;
    };
    auto checkFlatMap = [&checkFlatList, &params](const PropertyTreeMap& map) -> bool {
        if (!params.m_compactMaps)
            return false;
        for (const auto& [key, child] : map) {
            if (child.isList()) {
                if (child.getList().size() > params.m_smallArraySize || !checkFlatList(child.getList())) {
                    return false;
                }
            }
            if (child.isMap())
                return false;
        }
        return true;
    };
    const std::string padBase((level) *params.m_indentWidth, ' ');
    const std::string padNext((level + 1) * params.m_indentWidth, ' ');

    if (isNull() || isScalar()) {
        stream << padBase;
        simplePrint(*this);
        return;
    }
    if (isList()) {
        const bool isFlatList = checkFlatList(getList());
        stream << padBase << '[' << (isFlatList ? "" : "\n");
        for (size_t index = 0, count = getList().size(); const auto &child : getList()) {
            //const bool simpleValue = isFlatList || child.isNull() || child.isScalar();
            child.dump(stream, params, isFlatList ? 0 : level + 1);

            if (index < count - 1)
                stream << ", ";
            if (!isFlatList)
                stream << '\n';
            index++;
        }
        stream << (isFlatList ? "" : padBase) << ']';
        return;
    }
    const bool isFlatMap = checkFlatMap(getMap());
    stream << padBase << '{' << (isFlatMap ? "" : "\n");

    for (size_t index = 0, count = getMap().size(); const auto& [key, child] : getMap()) {
        const bool simpleValue = isFlatMap || child.isNull() || child.isScalar();
        if (!isFlatMap)
            stream << padNext;
        printJsonString(stream, key, params.m_quoteKeys, params.m_escapeStrings);
        stream << ": " << (simpleValue ? "" : "\n");
        child.dump(stream, params, simpleValue ? 0 : level + 2);
        if (index < count - 1)
            stream << ", ";
        if (!isFlatMap)
            stream << '\n';
        index++;
    }
    stream << (isFlatMap ? "" : padBase) << '}';
}

void PropertyTree::mergePatch(PropertyTree& dest, const PropertyTree& source) noexcept(false)
{
    // https://tools.ietf.org/html/rfc7396
    //	define MergePatch(Target, Patch):
    //	  if Patch is an Object:
    //		if Target is not an Object:
    //		  Target = {} // Ignore the contents and set it to an empty Object
    //		for each Name/Value pair in Patch:
    //		  if Value is null:
    //			if Name exists in Target:
    //			  remove the Name/Value pair from Target
    //		  else:
    //			Target[Name] = MergePatch(Target[Name], Value)
    //		return Target
    //	  else:
    //		return Patch
    if (!source.isMap()) {
        dest = source;
        return;
    }
    if (dest.isNull())
        dest.convertToMap();

    if (!dest.isMap()) {
        dest = {};
        return;
    }
    auto& destMap = dest.getMap();
    for (const auto& [key, value] : source.getMap()) {
        // if Value is null:
        //   if Name exists in Target:
        //     remove the Name/Value pair from Target
        //   else:
        //     Target[Name] = MergePatch(Target[Name], Value)
        if (value.isNull())
            destMap.erase(key);
        else
            mergePatch(destMap[key], value);
    }
}

void PropertyTree::removeEqualValues(PropertyTree& one, PropertyTree& two) noexcept(false)
{
    if (one == two) {
        one = {};
        two = {};
        return;
    }

    if (one.isMap()) {
        if (!two.isMap())
            return;
        auto& oneMap = one.getMap();
        auto& twoMap = two.getMap();

        std::vector<std::string> commonKeys;
        for (auto&& [key, value] : oneMap) {
            if (!twoMap.contains(key))
                continue;
            commonKeys.push_back(key);
        }
        for (const auto& key : commonKeys) {
            auto& oneMapValue = oneMap[key];
            auto& twoMapValue = twoMap[key];
            removeEqualValues(oneMapValue, twoMapValue);
            if (oneMapValue.isNull() && twoMapValue.isNull()) {
                oneMap.erase(key);
                twoMap.erase(key);
            }
        }
    }
    if (one.isList()) {
        if (!two.isList())
            return;

        auto& oneList = one.getList();
        auto& twoList = two.getList();

        const auto minSize = std::min(oneList.size(), twoList.size());
        for (size_t i = 0; i < minSize; ++i) {
            auto& oneListValue = oneList[i];
            auto& twoListValue = twoList[i];
            removeEqualValues(oneListValue, twoListValue);
        }
    }
}

void PropertyTree::printReadableJson(std::ostream& stream, const PropertyTree& source) noexcept
{
    source.dump(stream, DumpParams{
                            .m_indentWidth   = 4,
                            .m_isDump        = false,
                            .m_quoteKeys     = true,
                            .m_quoteValues   = true,
                            .m_escapeStrings = true,
                        },
                0);
    stream << '\n';
}

std::ostream& operator<<(std::ostream& stream, const PropertyTree& tree)
{
    tree.dump(stream, PropertyTree::DumpParams{}, 0);
    stream << '\n';
    return stream;
}

}
