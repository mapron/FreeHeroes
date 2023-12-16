/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHTemplate.hpp"

#include "MapTileContainer.hpp"
#include "MapTileRegionWithEdge.hpp"

namespace FreeHeroes {
namespace Core {
class IRandomGenerator;
}

template<class T, T invalid>
struct RegionMapping {
    MapTileRegion              m_all;
    std::map<T, MapTileRegion> m_byLevel;
    std::map<MapTilePtr, T>    m_tileLevels;

    void add(MapTilePtr tile, T level)
    {
        assert(level != invalid);
        m_all.insert(tile);
        if (T oldLevel = getLevel(tile); oldLevel != invalid)
            m_byLevel[oldLevel].erase(tile);
        m_byLevel[level].insert(tile);
        m_tileLevels[tile] = level;
    }

    void addIfNotExist(MapTilePtr tile, T level)
    {
        auto it = m_tileLevels.find(tile);
        if (it != m_tileLevels.cend())
            return;
        add(tile, level);
    }

    T getLevel(MapTilePtr tile) const
    {
        auto it = m_tileLevels.find(tile);
        return it == m_tileLevels.cend() ? invalid : it->second;
    }

    MapTileRegion getRegion(T level) const
    {
        auto it = m_byLevel.find(level);
        return it == m_byLevel.cend() ? MapTileRegion{} : it->second;
    }

    void erase(MapTilePtr tile)
    {
        auto it = m_tileLevels.find(tile);
        if (it == m_tileLevels.cend())
            return;
        auto level = it->second;
        m_tileLevels.erase(it);
        m_byLevel[level].erase(tile);
        m_all.erase(tile);
    }
};

struct TileZone {
    int                          m_index = 0;
    std::string                  m_id;
    FHRngZone                    m_rngZoneSettings;
    Core::IRandomGenerator*      m_rng             = nullptr;
    MapTileContainer*            m_tileContainer   = nullptr;
    Core::LibraryTerrainConstPtr m_terrain         = nullptr;
    Core::LibraryFactionConstPtr m_mainTownFaction = nullptr;
    Core::LibraryFactionConstPtr m_rewardsFaction  = nullptr;
    Core::LibraryFactionConstPtr m_dwellFaction    = nullptr;
    Core::LibraryPlayerConstPtr  m_player          = nullptr;

    MapTilePtr m_startTile = nullptr;
    MapTilePtr m_centroid  = nullptr;

    MapTileRegionWithEdge m_area;

    MapTileRegionWithEdge m_innerAreaUsable;
    MapTileRegion         m_innerAreaTownsBorders;

    class Segment : public MapTileRegionWithEdge
        , public MapTileSegment {
    public:
        Segment() = default;
        explicit Segment(MapTileRegionWithEdge region)
            : MapTileRegionWithEdge(std::move(region))
        {}
    };

    std::vector<Segment> m_innerAreaSegments;
    MapTileRegion        m_innerAreaSegmentsUnited;

    using RoadNetwork = RegionMapping<RoadLevel, RoadLevel::NoRoad>;
    using HeatData    = RegionMapping<int, -1>;

    MapTileRegion m_roadPotentialArea;
    RoadNetwork   m_roads;
    RoadNetwork   m_nodes;

    MapTileRegion m_midTownNodes; //nodes on the middle of roads connecting two towns. (towns.size - 1)
    MapTileRegion m_midExitNodes; //nodes on the middle of roads connecting exit and town. (exits.size)

    MapTileRegion m_protectionBorder; // can not be erased from blocked
    MapTileRegion m_unpassableArea;   // tiles that occupied by towns or scheduled
    MapTileRegion m_needPlaceObstacles;
    MapTileRegion m_needPlaceObstaclesTentative;

    MapTileRegion m_rewardTilesMain;
    MapTileRegion m_rewardTilesDanger;
    MapTileRegion m_rewardTilesSpacing;

    HeatData m_segmentHeat;
    HeatData m_roadHeat;

    std::map<std::string, MapTilePtr> m_namedTiles;

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

    void                      setSegments(MapTileRegionWithEdgeList list);
    MapTileRegionWithEdgeList getSegments() const;
    void                      updateSegmentIndex();

    using TileIntMapping = std::map<MapTilePtr, int>;
    using WeightTileMap  = std::map<int, MapTilePtrList>;

    TileIntMapping makeMoveCosts(bool onlyUsable = true) const;

    static WeightTileMap computeDistances(const TileIntMapping& costs, std::set<MapTilePtr> completed, std::set<MapTilePtr> remaining, int maxCost = -1, int iters = 10000);
};

}
