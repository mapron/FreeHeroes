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
struct FHMap;
class MapTileContainer;
struct ZoneObjectWrap : public ZoneObjectItem {
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

    MapTilePtr m_absPos        = nullptr;
    MapTilePtr m_guardAbsPos   = nullptr;
    FHPos      m_centerOffset  = g_invalidPos;
    bool       m_considerBlock = false;

    MapTileRegion m_rewardArea{};
    MapTileRegion m_extraObstacles{};

    MapTileRegion m_unpassableArea{}; // extraObstacles + [rewardArea optional]
    MapTileRegion m_occupiedArea{};   // tiles that physically takes place on map (both removable and permanent). reward+obstacles+guard

    MapTileRegion m_dangerZone{};             // tiles that under attack of guard but not occupied
    MapTileRegion m_occupiedWithDangerZone{}; // occupied + danger

    MapTileRegion m_passAroundEdge{};
    MapTileRegion m_allArea{}; // occupied + danger + passAround

    //MapTileRegion m_lastCellSource;

    bool m_absPosIsValid = false;

    //size_t getEstimatedArea() const { return m_occupiedWithDangerZone.size() + m_passAroundEdge.size() / 3; }

    //auto getSortTuple() const { return std::tuple{ getEstimatedArea(), m_absPos }; }

    int    m_placedHeat           = 0;
    size_t m_segmentIndex         = 0;
    size_t m_segmentFragmentIndex = 0;
    size_t m_estimatedArea        = 0;
    //size_t m_sizeInCells          = 0; // 3x3

    bool estimateOccupied(MapTilePtr absPosCenter);

    std::string toPrintableString() const;

    void place() const;
};
using ZoneObjectWrapList    = std::vector<ZoneObjectWrap>;
using ZoneObjectWrapPtrList = std::vector<ZoneObjectWrap*>;

class ZoneObjectDistributor {
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

        Retry,
    };

    struct DistributionResult;
    struct ZoneSegment {
        ZoneObjectWrapPtrList m_candidateObjectsNormal;
        ZoneObjectWrapPtrList m_successNormal;

        MapTileRegion m_originalArea;
        MapTileRegion m_freeArea;

        MapTilePtr m_originalAreaCentroid = nullptr;

        MapTileRegionList m_originalAreaCells; // 3x3

        std::map<int, MapTilePtr> m_heatCentroids;
        std::map<int, size_t>     m_freeAreaByHeat;
        size_t                    m_freeAreaByHeatTotal = 0;

        const TileZone* m_tileZone = nullptr;

        bool m_compact = false;

        size_t m_segmentIndex = 0;

        void removeHeatSize(size_t size, int startingHeat);

        int getFreePercent() const { return static_cast<int>(m_freeArea.size() * 100 / m_originalArea.size()); }

        std::string toPrintableString() const;

        MapTilePtr findBestHeatCentroid(int heat) const;

        void compactIfNeeded();
        void commitAll(FHMap& m_map, DistributionResult& distribution);
        void commitPlacement(FHMap& m_map, DistributionResult& distribution, ZoneObjectWrap* object);
        void recalcFree(ZoneObjectWrap* exclude = nullptr);

        PlacementResult placeOnMap(ZoneObjectWrap& bundle,
                                   MapTilePtr      posHint,
                                   bool            packPlacement);
    };
    using ZoneSegmentList = std::vector<ZoneSegment>;

    struct Guard {
        int64_t    m_value = 0;
        MapTilePtr m_pos   = nullptr;
    };
    using GuardList = std::vector<Guard>;

    struct DistributionResult {
        int m_maxHeat = 0;

        ZoneObjectWrapList m_allObjects;

        ZoneSegmentList m_segments;
        GuardList       m_guards;
        MapTileRegion   m_needBlock;

        std::vector<std::string> m_allOriginalIds; // for checking
        std::vector<std::string> m_placedIds;      // for checking

        ZoneObjectWrapPtrList m_candidateObjectsFreePickables;
        ZoneObjectWrapPtrList m_roadPickables;

        const TileZone* m_tileZone = nullptr;

        void init(TileZone& tileZone);
    };

    ZoneObjectDistributor(FHMap&                        map,
                          Core::IRandomGenerator* const rng,
                          MapTileContainer&             tileContainer,
                          std::ostream&                 logOutput)
        : m_map(map)
        , m_rng(rng)
        , m_tileContainer(tileContainer)
        , m_logOutput(logOutput)
    {
    }

    bool makeInitialDistribution(DistributionResult& distribution, const ZoneObjectGeneration& generated) const;
    bool doPlaceDistribution(DistributionResult& distribution) const;

public:
private:
private:
    const std::string       m_indent = "         ";
    FHMap&                  m_map;
    Core::IRandomGenerator* m_rng = nullptr;
    MapTileContainer&       m_tileContainer;
    std::ostream&           m_logOutput;
};

}
