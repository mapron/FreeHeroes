/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#pragma once

#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <stdexcept>

#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes {

template<class Key>
class FlatSet {
    using List = std::vector<Key>;

public:
    FlatSet() = default;
    explicit FlatSet(List data) noexcept
        : m_data(std::move(data))
    {
        //Mernel::ProfilerScope scoope2("FlatSet list ctor");
        std::sort(m_data.begin(), m_data.end());
        if (!m_data.empty()) {
            auto resIt   = std::unique(m_data.begin(), m_data.end());
            auto newSize = std::distance(m_data.begin(), resIt);
            m_data.resize(newSize);
        }
        updateIndex();
    }

    bool operator==(const FlatSet&) const = default;

    auto begin() const { return m_data.begin(); }
    auto end() const { return m_data.end(); }
    auto cbegin() const { return m_data.cbegin(); }
    auto cend() const { return m_data.cend(); }

    void clear()
    {
        m_data.clear();
        m_index.clear();
    }

    bool empty() const noexcept { return m_data.empty(); }

    size_t size() const noexcept { return m_data.size(); }

    const Key& operator[](size_t offset) const { return m_data[offset]; }

    void reserve(size_t size) { m_data.reserve(size); }

    void reserveExtra(size_t size)
    {
        m_data.reserve(m_data.size() + size);
    }

    void insert(const Key& key)
    {
        // Mernel::ProfilerScope scoope("insert key");
        auto it = m_index.find(key);
        if (it != m_index.cend())
            return;

        m_index.insert(it, key);
        if (m_data.empty()) {
            m_data.push_back(key);
            return;
        }

        if (key < m_data[0]) {
            m_data.insert(m_data.begin(), key);
            return;
        }
        auto itn = std::upper_bound(m_data.begin(), m_data.end(), key); // data{1,3,5}: key{2}, it => "3"  key{9}, it => "END"
        m_data.insert(itn, key);
    }
    void insert(FlatSet flatSet)
    {
        //Mernel::ProfilerScope scoope("insert set");
        if (flatSet.empty())
            return;

        if (empty()) {
            *this = std::move(flatSet);
            return;
        }

        if (flatSet.size() > size()) { // prefer inserting smaller stuff into larger stuff.
            flatSet.insert(*this);
            *this = std::move(flatSet);
            return;
        }
        if (flatSet.size() <= 16) { // arbitrary chosen "small set to insert" value.

            m_data.reserve(m_data.size() + flatSet.size());
            for (const auto& key : flatSet)
                insert(key);
            return;
        }

        //Mernel::ProfilerScope scoope2("set_union");
        List tmp;
        tmp.resize(m_data.size() + flatSet.size());
        auto resIt   = std::set_union(m_data.cbegin(), m_data.cend(), flatSet.m_data.cbegin(), flatSet.m_data.cend(), tmp.begin());
        auto newSize = std::distance(tmp.begin(), resIt);
        tmp.resize(newSize);
        m_data = std::move(tmp);
        updateIndex();
    }
    void insert(const List& list)
    {
        //Mernel::ProfilerScope scoope("insert list");
        insert(FlatSet(list));
    }
    void erase(const Key& key)
    {
        //Mernel::ProfilerScope scoope("erase key");
        if (m_data.empty())
            return;

        auto itn = m_index.find(key);
        if (itn == m_index.end())
            return;
        m_index.erase(itn);

        auto i = std::lower_bound(m_data.begin(), m_data.end(), key); // data{1,3,5}: key{5}, it => "3"  key{9}, it => "END"
        m_data.erase(i);
    }
    void erase(const FlatSet& flatSet)
    {
        //Mernel::ProfilerScope scoope("erase set");
        if (flatSet.empty() || m_data.empty())
            return;

        for (const auto& key : flatSet)
            erase(key);
    }
    void erase(const List& list)
    {
        //Mernel::ProfilerScope scoope("erase list");
        if (list.empty() || m_data.empty())
            return;

        for (const auto& key : list)
            erase(key);
    }

    FlatSet intersectWith(const FlatSet& other) const
    {
        if (empty() || other.empty())
            return {};

        FlatSet result;
        result.m_data.resize(std::max(m_data.size(), other.size()));
        auto resIt   = std::set_intersection(m_data.cbegin(), m_data.cend(), other.m_data.cbegin(), other.m_data.cend(), result.m_data.begin());
        auto newSize = std::distance(result.m_data.begin(), resIt);
        result.m_data.resize(newSize);
        result.updateIndex();
        return result;
    }

    FlatSet diffWith(const FlatSet& other) const
    {
        if (empty() || other.empty())
            return {};

        FlatSet result;
        result.m_data.resize(std::max(m_data.size(), other.size()));
        auto resIt   = std::set_difference(m_data.cbegin(), m_data.cend(), other.m_data.cbegin(), other.m_data.cend(), result.m_data.begin());
        auto newSize = std::distance(result.m_data.begin(), resIt);
        result.m_data.resize(newSize);
        result.updateIndex();
        return result;
    }

    bool contains(const Key& key) const
    {
        return m_index.contains(key);
    }

    void updateAllValues(auto&& callback)
    {
        for (auto& key : m_data) {
            key = callback(key);
        }
        updateIndex();
    }

private:
    void updateIndex()
    {
        //Mernel::ProfilerScope scoope2("updateIndex");
        m_index.clear();
        m_index.insert(m_data.cbegin(), m_data.cend());
    }

private:
    std::unordered_set<Key> m_index;

    List m_data; // sorted.
};

}
