/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ZoneObject.hpp"

#include "MapTileRegionWithEdge.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct TileZone;
class MapTileContainer;
struct ZoneObjectWrap {
    IZoneObjectPtr m_zoneObject;

    enum class GuardPosition
    {
        TL,
        T,
        TR,
        L,
        R,
        BL,
        B,
        BR,
    };

    MapTilePtr m_absPos      = nullptr;
    MapTilePtr m_guardAbsPos = nullptr;
    FHPos      m_centerOffset;
    bool       m_considerBlock = false;
    bool       m_isGuarded     = false;

    MapTileRegion m_rewardArea;
    MapTileRegion m_extraObstacles;

    MapTileRegion m_unpassableArea; // extraObstacles + [rewardArea optional]
    MapTileRegion m_occupiedArea;   // tiles that physically takes place on map (both removable and permanent). reward+obstacles+guard

    MapTileRegion m_dangerZone;             // tiles that under attack of guard but not occupied
    MapTileRegion m_occupiedWithDangerZone; // occupied + danger

    MapTileRegion m_passAroundEdge;
    MapTileRegion m_allArea; // occupied + danger + passAround

    FHPosDirection m_guardPosition = FHPosDirection::B;

    //MapTileRegion m_lastCellSource;

    bool m_absPosIsValid = false;

    size_t getEstimatedArea() const { return m_occupiedWithDangerZone.size() + m_passAroundEdge.size() / 3; }

    auto getSortTuple() const { return std::tuple{ getEstimatedArea(), m_absPos }; }

    size_t m_segmentIndex  = 0;
    size_t m_sizeInCells   = 0; // 3x3
    int    m_preferredHeat = 0;

    bool estimateOccupied(MapTilePtr absPos, MapTilePtr centroid);
    bool estimateShift(MapTilePtr absPos);
};
using ZoneObjectWrapList = std::vector<ZoneObjectWrap>;

class ZoneObjectDistributor {
public:
    struct ZoneSegment {
        ZoneObjectWrapList m_candidateObjectsNormal;
        ZoneObjectWrapList m_failedObjectsNormal;

        MapTileRegion m_originalArea;
        MapTileRegion m_freeArea;

        MapTilePtr m_originalAreaCentroid = nullptr;

        MapTileRegionList m_originalAreaCells; // 3x3

        std::map<int, size_t> m_freeAreaByHeat;
        size_t                m_freeAreaByHeatTotal = 0;

        bool removeHeatSize(size_t size, int startingHeat);

        int getFreePercent() const { return static_cast<int>(m_freeArea.size() * 100 / m_originalArea.size()); }
    };
    using ZoneSegmentList = std::vector<ZoneSegment>;

    struct Guard {
        int64_t    m_value = 0;
        MapTilePtr m_pos   = nullptr;
    };
    using GuardList = std::vector<Guard>;

    struct DistributionResult {
        ZoneSegmentList    m_segments;
        GuardList          m_guards;
        ZoneObjectWrapList m_candidateObjectsFreePickables;

        std::vector<std::string> m_allOriginalIds; // for checking
        std::vector<std::string> m_placedIds;      // for checking

        void init(TileZone& tileZone);
    };

    ZoneObjectDistributor(Core::IRandomGenerator* const rng, MapTileContainer& tileContainer, std::ostream& logOutput)
        : m_rng(rng)
        , m_tileContainer(tileContainer)
        , m_logOutput(logOutput)
    {
    }

    bool makeInitialDistribution(DistributionResult& distribution, const ZoneObjectGeneration& generated) const;
    bool doPlaceDistribution(DistributionResult& distribution) const;

public:
    enum class PlacementResult
    {
        Success,
        InsufficientSpaceInSource,
        EstimateOccupiedFailure,
        InvalidShiftValue,
        InvalidCollisionInputs,

        CollisionImpossibleShift,
        CollisionHasShift,
        RunOutOfShiftRetries,
        ShiftLoopDetected,
    };

private:
    PlacementResult placeOnMap(ZoneObjectWrap& bundle,
                               MapTileRegion&  region,
                               MapTilePtr      centroid,
                               MapTilePtr      posHint) const;

private:
    const std::string       m_indent = "         ";
    Core::IRandomGenerator* m_rng    = nullptr;
    MapTileContainer&       m_tileContainer;
    std::ostream&           m_logOutput;
};

}
