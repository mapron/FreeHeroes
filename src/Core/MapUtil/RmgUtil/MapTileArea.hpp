/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTile.hpp"
#include "FlatSet.hpp"

namespace FreeHeroes {

using MapTileRegion = FlatSet<MapTilePtr>;

struct MapTileArea {
    bool          m_diagonalGrowth = false;
    MapTileRegion m_innerArea;
    MapTileRegion m_innerEdge;   // subset of innerArea;
    MapTileRegion m_outsideEdge; // is not subset of inner area.

    static void addIf(MapTileRegion& reg, MapTilePtr cell, auto&& predicate)
    {
        if (!cell)
            return;
        if (!predicate(cell))
            return;
        reg.insert(cell);
    }

    void doSort();

    void makeEdgeFromInnerArea();
    void removeNonInnerFromInnerEdge();

    void makeOutsideEdge();

    void removeEdgeFromInnerArea();

    bool contains(MapTilePtr cell) const
    {
        return m_innerArea.contains(cell) || m_innerEdge.contains(cell);
    }

    MapTileRegion getBottomEdge() const;

    bool tryGrowOnce(MapTileRegion& lastGrowed, auto&& predicate)
    {
        bool result = false;
        lastGrowed.clear();
        for (auto* pos : m_outsideEdge) {
            if (predicate(pos)) {
                result = true;
                lastGrowed.insert(pos);
            }
        }
        lastGrowed.doSort();
        return result;
    }

    MapTileArea floodFillDiagonalByInnerEdge(MapTilePtr cellStart) const;

    std::vector<MapTileArea> splitByFloodFill(bool useDiag, MapTilePtr hint = nullptr) const;
    std::vector<MapTileArea> splitByMaxArea(std::ostream& os, size_t maxArea, bool repulse = false) const;
    std::vector<MapTileArea> splitByK(std::ostream& os, size_t k, bool repulse = false) const;

    static MapTileArea getInnerBorderNet(const std::vector<MapTileArea>& areas);
};

}
