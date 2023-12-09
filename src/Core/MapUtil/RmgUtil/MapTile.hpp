/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHPos.hpp"
#include "TemplateUtils.hpp"
#include "FlatSet.hpp"

#include "MapUtilExport.hpp"

#include <vector>

namespace FreeHeroes {

struct TileZone;
struct MapTile;
struct MapTileContainer;
using MapTilePtr           = MapTile*;
using MapTileConstPtr      = const MapTile*;
using MapTilePtrList       = FlatSetUnsortedList<MapTilePtr>;
using MapTilePtrSortedList = FlatSetSortedList<MapTilePtr>;
using MapTileRegion        = FlatSet<MapTilePtr>;

struct MAPUTIL_EXPORT MapTile {
    FHPos m_pos;

    MapTileContainer* m_container    = nullptr;
    TileZone*         m_zone         = nullptr;
    size_t            m_segmentIndex = 0;

    bool m_exFix = false;

    MapTilePtr m_neighborT = nullptr;
    MapTilePtr m_neighborL = nullptr;
    MapTilePtr m_neighborR = nullptr;
    MapTilePtr m_neighborB = nullptr;

    MapTilePtr m_neighborTL = nullptr;
    MapTilePtr m_neighborTR = nullptr;
    MapTilePtr m_neighborBL = nullptr;
    MapTilePtr m_neighborBR = nullptr;

    MapTilePtrSortedList m_orthogonalNeighbours;
    MapTilePtrSortedList m_diagNeighbours;
    MapTilePtrSortedList m_allNeighboursWithDiag;

    struct Transform {
        bool m_transpose = false;
        bool m_flipHor   = false;
        bool m_flipVert  = false;

        FHPos apply(FHPos pos) const noexcept;
    };

    const MapTilePtrSortedList& neighboursList(bool useDiag) const noexcept { return useDiag ? m_allNeighboursWithDiag : m_orthogonalNeighbours; }

    MapTilePtr     neighbourByOffset(FHPos offset) noexcept;
    MapTilePtrList neighboursByOffsets(const std::vector<FHPos>& offsets, const Transform& transform) noexcept;

    std::string toPrintableString() const noexcept;
};

constexpr inline int64_t posDistance(MapTileConstPtr from, MapTileConstPtr to, int64_t mult = 1)
{
    return posDistance(from->m_pos, to->m_pos, mult);
}

}
