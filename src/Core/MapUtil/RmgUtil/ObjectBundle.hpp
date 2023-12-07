/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ObjectGenerator.hpp"

#include "MapTileArea.hpp"

namespace FreeHeroes {

struct TileZone;
struct MapTileContainer;
struct ObjectBundle {
    struct Item {
        ObjectGenerator::IObjectPtr m_obj;
        int64_t                     m_guard  = 0;
        MapTilePtr                  m_absPos = nullptr;
    };
    std::vector<Item> m_items;

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
    int64_t    m_guard         = 0;
    bool       m_considerBlock = false;

    ObjectGenerator::IObject::Type m_type = ObjectGenerator::IObject::Type::Visitable;

    MapTileRegion m_rewardArea;
    MapTileRegion m_extraObstacles;

    MapTileRegion m_unpassableArea; // extraObstacles + [rewardArea optional]
    MapTileRegion m_occupiedArea;   // tiles that physically takes place on map (both removable and permanent). reward+obstacles+guard

    MapTileRegion m_dangerZone;             // tiles that under attack of guard but not occupied
    MapTileRegion m_occupiedWithDangerZone; // occupied + danger

    MapTileRegion m_passAroundEdge;
    MapTileRegion m_allArea; // occupied + danger + passAround

    FHPosDirection m_guardPosition = FHPosDirection::B;

    bool m_absPosIsValid = false;

    size_t getEstimatedArea() const { return m_allArea.size(); }

    auto getSortTuple() const { return std::tuple{ getEstimatedArea(), m_absPos }; }

    size_t      m_itemLimit    = 0;
    bool        m_canPushMore  = false;
    int64_t     m_targetGuard  = 0;
    size_t      m_segmentIndex = 0;
    std::string m_repulseId;

    bool estimateOccupied(MapTilePtr absPos, MapTilePtr cetroid);

    bool tryPush(const Item& item);

    std::string toPrintableString() const;
};

class ObjectBundleSet {
public:
    struct BucketItem {
        ObjectGenerator::IObjectPtr m_obj;
        int64_t                     m_guard = 0;
    };

    struct Bucket {
        std::vector<BucketItem> m_guarded;
        std::vector<BucketItem> m_nonGuarded;
    };

    struct ZoneSegment {
        MapTileRegion m_innerEdge;
        MapTileRegion m_cells;
        MapTileRegion m_cellsForUnguardedInner;

        size_t         m_objectCount = 0;
        MapTilePtrList m_centroids;
        MapTilePtr     m_mainCetroid = nullptr;
    };

    struct ConsumeResult {
        std::vector<ObjectBundle> m_bundlesGuarded;
        std::vector<ObjectBundle> m_bundlesNonGuarded;
        std::vector<ObjectBundle> m_bundlesNonGuardedPickable;

        std::map<ObjectGenerator::IObject::Type, Bucket> m_buckets;

        std::vector<ZoneSegment> m_segments;

        MapTileRegion m_cellsForUnguardedRoads;

        size_t m_currentSegment = 0;

        MapTilePtrList m_centroidsALL;
    };
    ConsumeResult m_consumeResult;

    struct Guard {
        int64_t    m_value = 0;
        MapTilePtr m_pos   = nullptr;
        TileZone*  m_zone  = nullptr;
    };
    std::vector<Guard> m_guards;

    Core::IRandomGenerator* m_rng = nullptr;
    MapTileContainer&       m_tileContainer;
    std::ostream&           m_logOutput;

    ObjectBundleSet(Core::IRandomGenerator* const rng, MapTileContainer& tileContainer, std::ostream& logOutput)
        : m_rng(rng)
        , m_tileContainer(tileContainer)
        , m_logOutput(logOutput)
    {
    }

    bool consume(const ObjectGenerator& generated,
                 TileZone&              tileZone);

private:
    bool placeOnMap(ObjectBundle& bundle,
                    ZoneSegment&  currentSegment,
                    bool          useCentroids);
};

}
