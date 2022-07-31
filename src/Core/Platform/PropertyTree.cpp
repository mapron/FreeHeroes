/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "PropertyTree.hpp"

#include <iostream>

namespace FreeHeroes {

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

std::string PropertyTreeScalar::toString() const noexcept
{
    if (const auto* sval = std::get_if<std::string>(&m_data); sval)
        return *sval;
    return {};
}

const char* PropertyTreeScalar::toCString() const noexcept
{
    if (const auto* sval = std::get_if<std::string>(&m_data); sval)
        return sval->c_str();
    return nullptr;
}

int64_t PropertyTreeScalar::toInt() const noexcept
{
    if (const auto* bval = std::get_if<bool>(&m_data); bval)
        return *bval;
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival)
        return *ival;
    if (const auto* dval = std::get_if<double>(&m_data); dval)
        return static_cast<int64_t>(*dval);
    return 0;
}

double PropertyTreeScalar::toDouble() const noexcept
{
    if (const auto* ival = std::get_if<int64_t>(&m_data); ival)
        return static_cast<double>(*ival);
    if (const auto* dval = std::get_if<double>(&m_data); dval)
        return *dval;
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

void PropertyTree::merge(const PropertyTree& another) noexcept(false)
{
    convertToMap();
    if (!another.isMap())
        return;
    PropertyTreeMap& m = std::get<PropertyTreeMap>(m_data);
    for (const auto& p : another.getMap())
        m[p.first] = p.second;
}

void PropertyTree::mergePatch(const PropertyTree& another) noexcept(false)
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
    if (!another.isMap()) {
        *this = another;
        return;
    }
    if (isNull())
        convertToMap();

    if (!isMap()) {
        *this = {};
        return;
    }
    auto& selfMap = getMap();
    for (const auto& [key, value] : another.getMap()) {
        // if Value is null:
        //   if Name exists in Target:
        //     remove the Name/Value pair from Target
        //   else:
        //     Target[Name] = MergePatch(Target[Name], Value)
        if (value.isNull())
            selfMap.erase(key);
        else
            selfMap[key].mergePatch(value);
    }
}

void PropertyTree::removeEqualValues(const PropertyTree& another) noexcept(false)
{
    convertToMap();
    if (!another.isMap())
        return;
    PropertyTreeMap& m = std::get<PropertyTreeMap>(m_data);
    for (const auto& p : another.getMap()) {
        if (m.contains(p.first) && m[p.first] == p.second)
            m.erase(p.first);
    }
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

void PropertyTree::dump(std::ostream& stream, int level) const noexcept
{
    const std::string pad(level * 2, ' ');
    if (isNull()) {
        stream << pad << "NULL\n";
        return;
    }
    if (isScalar()) {
        stream << pad << getScalar().dump() << '\n';
        return;
    }
    if (isList()) {
        stream << pad << '[' << '\n';
        for (const auto& child : getList()) {
            child.dump(stream, level + 1);
        }
        stream << pad << ']' << '\n';
        return;
    }
    stream << pad << '{' << '\n';
    for (const auto& [key, child] : getMap()) {
        if (child.isScalar()) {
            stream << pad << key << ": " << child.getScalar().dump() << '\n';
            continue;
        }
        stream << pad << key << ":" << '\n';
        child.dump(stream, level + 1);
    }
    stream << pad << '}' << '\n';
}

std::ostream& operator<<(std::ostream& stream, const PropertyTree& tree)
{
    tree.dump(stream, 0);
    return stream;
}

}
