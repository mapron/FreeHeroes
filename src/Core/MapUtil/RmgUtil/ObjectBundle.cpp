/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ObjectBundle.hpp"
#include "TileZone.hpp"
#include "KMeans.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>
#include <sstream>

namespace FreeHeroes {

namespace {

const std::vector<ObjectBundle::GuardPosition> g_allPositions{
    ObjectBundle::GuardPosition::TL,
    ObjectBundle::GuardPosition::T,
    ObjectBundle::GuardPosition::TR,
    ObjectBundle::GuardPosition::L,
    ObjectBundle::GuardPosition::R,
    ObjectBundle::GuardPosition::BL,
    ObjectBundle::GuardPosition::B,
    ObjectBundle::GuardPosition::BR
};

// clang-format off
const std::vector<FHPos> g_deltasToTry{ 
    FHPos{}, 
    FHPos{ -1,  0 }, FHPos{ +1,  0 }, FHPos{  0, -1 }, FHPos{  0, +1 },
    FHPos{ -2,  0 }, FHPos{ +2,  0 }, FHPos{  0, -2 }, FHPos{  0, +2 },
    FHPos{ -1, -1 }, FHPos{ +1, -1 }, FHPos{ +1, +1 }, FHPos{ -1, +1 },
    FHPos{ -2, -2 }, FHPos{ +2, -2 }, FHPos{ +2, +2 }, FHPos{ -2, +2 },
    FHPos{ -3,  0 }, FHPos{ +3,  0 }, FHPos{  0, -3 }, FHPos{  0, +3 },
};
// clang-format on

MapTileRegion blurSet(const MapTileRegion& source, bool diag, bool excludeOriginal = true)
{
    MapTileRegion result;
    for (auto pos : source) {
        result.insert(pos);
        result.insert(pos->neighboursList(diag));
    }
    result.doSort();
    if (excludeOriginal) {
        result.erase(source);
        result.doSort();
    }
    return result;
}
MapTileRegion blurSet(MapTileRegion&& source, bool diag, bool excludeOriginal = true)
{
    source.doSort();
    return blurSet(source, diag, excludeOriginal);
}

}

void ObjectBundle::estimateOccupied()
{
    Mernel::ProfilerScope scope("estimateOccupied");
    m_estimatedOccupied.clear();
    m_protectionBorder.clear();
    m_blurForPassable.clear();
    m_allArea.clear();
    m_fitArea.clear();
    m_guardRegion.clear();
    m_absPosIsValid = false;
    if (!m_absPos)
        return;

    if (m_type == ObjectGenerator::IObject::Type::Pickable) {
        size_t       itemCount      = m_items.size();
        const size_t maxRowSize     = 2;
        const size_t itemRectHeight = (itemCount + maxRowSize - 1) / maxRowSize;
        const size_t itemRectWidth  = std::min(maxRowSize, itemCount);
        for (size_t index = 0; auto& item : m_items) {
            FHPos itemOffset;

            itemOffset.m_y -= index / itemRectWidth;
            itemOffset.m_x -= index % itemRectWidth;
            auto visualPos = m_absPos->neighbourByOffset(itemOffset);
            if (!visualPos)
                return;

            m_estimatedOccupied.insert(visualPos);
            item.m_absPos = visualPos->neighbourByOffset(item.m_obj->getOffset());
            if (!item.m_absPos)
                return;

            index++;
        }

        if (m_guard) {
            FHPos guardOffset;

            if (m_guardPosition == GuardPosition::B) {
                guardOffset.m_y++;
            }
            if (m_guardPosition == GuardPosition::BR) {
                guardOffset.m_y++;
                guardOffset.m_x++;
            }
            if (m_guardPosition == GuardPosition::BL) {
                guardOffset.m_y++;
                guardOffset.m_x -= itemRectWidth;
            }
            if (m_guardPosition == GuardPosition::R) {
                guardOffset.m_x++;
            }
            if (m_guardPosition == GuardPosition::TR) {
                guardOffset.m_x++;
                guardOffset.m_y -= itemRectHeight;
            }
            if (m_guardPosition == GuardPosition::T) {
                guardOffset.m_y -= itemRectHeight;
            }
            if (m_guardPosition == GuardPosition::TL) {
                guardOffset.m_y -= itemRectHeight;
                guardOffset.m_x -= itemRectWidth;
            }
            if (m_guardPosition == GuardPosition::L) {
                guardOffset.m_x -= itemRectWidth;
            }

            m_guardAbsPos = m_absPos->neighbourByOffset(guardOffset);
            if (!m_guardAbsPos)
                return;

            m_guardRegion = blurSet(MapTileRegion({ m_guardAbsPos }), true, false);

            m_estimatedOccupied.doSort();
            m_protectionBorder = blurSet(m_estimatedOccupied, true);
            m_protectionBorder.erase(m_guardRegion);
            m_protectionBorder.doSort();
        }
    }
    if (m_type == ObjectGenerator::IObject::Type::Visitable) {
        MapTilePtr mainPos = m_absPos;
        for (auto& item : m_items) {
            item.m_absPos = m_absPos;
            auto* def     = item.m_obj->getDef();
            assert(def);

            for (auto&& point : def->combinedMask.m_blocked) {
                auto occPos = m_absPos->neighbourByOffset(FHPos{ point.m_x, point.m_y });
                if (!occPos)
                    return;
                m_estimatedOccupied.insert(occPos);
            }
            for (auto&& point : def->combinedMask.m_visitable) {
                mainPos = m_absPos->neighbourByOffset(FHPos{ point.m_x, point.m_y });
                if (!mainPos)
                    return;
            }
        }

        if (m_guard) {
            FHPos guardOffset;
            guardOffset.m_y++;
            if (m_guardPosition == GuardPosition::TL || m_guardPosition == GuardPosition::L || m_guardPosition == GuardPosition::BL)
                guardOffset.m_x--;
            if (m_guardPosition == GuardPosition::TR || m_guardPosition == GuardPosition::R || m_guardPosition == GuardPosition::BR)
                guardOffset.m_x++;
            m_guardAbsPos = mainPos->neighbourByOffset(guardOffset);
            if (!m_guardAbsPos)
                return;
            m_guardRegion = blurSet(MapTileRegion({ m_guardAbsPos }), true, false);
        }
    }

    if (m_guard) {
        if (!m_considerBlock)
            m_estimatedOccupied.insert(m_guardAbsPos);
        else
            m_estimatedOccupied.insert(m_guardRegion);
    }

    m_estimatedOccupied.doSort();

    if (m_guard || m_type == ObjectGenerator::IObject::Type::Visitable) {
        auto current = m_estimatedOccupied;
        current.insert(m_protectionBorder);
        current.doSort();

        m_blurForPassable = blurSet(current, false);
    }
    m_allArea.insert(m_protectionBorder);
    m_allArea.insert(m_blurForPassable);
    m_allArea.insert(m_estimatedOccupied);
    m_allArea.doSort();

    m_fitArea.insert(m_protectionBorder);
    m_fitArea.insert(m_estimatedOccupied);
    m_fitArea.insert(m_guardRegion);
    m_fitArea.doSort();

    m_absPosIsValid = true;
}

bool ObjectBundle::tryPush(const Item& item)
{
    if (m_items.size() >= m_itemLimit)
        return false;

    int64_t newGuard = m_guard + item.m_guard;

    if (!m_items.empty()) {
        if (newGuard > m_targetGuard)
            return false;
    }
    if (item.m_obj->preventDuplicates()) {
        for (auto& existingItem : m_items) {
            if (existingItem.m_obj->getId() == item.m_obj->getId())
                return false;
        }
    }
    if (!m_repulseId.empty() && !item.m_obj->getRepulseId().empty())
        return false;
    m_guard     = newGuard;
    m_repulseId = item.m_obj->getRepulseId();

    m_items.push_back(item);
    return true;
}

std::string ObjectBundle::toPrintableString() const
{
    std::ostringstream os;
    os << "[";
    for (auto& item : m_items)
        os << item.m_obj->getId() << ", ";
    os << "]";
    return os.str();
}

bool ObjectBundleSet::consume(const ObjectGenerator& generated,
                              TileZone&              tileZone)
{
    Mernel::ProfilerScope scope("consume");

    m_consumeResult           = {};
    m_guards                  = {};
    MapTileRegion safePadding = tileZone.m_roadNodesHighPriority;
    {
        auto guards = tileZone.m_breakGuardTiles;
        guards.doSort();
        guards = blurSet(guards, true, false);
        safePadding.insert(guards);
        safePadding.doSort();
        safePadding = blurSet(safePadding, true, false);
    }

    for (auto& seg : tileZone.m_innerAreaSegments) {
        ZoneSegment zs;
        zs.m_cells = seg.m_innerArea;
        zs.m_cells.doSort();
        zs.m_cells.erase(safePadding);
        zs.m_cells.doSort();
        zs.m_cellsForUnguardedInner = zs.m_cells;
        m_consumeResult.m_segments.push_back(std::move(zs));
    }

    m_consumeResult.m_cellsForUnguardedRoads = tileZone.m_placedRoads;
    m_consumeResult.m_cellsForUnguardedRoads.erase(safePadding);
    m_consumeResult.m_cellsForUnguardedRoads.doSort();

    for (const auto& group : generated.m_groups) {
        for (const auto& obj : group.m_objects) {
            BucketItem item;
            item.m_guard = obj->getGuard() * group.m_guardPercent / 100;
            item.m_obj   = obj;

            auto& buck = m_consumeResult.m_buckets[obj->getType()];
            if (item.m_guard)
                buck.m_guarded.push_back(item);
            else
                buck.m_nonGuarded.push_back(item);
        }
    }

    auto makeNewObjectBundle = [this, &tileZone](ObjectGenerator::IObject::Type type) -> ObjectBundle {
        ObjectBundle obj;
        int64_t      min  = tileZone.m_rngZoneSettings.m_guardMin;
        int64_t      max  = tileZone.m_rngZoneSettings.m_guardMax;
        obj.m_targetGuard = min + m_rng->gen(max - min);
        obj.m_itemLimit   = 1;
        if (type == ObjectGenerator::IObject::Type::Pickable)
            obj.m_itemLimit += m_rng->genSmall(3);

        obj.m_type          = type;
        obj.m_guardPosition = g_allPositions[m_rng->genSmall(g_allPositions.size() - 1)];

        return obj;
    };

    for (const auto& [type, bucket] : m_consumeResult.m_buckets) {
        for (const auto& item : bucket.m_nonGuarded) {
            ObjectBundle bundleNonGuarded;
            bundleNonGuarded.m_items.push_back(ObjectBundle::Item{ .m_obj = item.m_obj });
            bundleNonGuarded.m_type   = type;
            bundleNonGuarded.m_absPos = m_tileContainer.m_centerTile;
            bundleNonGuarded.estimateOccupied();
            assert(bundleNonGuarded.m_absPosIsValid);

            if (type == ObjectGenerator::IObject::Type::Pickable)
                m_consumeResult.m_bundlesNonGuardedPickable.push_back(std::move(bundleNonGuarded));
            else
                m_consumeResult.m_bundlesNonGuarded.push_back(std::move(bundleNonGuarded));
        }

        ObjectBundle bundleGuarded = makeNewObjectBundle(type);

        auto pushIfNeeded = [this, &tileZone, &bundleGuarded, &makeNewObjectBundle]() {
            bundleGuarded.m_considerBlock = bundleGuarded.m_guard > tileZone.m_rngZoneSettings.m_guardBlock;
            bundleGuarded.m_absPos        = m_tileContainer.m_centerTile;
            bundleGuarded.estimateOccupied();
            assert(bundleGuarded.m_absPosIsValid);

            m_consumeResult.m_bundlesGuarded.push_back(bundleGuarded);
            bundleGuarded = makeNewObjectBundle(bundleGuarded.m_type);
        };

        for (const auto& item : bucket.m_guarded) {
            auto newItem = ObjectBundle::Item{ .m_obj = item.m_obj, .m_guard = item.m_guard };
            if (bundleGuarded.tryPush(newItem))
                continue;
            pushIfNeeded();

            if (!bundleGuarded.tryPush(newItem))
                throw std::runtime_error("sanity check failed: failed push to empty bundle.");
        }
        if (bundleGuarded.m_items.size())
            pushIfNeeded();
    }

    std::sort(m_consumeResult.m_bundlesGuarded.begin(), m_consumeResult.m_bundlesGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getEstimatedArea() > r.getEstimatedArea();
    });
    std::sort(m_consumeResult.m_bundlesNonGuarded.begin(), m_consumeResult.m_bundlesNonGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getEstimatedArea() > r.getEstimatedArea();
    });

    std::string m_indent = "        ";

    auto setSegToBundle = [this](ObjectBundle& bundle, size_t incCount) {
        bundle.m_segmentIndex = m_consumeResult.m_currentSegment;
        m_consumeResult.m_segments[m_consumeResult.m_currentSegment].m_objectCount += incCount;
        m_consumeResult.m_currentSegment++;
        m_consumeResult.m_currentSegment %= m_consumeResult.m_segments.size();
    };

    {
        std::map<std::string, std::vector<ObjectBundle*>> objectsByRepulse;
        for (auto& bundle : m_consumeResult.m_bundlesGuarded)
            objectsByRepulse[bundle.m_repulseId].push_back(&bundle);

        for (const auto& [key, bundleList] : objectsByRepulse) {
            if (key.empty())
                continue;
            for (auto* bundle : bundleList) {
                setSegToBundle(*bundle, 1);
            }
        }
        for (const auto& [key, bundleList] : objectsByRepulse) {
            if (!key.empty())
                continue;
            for (auto* bundle : bundleList) {
                setSegToBundle(*bundle, 1);
            }
        }
    }
    for (auto& bundle : m_consumeResult.m_bundlesNonGuarded) {
        setSegToBundle(bundle, 1);
    }
    for (auto& bundle : m_consumeResult.m_bundlesNonGuardedPickable) {
        setSegToBundle(bundle, 0);
    }

    for (auto& zs : m_consumeResult.m_segments) {
        if (!zs.m_objectCount)
            continue;
        MapTileArea area;
        area.m_innerArea = zs.m_cells;
        auto parts       = area.splitByK(m_logOutput, zs.m_objectCount);
        for (auto& part : parts) {
            FHPos centroid = TileZone::makeCentroid(part.m_innerArea);
            auto* cell     = m_tileContainer.m_tileIndex.at(centroid);
            zs.m_centroids.push_back(cell);
            m_consumeResult.m_centroidsALL.push_back(cell);
        }
    }
    bool successPlacement = true;
    auto placeOnMapWrap   = [this, &m_indent, &successPlacement](ObjectBundle& bundle, size_t i, const std::string& prefix) {
        if (placeOnMap(bundle, m_consumeResult.m_segments[bundle.m_segmentIndex], true))
            return true;
        if (placeOnMap(bundle, m_consumeResult.m_segments[bundle.m_segmentIndex], false))
            return true;

        for (int att = 0; att <= 5; ++att) {
            m_consumeResult.m_currentSegment++;
            m_consumeResult.m_currentSegment %= m_consumeResult.m_segments.size();
            if (placeOnMap(bundle, m_consumeResult.m_segments[m_consumeResult.m_currentSegment], true))
                return true;
            if (placeOnMap(bundle, m_consumeResult.m_segments[m_consumeResult.m_currentSegment], false))
                return true;
        }
        m_logOutput << m_indent << prefix << " placement failure [" << i << "]: size=" << bundle.getEstimatedArea() << "; " << bundle.toPrintableString() << "\n";

        successPlacement = false;
        return false;
    };

    for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesGuarded) {
        if (!placeOnMapWrap(bundle, i++, "g"))
            continue;

        for (auto* pos : bundle.m_protectionBorder) {
            tileZone.m_needBeBlocked.insert(pos);

            //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = tileZone.m_index, .m_valueB = 1 });
        }

        if (bundle.m_guard) {
            Guard guard;
            guard.m_value = bundle.m_guard;
            guard.m_pos   = bundle.m_guardAbsPos;
            guard.m_zone  = &tileZone;
            m_guards.push_back(guard);
        }
    }

    for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesNonGuarded) {
        placeOnMapWrap(bundle, i++, "u");
    }
    size_t remainArea = 0;
    for (auto& zs : m_consumeResult.m_segments) {
        remainArea += zs.m_cells.size();
    }
    const size_t segTotal      = tileZone.m_innerAreaUsable.m_innerArea.size() - tileZone.m_possibleRoadsArea.size();
    const size_t remainPercent = (remainArea * 100 / segTotal);
    m_logOutput << m_indent << "remaining size=" << remainArea << " / " << segTotal << "\n";
    if (remainPercent < 10)
        m_logOutput << m_indent << "Warning: only " << remainPercent << "% left\n";

    if (1) {
        for (auto& zoneSegment : m_consumeResult.m_segments) {
            MapTileArea blockedEst;
            blockedEst.m_innerArea = zoneSegment.m_cells;
            auto          parts    = blockedEst.splitByFloodFill(false);
            MapTileRegion needBlock;
            for (auto& part : parts) {
                if (part.m_innerArea.size() < 3)
                    continue;
                const size_t maxArea = 12;

                auto segments  = part.splitByMaxArea(m_logOutput, maxArea);
                auto borderNet = MapTileArea::getInnerBorderNet(segments);

                for (const auto& seg : segments) {
                    for (auto* tile : seg.m_innerArea) {
                        if (borderNet.contains(tile))
                            continue;

                        needBlock.insert(tile);
                    }
                }
            }
            needBlock.doSort();

            tileZone.m_needBeBlocked.insert(needBlock);
            zoneSegment.m_cells.erase(needBlock);
            zoneSegment.m_cellsForUnguardedInner.erase(needBlock);

            zoneSegment.m_cells.doSort();
            zoneSegment.m_cellsForUnguardedInner.doSort();
        }

        tileZone.m_needBeBlocked.doSort();
    }

    for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesNonGuardedPickable) {
        placeOnMapWrap(bundle, i++, "p");
    }

    return successPlacement;
}

