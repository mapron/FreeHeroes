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

void ObjectBundle::sumGuard()
{
    m_guard = 0;
    for (auto& item : m_items)
        m_guard += item.m_guard;
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
        for (size_t index = 0; auto& item : m_items) {
            item.m_absPos = m_absPos;
            auto* def     = item.m_obj->getDef();
            assert(def);

            // std::cerr << item.m_obj->getId() << "-> " << def->blockMapPlanar.width << "x" << def->blockMapPlanar.height << "\n";

            const FHPos blockMaskSizePos{ (int) def->blockMapPlanar.width - 1, (int) def->blockMapPlanar.height - 1, 0 };
            const FHPos visitMaskSizePos{ (int) def->visitMapPlanar.width - 1, (int) def->visitMapPlanar.height - 1, 0 };

            for (size_t my = 0; my < def->blockMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->blockMapPlanar.width; ++mx) {
                    if (def->blockMapPlanar.data[my][mx] == 0)
                        continue;
                    FHPos maskBitPos{ (int) mx, (int) my, 0 };
                    auto  occPos = m_absPos->neighbourByOffset(FHPos{} - blockMaskSizePos + maskBitPos);
                    if (!occPos)
                        return;
                    m_estimatedOccupied.insert(occPos);
                }
            }

            for (size_t my = 0; my < def->visitMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->visitMapPlanar.width; ++mx) {
                    if (def->visitMapPlanar.data[my][mx] == 0)
                        continue;
                    FHPos maskBitPos{ (int) mx, (int) my, 0 };
                    mainPos = m_absPos->neighbourByOffset(FHPos{} - visitMaskSizePos + maskBitPos);
                    if (!mainPos)
                        return;
                }
            }

            index++;
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
    m_fitArea.doSort();

    m_absPosIsValid = true;
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

    m_consumeResult = {};

    for (auto& seg : tileZone.m_innerAreaSegments) {
        m_consumeResult.m_cells.insert(seg.m_innerArea);
    }
    m_consumeResult.m_cells.doSort();

    m_consumeResult.m_cellsForUnguardedInner = m_consumeResult.m_cells;
    for (auto* cell : tileZone.m_innerAreaUsable.m_innerArea) {
        if (!m_consumeResult.m_cells.contains(cell))
            m_consumeResult.m_cellsForUnguardedRoads.insert(cell);
    }
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
        if (type == ObjectGenerator::IObject::Type::Pickable)
            obj.m_itemLimit = 1 + m_rng->genSmall(3);

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

        auto pushIfNeeded = [this, &tileZone, &bundleGuarded, &makeNewObjectBundle](bool force) {
            bundleGuarded.sumGuard();
            bundleGuarded.m_considerBlock = bundleGuarded.m_guard > tileZone.m_rngZoneSettings.m_guardBlock;

            if (!force && bundleGuarded.m_guard < bundleGuarded.m_targetGuard) {
                if (bundleGuarded.m_items.size() < bundleGuarded.m_itemLimit)
                    return;
            }
            if (!bundleGuarded.m_items.size())
                return;

            bundleGuarded.m_absPos = m_tileContainer.m_centerTile;
            bundleGuarded.estimateOccupied();
            assert(bundleGuarded.m_absPosIsValid);

            m_consumeResult.m_bundlesGuarded.push_back(bundleGuarded);
            bundleGuarded = makeNewObjectBundle(bundleGuarded.m_type);
        };

        for (const auto& item : bucket.m_guarded) {
            bundleGuarded.m_items.push_back(ObjectBundle::Item{ .m_obj = item.m_obj, .m_guard = item.m_guard });
            pushIfNeeded(false);
        }
        pushIfNeeded(true);
    }

    std::sort(m_consumeResult.m_bundlesGuarded.begin(), m_consumeResult.m_bundlesGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getEstimatedArea() > r.getEstimatedArea();
    });
    std::sort(m_consumeResult.m_bundlesNonGuarded.begin(), m_consumeResult.m_bundlesNonGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getEstimatedArea() > r.getEstimatedArea();
    });

    std::string m_indent = "        ";
    bool        success  = true;

    for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesGuarded) {
        i++;

        if (!placeOnMap(bundle)) {
            success = false;
            m_logOutput << m_indent << "g placement failure [" << i << "]: size=" << bundle.getEstimatedArea() << "; " << bundle.toPrintableString() << "\n";
            continue;
        }

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
        i++;
        if (!this->placeOnMap(bundle)) {
            success = false;
            m_logOutput << m_indent << "u placement failure [" << i << "]: size=" << bundle.getEstimatedArea() << "; " << bundle.toPrintableString() << "\n";
            continue;
        }
    }

    {
        MapTileArea blockedEst;
        blockedEst.m_innerArea = m_consumeResult.m_cells;
        auto          parts    = blockedEst.splitByFloodFill(false);
        MapTileRegion needBlock;
        for (auto& part : parts) {
            if (part.m_innerArea.size() < 3)
                continue;
            const size_t maxArea = 12;

            auto segments  = part.splitByMaxArea(maxArea);
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
        m_consumeResult.m_cells.erase(needBlock);
        m_consumeResult.m_cellsForUnguardedInner.erase(needBlock);
        m_consumeResult.m_cellsForUnguardedRoads.erase(needBlock);

        tileZone.m_needBeBlocked.doSort();
        m_consumeResult.m_cells.doSort();
        m_consumeResult.m_cellsForUnguardedInner.doSort();
        m_consumeResult.m_cellsForUnguardedRoads.doSort();

        // @todo: max 6x6 blocks
    }

    for (size_t i = 0; auto& bundle : m_consumeResult.m_bundlesNonGuardedPickable) {
        i++;
        if (!this->placeOnMap(bundle)) {
            success = false;
            m_logOutput << m_indent << "p placement failure [" << i << "]: size=" << bundle.getEstimatedArea() << "; " << bundle.toPrintableString() << "\n";
            continue;
        }
    }

    return success;
}

