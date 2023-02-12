/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryObjectDef.hpp"
#include "TranslationMap.hpp"

#include <string>
#include <vector>

namespace FreeHeroes::Core {

struct LibraryResource {
    [[maybe_unused]] static inline constexpr const std::string_view s_treasureChest = "sod.resource.treasureChest";
    [[maybe_unused]] static inline constexpr const std::string_view s_campfire      = "sod.resource.campfire";

    struct Presentation {
        int            orderKingdom = 0;
        int            orderCommon  = 0;
        std::string    icon;
        std::string    iconLarge;
        std::string    iconTrade;
        TranslationMap name;
    };
    enum class Rarity
    {
        Invalid,
        Gold,
        Common,
        Rare,
    };

    enum class SpecialResource
    {
        Invalid,
        TreasureChest,
        CampFire,
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId = -1;

    int    pileSize = 1;
    int    value    = 0;
    Rarity rarity   = Rarity::Invalid;

    ObjectDefMappings objectDefs;
    ObjectDefMappings minesDefs;
    Presentation      presentationParams;

    static inline std::string getSpecialId(SpecialResource type)
    {
        if (type == SpecialResource::TreasureChest)
            return std::string(s_treasureChest);
        if (type == SpecialResource::CampFire)
            return std::string(s_campfire);
        return {};
    }
};

struct ResourceAmount {
    using Map = std::map<LibraryResourceConstPtr, int>;
    Map data;

    void insertMissing(const ResourceAmount& another)
    {
        for (const auto& [res, val] : another.data) {
            if (!data.contains(res))
                data[res] = 0;
        }
    }

    void maxWith(const ResourceAmount& another)
    {
        Map newData = data;
        insertMissing(another);
        for (const auto& [res, val] : data) {
            if (another.data.contains(res))
                newData[res] = std::max(newData[res], another.data.at(res));
        }
        data = std::move(newData);
        removeEmpty();
    }
    void removeEmpty()
    {
        Map newData;
        for (const auto& [res, val] : data) {
            if (val != 0)
                newData[res] = val;
        }
        data = std::move(newData);
    }

    size_t nonEmptyAmount() const noexcept { return data.size(); }

    bool operator==(const ResourceAmount&) const noexcept = default;

    ResourceAmount operator+(const ResourceAmount& rh) const noexcept
    {
        ResourceAmount result = *this;
        result += rh;
        return result;
    }
    ResourceAmount operator-(const ResourceAmount& rh) const noexcept
    {
        ResourceAmount result = *this;
        result -= rh;
        return result;
    }
    ResourceAmount operator-() const noexcept
    {
        ResourceAmount result;
        result -= *this;
        return result;
    }
    ResourceAmount& operator+=(const ResourceAmount& rh) noexcept
    {
        insertMissing(rh);
        for (auto& [res, val] : data) {
            if (rh.data.contains(res))
                val += rh.data.at(res);
        }
        return *this;
    }
    ResourceAmount& operator-=(const ResourceAmount& rh) noexcept
    {
        insertMissing(rh);
        for (auto& [res, val] : data) {
            if (rh.data.contains(res))
                val -= rh.data.at(res);
        }
        return *this;
    }

    std::string toString() const
    {
        std::string result;
        for (auto& [res, val] : data) {
            result += res->id + "=" + std::to_string(val) + ",";
        }
        if (data.empty())
            result = "null";
        return result;
    }

    // lua support.
    void incById(const std::string& id, int incVal)
    {
        for (auto& [res, val] : data) {
            if (res->id == id)
                val += incVal;
        }
    }
};

}
