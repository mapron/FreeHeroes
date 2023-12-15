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
class FlatSetUnsortedList : public std::vector<Key> {
public:
    using std::vector<Key>::vector;
    /*implicit*/ FlatSetUnsortedList(const std::vector<Key>& data)
        : std::vector<Key>(data)
    {}
    /*implicit*/ FlatSetUnsortedList(std::vector<Key>&& data)
        : std::vector<Key>(std::move(data))
    {}
};

template<class Key>
class FlatSetSortedList : public std::vector<Key> {
public:
    using SortedList = FlatSetSortedList<Key>;
    using std::vector<Key>::vector;
    /*implicit*/ FlatSetSortedList(const std::vector<Key>& data)
        : std::vector<Key>(data)
    {
        ensureSorted();
    }
    /*implicit*/ FlatSetSortedList(std::vector<Key>&& data)
        : std::vector<Key>(std::move(data))
    {
        ensureSorted();
    }

    void ensureSorted()
    {
        std::sort(this->begin(), this->end());
        if (!this->empty()) {
            const auto resIt   = std::unique(this->begin(), this->end());
            const auto newSize = std::distance(this->begin(), resIt);
            this->resize(newSize);
        }
    }

    SortedList unionWith(const SortedList& another) const
    {
        SortedList tmp;
        tmp.resize(this->size() + another.size());
        auto resIt   = std::set_union(this->cbegin(), this->cend(), another.cbegin(), another.cend(), tmp.begin());
        auto newSize = std::distance(tmp.begin(), resIt);
        tmp.resize(newSize);
        return tmp;
    }
    SortedList intersectWith(const SortedList& another) const
    {
        SortedList tmp;
        tmp.resize(std::max(this->size(), another.size()));
        auto resIt   = std::set_intersection(this->cbegin(), this->cend(), another.cbegin(), another.cend(), tmp.begin());
        auto newSize = std::distance(tmp.begin(), resIt);
        tmp.resize(newSize);
        return tmp;
    }
    SortedList diffWith(const SortedList& another) const
    {
        SortedList tmp;
        tmp.resize(std::max(this->size(), another.size()));
        auto resIt   = std::set_difference(this->cbegin(), this->cend(), another.cbegin(), another.cend(), tmp.begin());
        auto newSize = std::distance(tmp.begin(), resIt);
        tmp.resize(newSize);
        return tmp;
    }
};

template<class Key>
class FlatSet {
    using UnsortedList = FlatSetUnsortedList<Key>;
    using SortedList   = FlatSetSortedList<Key>;

public:
    FlatSet() = default;
    explicit FlatSet(SortedList data) noexcept
        : m_data(std::move(data))
    {
    }
    explicit FlatSet(UnsortedList data) noexcept
        : m_data(std::move(data))
    {
    }

    bool operator==(const FlatSet&) const = default;

    auto begin() const { return m_data.begin(); }
    auto end() const { return m_data.end(); }
    auto cbegin() const { return m_data.cbegin(); }
    auto cend() const { return m_data.cend(); }

    void clear()
    {
        m_data.clear();
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
        if (m_data.empty()) {
            m_data.push_back(key);
            return;
        }

        auto it = std::lower_bound(m_data.begin(), m_data.end(), key);
        if (it != m_data.end() && !(key < *it))
            return;

        m_data.insert(it, key);
    }
    void insert(const SortedList& list)
    {
        if (list.empty())
            return;

        if (empty()) {
            m_data = list;
            return;
        }
        if (list.size() <= 16) { // arbitrary chosen "small set to insert" value.
            m_data.reserve(m_data.size() + list.size());
            for (const auto& key : list)
                insert(key);
            return;
        }
        m_data = m_data.unionWith(list);
    }
    void insert(const UnsortedList& list)
    {
        insert(SortedList(list));
    }
    void insert(FlatSet flatSet)
    {
        if (flatSet.size() > size()) { // prefer inserting smaller stuff into larger stuff.
            flatSet.insert(*this);
            *this = std::move(flatSet);
            return;
        }

        insert(flatSet.m_data);
    }

    void erase(const Key& key)
    {
        if (m_data.empty())
            return;

        auto i = binarySearch(key);
        if (i != m_data.end())
            m_data.erase(i);
    }
    void erase(const SortedList& list)
    {
        if (list.empty() || m_data.empty())
            return;

        if (list.size() <= 16) { // arbitrary chosen "small set to erase" value.
            for (const auto& key : list)
                erase(key);
            return;
        }
        m_data = m_data.diffWith(list);
    }
    void erase(const UnsortedList& list)
    {
        if (list.empty() || m_data.empty())
            return;

        erase(SortedList(list));
    }

    void erase(const FlatSet& flatSet)
    {
        erase(flatSet.m_data);
    }

    FlatSet unionWith(const FlatSet& other) const
    {
        if (empty())
            return other;
        if (other.empty())
            return *this;

        FlatSet result;
        result.m_data = m_data.unionWith(other.m_data);
        return result;
    }

    FlatSet intersectWith(const FlatSet& other) const
    {
        if (empty() || other.empty())
            return {};

        FlatSet result;
        result.m_data = m_data.intersectWith(other.m_data);
        return result;
    }

    FlatSet diffWith(const FlatSet& other) const
    {
        if (empty())
            return {};
        if (other.empty())
            return *this;

        FlatSet result;
        result.m_data = m_data.diffWith(other.m_data);
        return result;
    }

    bool contains(const Key& key) const
    {
        return std::binary_search(m_data.cbegin(), m_data.cend(), key);
    }

    /// @todo: UNTESTED!
    bool contains(const SortedList& list) const
    {
        return m_data.intersectWith(list).size() == list.size();
    }

    void updateAllValues(auto&& callback)
    {
        for (auto& key : m_data) {
            key = callback(key);
        }
    }

private:
    auto binarySearch(const Key& key)
    {
        auto it = std::lower_bound(m_data.begin(), m_data.end(), key);
        if (it != m_data.end() && !(key < *it))
            return it;
        else
            return m_data.end();
    }

private:
    SortedList m_data;
};

}
