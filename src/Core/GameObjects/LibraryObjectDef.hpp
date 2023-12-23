/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <string>
#include <vector>
#include <map>
#include <set>

namespace FreeHeroes::Core {

struct PlanarMask {
    std::vector<std::vector<uint8_t>> m_rows; // rows of cols of bits.  from top left corner, to right and then to bottom.

    size_t m_width  = 0;
    size_t m_height = 0;

    bool operator==(const PlanarMask&) const noexcept = default;
};

struct CombinedMask {
    struct Cell {
        bool m_blocked   = false;
        bool m_visitable = false;

        bool operator==(const Cell&) const noexcept = default;
    };
    struct Point {
        int m_x = 0; // offset from bottom right corner
        int m_y = 0;

        auto operator<=>(const Point&) const noexcept = default;
    };

    std::vector<std::vector<Cell>> m_rows; // rows of cols of cells.  from top left corner, to right and then to bottom.

    std::set<Point> m_blocked;
    std::set<Point> m_visitable;

    size_t m_width  = 0;
    size_t m_height = 0;

    bool operator==(const CombinedMask&) const noexcept = default;
};

inline CombinedMask createOneTileCombinedMask()
{
    Core::CombinedMask result{
        .m_blocked{ { 0, 0 } },
        .m_visitable{ { 0, 0 } },
        .m_width  = 1,
        .m_height = 1,
    };
    result.m_rows.resize(1);
    result.m_rows[0].resize(1);
    result.m_rows[0][0] = Core::CombinedMask::Cell{ .m_blocked = true, .m_visitable = true };

    return result;
};

struct LibraryObjectDef {
    int                  legacyId = -1;
    std::string          id;
    std::string          defFile;
    std::vector<uint8_t> blockMap;
    std::vector<uint8_t> visitMap;
    std::vector<uint8_t> terrainsHard;
    std::vector<uint8_t> terrainsSoft;

    int objId    = -1;
    int subId    = -1;
    int type     = -1;
    int priority = -1;

    using SortingTuple = std::tuple<int, int, int>;
    using SortingMap   = std::map<SortingTuple, LibraryObjectDefConstPtr>;

    constexpr SortingTuple asUniqueTuple() const noexcept { return std::tie(objId, subId, type); }

    constexpr auto asTuple() const noexcept { return std::tie(defFile, blockMap, visitMap, terrainsHard, terrainsSoft, objId, subId, type, priority); }
    bool           operator==(const LibraryObjectDef& rh) const noexcept { return asTuple() == rh.asTuple(); }
    bool           operator!=(const LibraryObjectDef& rh) const noexcept { return asTuple() != rh.asTuple(); }

    LibraryObjectDefConstPtr substituteFor = nullptr;
    std::string              substituteKey;

    std::map<std::string, LibraryObjectDefConstPtr> substitutions; // generated;
    std::set<LibraryTerrainConstPtr>                terrainsSoftCache;

    struct Mappings {
        std::string                 key;
        LibraryArtifactConstPtr     artifact     = nullptr;
        LibraryDwellingConstPtr     dwelling     = nullptr;
        LibraryFactionConstPtr      factionTown  = nullptr;
        LibraryMapBankConstPtr      mapBank      = nullptr;
        LibraryMapObstacleConstPtr  mapObstacle  = nullptr;
        LibraryMapVisitableConstPtr mapVisitable = nullptr;
        LibraryResourceConstPtr     resource     = nullptr;
        LibraryResourceConstPtr     resourceMine = nullptr;
        LibraryUnitConstPtr         unit         = nullptr;
    } mappings; // generated

    PlanarMask blockMapPlanar;
    PlanarMask visitMapPlanar;

    CombinedMask combinedMask;

    LibraryObjectDefConstPtr get(const std::string& substitutionId) const noexcept
    {
        if (substitutionId.empty())
            return this;
        auto it = substitutions.find(substitutionId);
        return it == substitutions.cend() ? this : it->second;
    }
};

struct ObjectDefIndex {
    std::string variant;
    std::string substitution;

    bool isEmpty() const noexcept { return variant.empty() && substitution.empty(); }

    bool operator==(const ObjectDefIndex&) const noexcept = default;
};

struct ObjectDefMappings {
    using Map = std::map<std::string, LibraryObjectDefConstPtr>;

    Map variants;

    LibraryObjectDefConstPtr get(const ObjectDefIndex& index) const noexcept
    {
        auto it = variants.find(index.variant);
        if (it == variants.cend()) {
            return index.variant.empty() ? nullptr : get({ "", index.substitution });
        }
        return it->second->get(index.substitution);
    }
};

}
