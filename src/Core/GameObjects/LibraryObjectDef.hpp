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

    struct PlanarMask {
        std::vector<std::vector<uint8_t>> data; // rows of cols of bits.  from top left corner, to right and then to bottom.

        size_t width  = 0;
        size_t height = 0;

        bool operator==(const PlanarMask&) const noexcept = default;
    };

    PlanarMask blockMapPlanar;
    PlanarMask visitMapPlanar;

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
