/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTile.hpp"

#include "MapUtilExport.hpp"

namespace FreeHeroes {

struct MAPUTIL_EXPORT MapTileArea {
    bool          m_diagonalGrowth = false;
    MapTileRegion m_innerArea;
    MapTileRegion m_innerEdge;   // subset of innerArea;
    MapTileRegion m_outsideEdge; // is not subset of inner area.

    enum class RefineTask
    {
        RemoveHollows,
        RemoveSpikes,
        Expand,
    };

    void makeEdgeFromInnerArea();
    void removeNonInnerFromInnerEdge();

    void makeOutsideEdge();

    void removeEdgeFromInnerArea();

    bool refineEdge(RefineTask task, const MapTileRegion& allowedArea, size_t index);

    bool contains(MapTilePtr cell) const
    {
        return m_innerArea.contains(cell) || m_innerEdge.contains(cell);
    }

    MapTileRegion getBottomEdge() const;

    bool tryGrowOnce(MapTileRegion& lastGrowed, auto&& predicate)
    {
        bool result = false;
        lastGrowed.clear();
        lastGrowed.reserve(m_outsideEdge.size());
        for (auto* pos : m_outsideEdge) {
            if (predicate(pos)) {
                result = true;
                lastGrowed.insert(pos);
            }
        }
        return result;
    }

    MapTileArea floodFillDiagonalByInnerEdge(MapTilePtr cellStart) const;

    std::vector<MapTileArea> splitByFloodFill(bool useDiag, MapTilePtr hint = nullptr) const;
    std::vector<MapTileArea> splitByMaxArea(std::ostream& os, size_t maxArea, bool repulse = false) const;
    std::vector<MapTileArea> splitByK(std::ostream& os, size_t k, bool repulse = false) const;

    enum class CollisionResult
    {
        InvalidInputs,
        NoCollision,
        ImpossibleShift,
        HasShift,
    };

    static MapTilePtr                        makeCentroid(const MapTileRegion& region, bool ensureInbounds = true);
    static MapTileArea                       getInnerBorderNet(const std::vector<MapTileArea>& areas);
    static std::pair<CollisionResult, FHPos> getCollisionShiftForObject(const MapTileRegion& object, const MapTileRegion& obstacle, bool invertObstacle = false);

    static void decompose(MapTileContainer* tileContainer, MapTileRegion& object, MapTileRegion& obstacle, const std::string& serialized, int width, int height);
    static void compose(const MapTileRegion& object, const MapTileRegion& obstacle, std::string& serialized, bool obstacleInverted = false, bool printable = false);
};

}
