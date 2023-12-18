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

    bool refineEdgeRemoveHollows(MapTileRegion& allowedArea);
    bool refineEdgeRemoveSpikes(MapTileRegion& allowedArea);
    bool refineEdgeExpand(MapTileRegion& allowedArea);
    bool refineEdgeShrink(MapTileRegion& allowedArea);

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

    enum class CollisionResult
    {
        InvalidInputs,
        NoCollision,
        ImpossibleShift,
        HasShift,
    };

    static MapTileRegionWithEdgeList         makeEdgeList(const MapTileRegionList& regions);
    static MapTileRegion                     getInnerBorderNet(const MapTileRegionWithEdgeList& areas);
    static MapTileRegion                     getOuterBorderNet(const MapTileRegionWithEdgeList& areas);
    static std::pair<CollisionResult, FHPos> getCollisionShiftForObject(const MapTileRegion& object, const MapTileRegion& obstacle, bool invertObstacle = false);
};

}
