/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ObjectBundle.hpp"
#include "TemplateZone.hpp"

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
};
// clang-format on

std::set<FHPos> blurSet(const std::set<FHPos>& source, bool diag, bool excludeOriginal = true)
{
    std::set<FHPos> result;
    for (auto pos : source) {
        result.insert(pos + FHPos{ -1, 0 });
        result.insert(pos + FHPos{ +1, 0 });
        result.insert(pos + FHPos{ 0, -1 });
        result.insert(pos + FHPos{ 0, +1 });
        if (diag) {
            result.insert(pos + FHPos{ -1, -1 });
            result.insert(pos + FHPos{ +1, -1 });
            result.insert(pos + FHPos{ -1, +1 });
            result.insert(pos + FHPos{ +1, +1 });
        }
    }
    if (excludeOriginal) {
        for (auto it = result.begin(); it != result.end();) {
            if (source.contains(*it)) {
                it = result.erase(it);
            } else {
                ++it;
            }
        }
    }
    return result;
}

}

void ObjectBundleSet::consume(const ObjectGenerator&        generated,
                              TileZone&                     tileZone,
                              Core::IRandomGenerator* const rng)
{
    for (auto& seg : tileZone.m_innerAreaSegments) {
        for (auto* cell : seg.m_innerArea)
            m_cells.insert({ cell->m_pos });
    }

    for (const auto& group : generated.m_groups) {
        for (const auto& obj : group.m_objects) {
            BucketItem item;
            item.m_guard = obj->getGuard() * group.m_guardPercent / 100;
            item.m_obj   = obj;

            auto& buck = m_buckets[obj->getType()];
            if (item.m_guard)
                buck.m_guarded.push_back(item);
            else
                buck.m_nonGuarded.push_back(item);
        }
    }

    auto makeNewTargetGuard = [rng, &tileZone]() {
        int64_t min         = tileZone.m_rngZoneSettings.m_guardMin;
        int64_t max         = tileZone.m_rngZoneSettings.m_guardMax;
        int64_t targetGuard = min + rng->gen(max - min);
        return targetGuard;
    };

    for (const auto& [type, bucket] : m_buckets) {
        for (const auto& item : bucket.m_nonGuarded) {
            ObjectBundle bundleNonGuarded;
            bundleNonGuarded.m_items.push_back(ObjectBundle::Item{ .m_obj = item.m_obj });
            bundleNonGuarded.m_type = type;

            m_bundlesNonGuarded.push_back(std::move(bundleNonGuarded));
        }

        ObjectBundle bundleGuarded;
        bundleGuarded.m_type = type;
        int64_t targetGuard  = makeNewTargetGuard();

        auto pushIfNeeded = [this, &bundleGuarded, &targetGuard, &makeNewTargetGuard](bool force) {
            bundleGuarded.sumGuard();
            if (!force && bundleGuarded.m_guard < targetGuard) {
                if (bundleGuarded.m_canPushMore)
                    return;
            }

            m_bundlesGuarded.push_back(bundleGuarded);
            bundleGuarded.m_items.clear();
            targetGuard = makeNewTargetGuard();
        };

        for (const auto& item : bucket.m_guarded) {
            bundleGuarded.m_items.push_back(ObjectBundle::Item{ .m_obj = item.m_obj, .m_guard = item.m_guard });
            bundleGuarded.checkIfCanPushMore();
            pushIfNeeded(false);
        }
        pushIfNeeded(true);
    }

    std::sort(m_bundlesGuarded.begin(), m_bundlesGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getEstimatedArea() > r.getEstimatedArea();
    });
    std::sort(m_bundlesNonGuarded.begin(), m_bundlesNonGuarded.end(), [](const ObjectBundle& l, const ObjectBundle& r) {
        return l.getEstimatedArea() > r.getEstimatedArea();
    });
}

void ObjectBundle::sumGuard()
{
    m_guard = 0;
    for (auto& item : m_items)
        m_guard += item.m_guard;
}

void ObjectBundle::checkIfCanPushMore()
{
    m_canPushMore = false;
    if (m_type == ObjectGenerator::IObject::Type::Pickable) {
        m_canPushMore = m_items.size() < 4;
    }
}

