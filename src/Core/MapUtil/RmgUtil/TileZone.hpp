/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHTemplate.hpp"

#include "MapTileContainer.hpp"

namespace FreeHeroes {
namespace Core {
class IRandomGenerator;
}

struct TileZone {
    int                          m_index = 0;
    std::string                  m_id;
    FHRngZone                    m_rngZoneSettings;
    Core::IRandomGenerator*      m_rng             = nullptr;
    MapTileContainer*            m_tileContainer   = nullptr;
    Core::LibraryTerrainConstPtr m_terrain         = nullptr;
    Core::LibraryFactionConstPtr m_mainTownFaction = nullptr;
    Core::LibraryFactionConstPtr m_rewardsFaction  = nullptr;
    Core::LibraryPlayerConstPtr  m_player          = nullptr;

    MapTilePtr m_startTile = nullptr;
    MapTilePtr m_centroid  = nullptr;

    MapTileArea m_area;

    MapTileArea   m_innerAreaUsable;
    MapTileRegion m_innerAreaBottomLine;

    std::vector<MapTileArea> m_innerAreaSegments;

    MapTileRegion m_innerAreaSegmentsRoads;
    MapTileRegion m_placedRoads;

    MapTileRegion m_roadNodes;
    MapTileRegion m_roadNodesHighPriority;

    int64_t m_relativeArea   = 0;
    int64_t m_absoluteArea   = 0;
    int64_t m_absoluteRadius = 0;

    int64_t getPlacedArea() const
    {
        return m_area.m_innerArea.size();
    }
    int64_t getAreaDeficit() const
    {
        return m_absoluteArea - getPlacedArea();
    }

    int64_t getAreaDeficitPercent() const
    {
        return getAreaDeficit() * 100 / m_absoluteArea;
    }

    static FHPos makeCentroid(const MapTileRegion& region);

    void estimateCentroid();

    void readFromMap();
    void readFromMapIfDirty();

    bool tryGrowOnceToNeighbour(size_t limit, TileZone* prioritized);

    bool tryGrowOnceToUnzoned();

    void fillDeficit(int thresholdPercent, TileZone* prioritized);

    void fillUnzoned();
};

}
