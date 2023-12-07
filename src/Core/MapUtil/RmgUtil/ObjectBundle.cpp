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
    if (source.empty())
        return {};

    MapTileRegion result;
    result.reserve(source.size() * 3);
    for (auto pos : source) {
        result.insert(pos);
        result.insert(pos->neighboursList(diag));
    }
    if (excludeOriginal) {
        result.erase(source);
    }
    return result;
}

}

bool ObjectBundle::estimateOccupied(MapTilePtr absPos, MapTilePtr cetroid)
{
    if (!absPos)
        return false;

    auto guardPosition = posDirectionTo(cetroid->m_pos, absPos->m_pos);
    if (guardPosition == FHPosDirection::Invalid)
        guardPosition = FHPosDirection::B;

    if (m_absPosIsValid && m_guardPosition == guardPosition) {
        Mernel::ProfilerScope scope("estimate shift");
        auto                  delta = absPos->m_pos - m_absPos->m_pos;
        m_absPos                    = absPos;

        auto shiftArea = [this, delta](MapTileRegion& area, std::string desc) {
            if (!m_absPosIsValid)
                return;

            area.updateAllValues([delta, this](MapTilePtr tile) {
                tile = tile->neighbourByOffset(delta);
                if (!tile) {
                    m_absPosIsValid = false;
                }
                return tile;
            });
        };
        shiftArea(m_rewardArea, "m_rewardArea");
        shiftArea(m_extraObstacles, "m_extraObstacles");
        shiftArea(m_unpassableArea, "m_unpassableArea");
        shiftArea(m_occupiedArea, "m_occupiedArea");

        shiftArea(m_dangerZone, "m_dangerZone");
        shiftArea(m_occupiedWithDangerZone, "m_occupiedWithDangerZone");
        shiftArea(m_passAroundEdge, "m_passAroundEdge");
        shiftArea(m_allArea, "m_allArea");

        if (m_guardAbsPos) {
            m_guardAbsPos = m_guardAbsPos->neighbourByOffset(delta);
            if (!m_guardAbsPos)
                m_absPosIsValid = false;
        }

        for (auto& item : m_items) {
            item.m_absPos = item.m_absPos->neighbourByOffset(delta);
            if (!item.m_absPos) {
                m_absPosIsValid = false;
            }
        }

        if (m_absPosIsValid)
            return true;
    }

    Mernel::ProfilerScope scope("estimateOccupied");
    m_rewardArea.clear();
    m_extraObstacles.clear();
    m_unpassableArea.clear();
    m_occupiedArea.clear();

    m_dangerZone.clear();
    m_occupiedWithDangerZone.clear();
    m_passAroundEdge.clear();
    m_allArea.clear();

    m_absPosIsValid = false;

    m_absPos        = absPos;
    m_guardPosition = guardPosition;

    if (m_type == ObjectGenerator::IObject::Type::Pickable) {
        size_t       itemCount      = m_items.size();
        const size_t maxRowSize     = 2;
        const size_t itemRectHeight = (itemCount + maxRowSize - 1) / maxRowSize;
        const size_t itemRectWidth  = std::min(maxRowSize, itemCount);
        m_rewardArea.reserve(m_items.size());
        for (size_t index = 0; auto& item : m_items) {
            FHPos itemOffset;

            itemOffset.m_y -= index / itemRectWidth;
            itemOffset.m_x -= index % itemRectWidth;
            auto visualPos = m_absPos->neighbourByOffset(itemOffset);
            if (!visualPos)
                return false;

            m_rewardArea.insert(visualPos);
            item.m_absPos = visualPos->neighbourByOffset(item.m_obj->getOffset());
            if (!item.m_absPos)
                return false;

            index++;
        }

        if (m_guard) {
            FHPos guardOffset;

            if (guardPosition == FHPosDirection::B) {
                guardOffset.m_y++;
            }
            if (guardPosition == FHPosDirection::BR) {
                guardOffset.m_y++;
                guardOffset.m_x++;
            }
            if (guardPosition == FHPosDirection::BL) {
                guardOffset.m_y++;
                guardOffset.m_x -= itemRectWidth;
            }
            if (guardPosition == FHPosDirection::R) {
                guardOffset.m_x++;
            }
            if (guardPosition == FHPosDirection::TR) {
                guardOffset.m_x++;
                guardOffset.m_y -= itemRectHeight;
            }
            if (guardPosition == FHPosDirection::T) {
                guardOffset.m_y -= itemRectHeight;
            }
            if (guardPosition == FHPosDirection::TL) {
                guardOffset.m_y -= itemRectHeight;
                guardOffset.m_x -= itemRectWidth;
            }
            if (guardPosition == FHPosDirection::L) {
                guardOffset.m_x -= itemRectWidth;
            }

            m_guardAbsPos = m_absPos->neighbourByOffset(guardOffset);
            if (!m_guardAbsPos)
                return false;

            m_dangerZone = blurSet(MapTileRegion({ m_guardAbsPos }), true, false);

            m_extraObstacles = blurSet(m_rewardArea, true);
            m_extraObstacles.erase(m_dangerZone);

            m_unpassableArea = m_extraObstacles;
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
                    return false;
                m_rewardArea.insert(occPos);
            }
            for (auto&& point : def->combinedMask.m_visitable) {
                mainPos = m_absPos->neighbourByOffset(FHPos{ point.m_x, point.m_y });
                if (!mainPos)
                    return false;
            }
        }
        m_unpassableArea = m_rewardArea;

        if (m_guard) {
            FHPos guardOffset;
            guardOffset.m_y++;
            if (guardPosition == FHPosDirection::TL || guardPosition == FHPosDirection::L || guardPosition == FHPosDirection::BL)
                guardOffset.m_x--;
            if (guardPosition == FHPosDirection::TR || guardPosition == FHPosDirection::R || guardPosition == FHPosDirection::BR)
                guardOffset.m_x++;
            m_guardAbsPos = mainPos->neighbourByOffset(guardOffset);
            if (!m_guardAbsPos)
                return false;
            m_dangerZone = blurSet(MapTileRegion({ m_guardAbsPos }), true, false);
        }
    }

    m_occupiedArea.insert(m_extraObstacles);
    m_occupiedArea.insert(m_rewardArea);
    if (m_guardAbsPos)
        m_occupiedArea.insert(m_guardAbsPos);

    m_dangerZone.erase(m_occupiedArea);

    m_occupiedWithDangerZone = m_occupiedArea;
    m_occupiedWithDangerZone.insert(m_dangerZone);

    m_passAroundEdge = blurSet(m_occupiedWithDangerZone, false);

    m_allArea = m_occupiedWithDangerZone;
    m_allArea.insert(m_passAroundEdge);

    m_absPosIsValid = true;
    return true;
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
        guards      = blurSet(guards, true, false);
        safePadding.insert(guards);
        safePadding = blurSet(safePadding, true, false);
    }

    for (auto& seg : tileZone.m_innerAreaSegments) {
        ZoneSegment zs;
        zs.m_innerEdge = seg.m_innerEdge;
        zs.m_cells     = seg.m_innerArea;
        zs.m_cells.erase(safePadding);
        zs.m_cellsForUnguardedInner = zs.m_cells;
        FHPos centroid              = TileZone::makeCentroid(zs.m_cells);
        zs.m_mainCetroid            = m_tileContainer.m_tileIndex.at(centroid);
        m_consumeResult.m_segments.push_back(std::move(zs));
    }

    m_consumeResult.m_cellsForUnguardedRoads = tileZone.m_placedRoads;
    m_consumeResult.m_cellsForUnguardedRoads.erase(safePadding);

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

        obj.m_type = type;

        return obj;
    };

    for (const auto& [type, bucket] : m_consumeResult.m_buckets) {
        for (const auto& item : bucket.m_nonGuarded) {
            ObjectBundle bundleNonGuarded;
            bundleNonGuarded.m_items.push_back(ObjectBundle::Item{ .m_obj = item.m_obj });
            bundleNonGuarded.m_type = type;
            bundleNonGuarded.estimateOccupied(m_tileContainer.m_centerTile, m_tileContainer.m_centerTile);
            assert(bundleNonGuarded.m_absPosIsValid);

            if (type == ObjectGenerator::IObject::Type::Pickable)
                m_consumeResult.m_bundlesNonGuardedPickable.push_back(std::move(bundleNonGuarded));
            else
                m_consumeResult.m_bundlesNonGuarded.push_back(std::move(bundleNonGuarded));
        }

        ObjectBundle bundleGuarded = makeNewObjectBundle(type);

        auto pushIfNeeded = [this, &tileZone, &bundleGuarded, &makeNewObjectBundle]() {
            bundleGuarded.m_considerBlock = bundleGuarded.m_guard > tileZone.m_rngZoneSettings.m_guardBlock;
            bundleGuarded.estimateOccupied(m_tileContainer.m_centerTile, m_tileContainer.m_centerTile);
            assert(bundleGuarded.m_absPosIsValid);

            m_consumeResult.m_bundlesGuarded.push_back(bundleGuarded);
            bundleGuarded = makeNewObjectBundle(bundleGuarded.m_type);
        };

        for (const auto& item : bucket.m_guarded) {
            auto newItem = ObjectBundle::Item{ .m_obj = item.m_obj, .m_guard = item.m_guard };
            if (bundleGuarded.tryPush(newItem)) {
                continue;
            }
            pushIfNeeded();

            if (!bundleGuarded.tryPush(newItem))
                throw std::runtime_error("sanity check failed: failed push to empty bundle.");
        }
        if (bundleGuarded.m_items.size())
            pushIfNeeded();
    }

    std::sort(m_consumeResult.m_bundlesGuarded.begin(), m_consumeResult.m_bundlesGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getSortTuple() > r.getSortTuple();
    });
    std::sort(m_consumeResult.m_bundlesNonGuarded.begin(), m_consumeResult.m_bundlesNonGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getSortTuple() > r.getSortTuple();
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

        for (auto* pos : bundle.m_extraObstacles) {
            tileZone.m_needBeBlocked.insert(pos);
        }

        tileZone.m_rewardTilesMain.insert(bundle.m_occupiedArea);
        tileZone.m_rewardTilesDanger.insert(bundle.m_dangerZone);
        tileZone.m_rewardTilesSpacing.insert(bundle.m_passAroundEdge);

        if (bundle.m_guard) {
            Guard guard;
            guard.m_value = bundle.m_guard;
            guard.m_pos   = bundle.m_guardAbsPos;
            guard.m_zone  = &tileZone;
            m_guards.push_back(guard);
        }
    }

    if (1) {
        for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesNonGuarded) {
            placeOnMapWrap(bundle, i++, "u");
        }
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
            blockedEst.m_innerArea.erase(zoneSegment.m_innerEdge);
            auto          parts = blockedEst.splitByFloodFill(false);
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

            tileZone.m_needBeBlocked.insert(needBlock);
            zoneSegment.m_cells.erase(needBlock);
            zoneSegment.m_cellsForUnguardedInner.erase(needBlock);
        }
    }

    if (1) {
        for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesNonGuardedPickable) {
            placeOnMapWrap(bundle, i++, "p");
        }
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
        Mernel::ProfilerScope scope("tryPlace");
        MapTilePtr            pos;
        if (useCentroids && !useRoad && !currentSegment.m_centroids.empty()) {
            rngCentroidIndex = m_rng->gen(currentSegment.m_centroids.size() - 1);
            pos              = currentSegment.m_centroids[rngCentroidIndex];
        } else {
            pos = (*cellSource)[m_rng->gen(cellSource->size() - 1)];
        }

        auto tryPlaceInner = [pos, &bundle, &cellSource, &currentSegment](FHPos delta) -> bool {
            if (!bundle.estimateOccupied(pos->neighbourByOffset(delta + FHPos{ 1, 1 }), currentSegment.m_mainCetroid))
                return false;

            for (auto* posOcc : bundle.m_occupiedWithDangerZone) {
                if (!cellSource->contains(posOcc)) {
                    return false;
                }
            }
            return true;
        };

        // if first attempt is succeed, we'll try to find near place where we don't fit, and then backup to more close fit.
        if (tryPlaceInner(g_deltasToTry[0])) {
            //std::cout << "!";
            bool  start     = true;
            FHPos deltaPrev = g_deltasToTry[0];
            for (FHPos delta : g_deltasToTry) {
                //std::cout << "*";
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
        Mernel::ProfilerScope scope1("failed");
        //std::cout << "#";
        for (FHPos delta : g_deltasToTry) {
            //std::cout << "*";
            if (tryPlaceInner(delta)) {
                Mernel::ProfilerScope scope2("deltashelp");
                return true;
            }
            Mernel::ProfilerScope scope3("deltasNOThelp");
        }
        return false;
    };

    for (int i = 0; i < 10; ++i) {
        //if (i > 0)
        //    std::cout << "R";
        if (tryPlace()) {
            for (auto& item : bundle.m_items) {
                item.m_obj->setPos(item.m_absPos->m_pos);
                item.m_obj->place();
            }
            if (rngCentroidIndex != size_t(-1))
                currentSegment.m_centroids.erase(currentSegment.m_centroids.begin() + rngCentroidIndex);
            {
                currentSegment.m_cells.erase(bundle.m_allArea);

                currentSegment.m_cellsForUnguardedInner.erase(bundle.m_occupiedArea);
                m_consumeResult.m_cellsForUnguardedRoads.erase(bundle.m_occupiedArea);

                currentSegment.m_cellsForUnguardedInner.erase(bundle.m_dangerZone);
                m_consumeResult.m_cellsForUnguardedRoads.erase(bundle.m_dangerZone);
            }

            return true;
        }
    }

    return false;
}

}
