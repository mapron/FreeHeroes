/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#pragma once

#include <set>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

namespace FreeHeroes {

/// This is not usual flat_set container you probably expect.
/// This class require user to manually call doSort() function they need to access data after any change.
template<class Key>
class FlatSet {
    using List = std::vector<Key>;

public:
    FlatSet() = default;
    explicit FlatSet(List data) noexcept
        : m_data(std::move(data))
    {
        m_sorted = m_data.empty();
        doSort();
    }

    auto begin() const
    {
        ensureSorted();
        return m_data.begin();
    }
    auto end() const { return m_data.end(); }
    auto cbegin() const
    {
        ensureSorted();
        return m_data.cbegin();
    }
    auto cend() const { return m_data.cend(); }

    auto begin()
    {
        ensureSorted();
        return m_data.begin();
    }
    auto end() { return m_data.end(); }
    auto cbegin()
    {
        ensureSorted();
        return m_data.cbegin();
    }
    auto cend() { return m_data.cend(); }

    void clear()
    {
        m_data.clear();
        m_index.clear();
        m_sorted = true;
    }

    bool empty() const noexcept
    {
        return m_data.empty();
    }

    size_t size() const noexcept
    {
        return m_data.size();
    }

    const Key& operator[](size_t offset) const
    {
        return m_data[offset];
    }

    void doSort()
    {
        if (m_sorted)
            return;

        std::set<Key> tmpSet(m_data.begin(), m_data.end());
        m_data.resize(tmpSet.size());
        for (size_t index = 0; const Key& key : tmpSet) {
            m_data[index] = key;
            index++;
        }
        recalcIndex();
        m_sorted = true;
    }
    void insert(const Key& key)
    {
        m_data.push_back(key);
        m_sorted = false;
    }
    void insert(const FlatSet& flatSet)
    {
        m_data.insert(m_data.end(), flatSet.m_data.cbegin(), flatSet.m_data.cend());
        if (!flatSet.empty())
            m_sorted = false;
    }
    void insert(const List& list)
    {
        m_data.insert(m_data.end(), list.cbegin(), list.cend());
        if (!list.empty())
            m_sorted = false;
    }
    void erase(const Key& key)
    {
        if (m_data.empty())
            return;

        auto it = find(key);
        if (it != end()) {
            m_data.erase(it);
            m_sorted = false;
        }
    }
    void erase(const FlatSet& flatSet)
    {
        if (flatSet.empty() || m_data.empty())
            return;

        m_sorted = false;
        List newData;
        newData.reserve(m_data.size());
        for (const Key& key : m_data) {
            if (!flatSet.contains(key))
                newData.push_back(key);
        }
        m_data = std::move(newData);
    }
    void erase(const List& flatList)
    {
        erase(FlatSet(flatList));
    }

    bool contains(const Key& key) const
    {
        ensureSorted();

        return m_index.find(key) != m_index.cend();
    }

private:
    auto find(const Key& key) const
    {
        ensureSorted();

        auto it = m_index.find(key);
        if (it != m_index.cend()) {
            size_t i = it->second;
            return m_data.begin() + i;
        }

        return m_data.end();
    }

    void recalcIndex()
    {
        m_index.clear();
        for (size_t index = 0; index < m_data.size(); ++index)
            m_index[m_data[index]] = index;
    }

    void ensureSorted() const
    {
        if (!m_sorted)
            throw std::runtime_error("doSort() is required for FlatSet.");
    }

private:
    std::unordered_map<Key, size_t> m_index;

    List m_data;
    bool m_sorted = true;
};

}
