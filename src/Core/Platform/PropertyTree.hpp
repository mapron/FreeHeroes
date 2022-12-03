/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CorePlatformExport.hpp"

#include <variant>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <iosfwd>
#include <concepts>

namespace FreeHeroes {

template<class T>
concept PropertyTreeScalarHeld = std::integral<T> || std::floating_point<T> || std::is_same_v<T, std::string>;

class COREPLATFORM_EXPORT PropertyTreeScalar {
public:
    PropertyTreeScalar()                          = default;
    PropertyTreeScalar(const PropertyTreeScalar&) = default;
    PropertyTreeScalar& operator=(const PropertyTreeScalar&) = default;

    PropertyTreeScalar(PropertyTreeScalar&&) noexcept = default;
    PropertyTreeScalar& operator=(PropertyTreeScalar&& rh) noexcept = default;

    PropertyTreeScalar(std::integral auto value)
        : m_data(value)
    {}
    PropertyTreeScalar(uint64_t value)
        : m_data(static_cast<int32_t>(value))
    {}
    PropertyTreeScalar(std::floating_point auto value)
        : m_data(value)
    {}
    PropertyTreeScalar(std::string value)
        : m_data(std::move(value))
    {}

    bool operator==(const PropertyTreeScalar& rh) const noexcept { return m_data == rh.m_data; }

    // convert value to standard scalar types. If conversion cannot be made, returns default value.
    bool               toBool() const noexcept;
    const std::string& toString() const noexcept;
    const char*        toCString() const noexcept;
    int64_t            toInt() const noexcept;
    double             toDouble() const noexcept;

    template<std::integral T>
    void convertTo(T& value) const noexcept
    {
        value = static_cast<T>(toInt());
    }
    void convertTo(bool& value) const noexcept
    {
        value = toBool();
    }
    template<std::floating_point T>
    void convertTo(T& value) const noexcept
    {
        value = static_cast<T>(toDouble());
    }
    void convertTo(std::string& value) const noexcept
    {
        value = toString();
    }

    // print any possible value as a string.
    std::string dump() const noexcept;
    void        print(std::ostream& os, bool addQuotes = true, bool escapeString = true) const noexcept;

    bool isNull() const noexcept { return m_data.index() == 0; }
    bool isBool() const noexcept { return m_data.index() == 1; }
    bool isInt() const noexcept { return m_data.index() == 2; }
    bool isDouble() const noexcept { return m_data.index() == 3; }
    bool isString() const noexcept { return m_data.index() == 4; }

private:
    std::variant<std::monostate, bool, int64_t, double, std::string> m_data;
};

class PropertyTree;

using PropertyTreeList      = std::vector<PropertyTree>;
using PropertyTreeMap       = std::map<std::string, PropertyTree>;
using PropertyTreeScalarMap = std::map<std::string, PropertyTreeScalar>;

class COREPLATFORM_EXPORT PropertyTree {
public:
    PropertyTree()                    = default;
    PropertyTree(const PropertyTree&) = default;
    PropertyTree& operator=(const PropertyTree&) = default;

    PropertyTree(PropertyTree&&) noexcept = default;
    PropertyTree& operator=(PropertyTree&& rh) noexcept = default;

    PropertyTree(PropertyTreeScalar scalar)
        : m_data(std::move(scalar))
    {}
    PropertyTree(const PropertyTreeScalarMap& scmap)
    {
        PropertyTreeMap copy;
        for (const auto& p : scmap)
            copy[p.first] = p.second;
        m_data = std::move(copy);
    }
    PropertyTree(const PropertyTreeMap& tmap)
        : m_data(std::move(tmap))
    {
    }

    // check what is inside
    bool isNull() const noexcept { return m_data.index() == 0; }
    bool isScalar() const noexcept { return m_data.index() == 1; }
    bool isList() const noexcept { return m_data.index() == 2; }
    bool isMap() const noexcept { return m_data.index() == 3; }

    // const access to scalar and containers (without checking). May throw.
    const auto& getScalar() const noexcept(false) { return std::get<PropertyTreeScalar>(m_data); }
    const auto& getList() const noexcept(false) { return std::get<PropertyTreeList>(m_data); }
    const auto& getMap() const noexcept(false) { return std::get<PropertyTreeMap>(m_data); }

    auto& getList() noexcept(false) { return std::get<PropertyTreeList>(m_data); }
    auto& getMap() noexcept(false) { return std::get<PropertyTreeMap>(m_data); }

    // checks if container have child object with provided key. returns false if property not a map.
    bool contains(const std::string& key) const noexcept
    {
        if (!isMap())
            return false;
        return getMap().contains(key);
    }

    // get direct access to child value without checking by key.
    const PropertyTree& operator[](const std::string& key) const noexcept(false)
    {
        return std::get<PropertyTreeMap>(m_data).at(key);
    }
    // add new key into map or modify existing one. property will automatically converted to map type.
    PropertyTree& operator[](const std::string& key) noexcept(false)
    {
        if (m_data.index() == 0)
            m_data = PropertyTreeMap{};
        return std::get<PropertyTreeMap>(m_data)[key];
    }
    PropertyTreeScalar value(const std::string& key, PropertyTreeScalar defaultValue) const noexcept(false)
    {
        if (!isMap())
            return defaultValue;
        const auto& map = std::get<PropertyTreeMap>(m_data);
        auto        it  = map.find(key);
        if (it == map.cend())
            return defaultValue;
        return it->second.getScalar();
    }
    bool operator==(const PropertyTree& rh) const noexcept { return m_data == rh.m_data; }

    void append(PropertyTree child) noexcept(false);
    void insert(const std::string& key, PropertyTree child) noexcept(false);

    void convertToList() noexcept(false);
    void convertToMap() noexcept(false);

    struct DumpParams {
        int    m_indentWidth    = 2;
        size_t m_smallArraySize = 10;
        bool   m_isDump         = true;
        bool   m_quoteKeys      = false;
        bool   m_quoteValues    = false;
        bool   m_escapeStrings  = false;
        bool   m_compactArrays  = true;
        bool   m_compactMaps    = true;
    };

    void dump(std::ostream& stream, const DumpParams& params, int level = 0) const noexcept;

public:
    static void mergePatch(PropertyTree& dest, const PropertyTree& source) noexcept(false);
    static void removeEqualValues(PropertyTree& one, PropertyTree& two) noexcept(false);
    static void printReadableJson(std::ostream& stream, const PropertyTree& source) noexcept;

    COREPLATFORM_EXPORT friend std::ostream& operator<<(std::ostream& stream, const PropertyTree& tree);

private:
    using Variant = std::variant<std::monostate, PropertyTreeScalar, PropertyTreeList, PropertyTreeMap>;
    Variant m_data;
};

}