bool ObjectBundleSet::placeOnMap(ObjectBundle& bundle)
{
    Mernel::ProfilerScope scope("placeOnMap");

    MapTileRegion* cellSource = &m_consumeResult.m_cells;
    if (!bundle.m_guard && bundle.m_type == ObjectGenerator::IObject::Type::Pickable) {
        auto useRoad = m_rng->genSmall(2) > 0;
        if (useRoad && !m_consumeResult.m_cellsForUnguardedRoads.empty())
            cellSource = &m_consumeResult.m_cellsForUnguardedRoads;
        else if (!m_consumeResult.m_cellsForUnguardedInner.empty())
            cellSource = &m_consumeResult.m_cellsForUnguardedInner;
    }

    if (cellSource->empty()) {
        return false;
    }

    auto tryPlace = [this, &bundle, cellSource]() -> bool {
        auto* pos = (*cellSource)[m_rng->gen(cellSource->size() - 1)];

        auto tryPlaceInner = [pos, &bundle, cellSource](FHPos delta) -> bool {
            Mernel::ProfilerScope scope("tryPlaceInner");
            bundle.m_absPos = pos->neighbourByOffset(delta);
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

            {
                Mernel::ProfilerScope scope1("erase");
                m_consumeResult.m_cells.erase(bundle.m_allArea);

                m_consumeResult.m_cellsForUnguardedInner.erase(bundle.m_fitArea);
                m_consumeResult.m_cellsForUnguardedRoads.erase(bundle.m_fitArea);

                m_consumeResult.m_cellsForUnguardedInner.erase(bundle.m_guardRegion);
                m_consumeResult.m_cellsForUnguardedRoads.erase(bundle.m_guardRegion);
            }

            {
                Mernel::ProfilerScope scope2("doSort");
                m_consumeResult.m_cells.doSort();
                m_consumeResult.m_cellsForUnguardedInner.doSort();
                m_consumeResult.m_cellsForUnguardedRoads.doSort();
            }

            return true;
        }
    }

    return false;
}

}