bool ObjectBundleSet::placeOnMap(ObjectBundle& bundle,
                                 ZoneSegment&  currentSegment,
                                 bool          useCentroids)
{
    Mernel::ProfilerScope scope("placeOnMap");

    const bool     unguardedPick = !bundle.m_guard && bundle.m_type == ObjectGenerator::IObject::Type::Pickable;
    bool           useRoad       = false;
    MapTileRegion* cellSource    = &currentSegment.m_cells;
    if (unguardedPick) {
        useRoad = m_rng->genSmall(2) == 0 && !m_consumeResult.m_cellsForUnguardedRoads.empty();
        if (useRoad)
            cellSource = &m_consumeResult.m_cellsForUnguardedRoads;
        else if (!currentSegment.m_cellsForUnguardedInner.empty())
            cellSource = &currentSegment.m_cellsForUnguardedInner;
    }

    if (cellSource->empty())
        return false;

    size_t rngCentroidIndex = size_t(-1);

    auto tryPlace = [this, &bundle, &currentSegment, useCentroids, useRoad, &rngCentroidIndex, &cellSource]() -> bool {
        MapTilePtr pos;
        if (useCentroids && !useRoad && !currentSegment.m_centroids.empty()) {
            rngCentroidIndex = m_rng->gen(currentSegment.m_centroids.size() - 1);
            pos              = currentSegment.m_centroids[rngCentroidIndex];
        } else {
            pos = (*cellSource)[m_rng->gen(cellSource->size() - 1)];
        }

        auto tryPlaceInner = [pos, &bundle, &cellSource](FHPos delta) -> bool {
            Mernel::ProfilerScope scope("tryPlaceInner");
            bundle.m_absPos = pos->neighbourByOffset(delta + FHPos{ 1, 1 });
            bundle.estimateOccupied();
            if (!bundle.m_absPosIsValid)
                return false;

            for (auto* posOcc : bundle.m_fitArea) {
                if (!cellSource->contains(posOcc)) {
                    return false;
                }
            }
            return true;
        };

        // if first attempt is succeed, we'll try to find near place where we don't fit, and then backup to more close fit.
        if (tryPlaceInner(g_deltasToTry[0])) {
            bool  start     = true;
            FHPos deltaPrev = g_deltasToTry[0];
            for (FHPos delta : g_deltasToTry) {
                if (start) {
                    start = false;
                    continue;
                }
                if (!tryPlaceInner(delta)) {
                    tryPlaceInner(deltaPrev);
                    return true;
                }
                deltaPrev = delta;
            }
            tryPlaceInner(deltaPrev);
            return true;
        }

        for (FHPos delta : g_deltasToTry) {
            if (tryPlaceInner(delta)) {
                return true;
            }
        }
        return false;
    };

    for (int i = 0; i < 10; ++i) {
        if (tryPlace()) {
            for (auto& item : bundle.m_items) {
                item.m_obj->setPos(item.m_absPos->m_pos);
                item.m_obj->place();
            }
            if (rngCentroidIndex != size_t(-1))
                currentSegment.m_centroids.erase(currentSegment.m_centroids.begin() + rngCentroidIndex);
            {
                Mernel::ProfilerScope scope1("erase");
                currentSegment.m_cells.erase(bundle.m_allArea);

                currentSegment.m_cellsForUnguardedInner.erase(bundle.m_fitArea);
                m_consumeResult.m_cellsForUnguardedRoads.erase(bundle.m_fitArea);

                currentSegment.m_cellsForUnguardedInner.erase(bundle.m_guardRegion);
                m_consumeResult.m_cellsForUnguardedRoads.erase(bundle.m_guardRegion);
            }

            {
                Mernel::ProfilerScope scope2("doSort");
                currentSegment.m_cells.doSort();
                currentSegment.m_cellsForUnguardedInner.doSort();
                m_consumeResult.m_cellsForUnguardedRoads.doSort();
            }

            return true;
        }
    }

    return false;
}

}
