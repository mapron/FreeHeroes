/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTile.hpp"
#include "MapTileRegion.hpp"

#include "MapUtilExport.hpp"

namespace FreeHeroes {

class MapTileRegionWithEdge;
using MapTileRegionWithEdgeList = std::vector<MapTileRegionWithEdge>;

class MAPUTIL_EXPORT MapTileRegionWithEdge {
public:
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
        return m_innerArea.contains(cell);
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

    MapTileRegion floodFillDiagonalByInnerEdge(MapTilePtr cellStart) const;

    enum class CollisionResult
    {
        InvalidInputs,
        NoCollision,
        ImpossibleShift,
        HasShift,
    };

    static MapTileRegionWithEdgeList         makeEdgeList(const MapTileRegionList& regions);
    static MapTileRegionWithEdge             getInnerBorderNet(const MapTileRegionWithEdgeList& areas);
    static std::pair<CollisionResult, FHPos> getCollisionShiftForObject(const MapTileRegion& object, const MapTileRegion& obstacle, bool invertObstacle = false);

    static void decompose(MapTileContainer* tileContainer, MapTileRegion& object, MapTileRegion& obstacle, const std::string& serialized, int width, int height);
    static void compose(const MapTileRegion& object, const MapTileRegion& obstacle, std::string& serialized, bool obstacleInverted = false, bool printable = false);
};

}