void ObjectBundle::estimateOccupied()
{
    m_estimatedOccupied.clear();
    m_protectionBorder.clear();
    m_blurForPassable.clear();
    m_allArea.clear();
    m_guardRegion.clear();

    if (m_type == ObjectGenerator::IObject::Type::Pickable) {
        for (size_t index = 0; auto& item : m_items) {
            auto pos = m_absPos;
            pos.m_y -= index / 2;
            pos.m_x -= index % 2;
            m_estimatedOccupied.insert(pos);
            item.m_absPos = pos;
            index++;
        }
        auto protection = blurSet(m_estimatedOccupied, true);

        if (m_guard) {
            m_guardAbsPos = m_absPos;
            m_guardAbsPos.m_y++;

            m_guardRegion = blurSet({ m_guardAbsPos }, true, false);

            for (auto pos : protection) {
                if (!m_guardRegion.contains(pos))
                    m_protectionBorder.insert(pos);
            }
        }
    }
    if (m_type == ObjectGenerator::IObject::Type::Visitable) {
        FHPos mainPos = m_absPos;
        for (size_t index = 0; auto& item : m_items) {
            item.m_absPos = m_absPos;
            auto* def     = item.m_obj->getDef();

            const FHPos blockMaskSizePos{ (int) def->blockMapPlanar.width - 1, (int) def->blockMapPlanar.height - 1, 0 };
            const FHPos visitMaskSizePos{ (int) def->visitMapPlanar.width - 1, (int) def->visitMapPlanar.height - 1, 0 };

            for (size_t my = 0; my < def->blockMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->blockMapPlanar.width; ++mx) {
                    if (def->blockMapPlanar.data[my][mx] == 0)
                        continue;
                    FHPos maskBitPos{ (int) mx, (int) my, 0 };

                    m_estimatedOccupied.insert(m_absPos - blockMaskSizePos + maskBitPos);
                }
            }

            for (size_t my = 0; my < def->visitMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->visitMapPlanar.width; ++mx) {
                    if (def->visitMapPlanar.data[my][mx] == 0)
                        continue;
                    FHPos maskBitPos{ (int) mx, (int) my, 0 };
                    mainPos = m_absPos - visitMaskSizePos + maskBitPos;
                }
            }

            index++;
        }

        if (m_guard) {
            m_guardAbsPos = mainPos;
            m_guardAbsPos.m_y++;

            m_guardRegion = blurSet({ m_guardAbsPos }, true, false);
        }
    }

    if (m_guard) {
        m_estimatedOccupied.insert(m_guardRegion.cbegin(), m_guardRegion.cend());
    }

    if (m_guard || m_type == ObjectGenerator::IObject::Type::Visitable) {
        std::set<FHPos> current = m_estimatedOccupied;
        current.insert(m_protectionBorder.cbegin(), m_protectionBorder.cend());

        m_blurForPassable = blurSet(current, false);
    }
    m_allArea.insert(m_protectionBorder.cbegin(), m_protectionBorder.cend());
    m_allArea.insert(m_blurForPassable.cbegin(), m_blurForPassable.cend());
    m_allArea.insert(m_estimatedOccupied.cbegin(), m_estimatedOccupied.cend());
}

bool ObjectBundle::placeOnMap(std::set<FHPos>& availableCells, Core::IRandomGenerator* const rng)
{
    std::vector<FHPos> allCells(availableCells.begin(), availableCells.end());

    if (allCells.empty()) {
        return false;
    }

    auto tryPlace = [&allCells, &availableCells, rng, this]() -> bool {
        FHPos pos = allCells[rng->gen(allCells.size() - 1)];

        auto tryPlaceInner = [pos, &availableCells, this](FHPos delta) -> bool {
            this->m_absPos = pos + delta;
            this->estimateOccupied();

            for (FHPos posOcc : m_allArea) {
                if (!availableCells.contains(posOcc)) {
                    return false;
                }
            }
            return true;
        };

        for (FHPos delta : g_deltasToTry) {
            if (tryPlaceInner(delta)) {
                return true;
            }
        }
        return false;
    };

    for (int i = 0; i < 5; ++i) {
        if (tryPlace()) {
            for (auto& item : m_items) {
                item.m_obj->setPos(item.m_absPos);
                item.m_obj->place();
            }

            for (FHPos posOcc : m_allArea)
                availableCells.erase(posOcc);

            return true;
        }
    }

    return false;
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

}
