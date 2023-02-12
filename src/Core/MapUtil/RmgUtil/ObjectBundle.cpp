/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ObjectBundle.hpp"
#include "TemplateZone.hpp"

#include <iostream>

namespace FreeHeroes {

void ObjectBundleSet::consume(const ObjectGenerator&        generated,
                              TileZone&                     tileZone,
                              Core::IRandomGenerator* const rng)
{
    std::set<FHPos> cells;

    for (auto& seg : tileZone.m_innerAreaSegments) {
        for (auto* cell : seg.m_innerArea)
            cells.insert({ cell->m_pos });
    }

    for (const auto& group : generated.m_groups) {
        for (const auto& obj : group.m_objects) {
            ObjectBundle bundle;
            bundle.m_items.push_back(ObjectBundle::Item{ .m_obj = obj });

            bundle.makeGuard(group.m_guardPercent);

            m_bundles.push_back(bundle);
        }
    }

    for (size_t i = 0; auto& bundle : m_bundles) {
        i++;
        bundle.placeOnMap(cells, rng);
    }
}

void ObjectBundle::makeGuard(int percent)
{
    int64_t total = 0;
    for (auto& item : m_items)
        total += item.m_obj->getGuard();
    m_guard = total * percent / 100;
}

void ObjectBundle::estimateOccupied()
{
    m_estimatedOccupied.clear();

    for (auto& item : m_items) {
        auto type = item.m_obj->getType();

        if (type == ObjectGenerator::IObject::Type::Pickable) {
            m_estimatedOccupied.insert(m_absPos);

            if (m_guard) {
                m_guardAbsPos = m_absPos;
                m_guardAbsPos.m_y++;

                m_estimatedOccupied.insert(m_guardAbsPos);

                m_estimatedOccupied.insert(m_absPos + FHPos{ -1, -1 });
                m_estimatedOccupied.insert(m_absPos + FHPos{ +0, -1 });
                m_estimatedOccupied.insert(m_absPos + FHPos{ +1, -1 });
                m_estimatedOccupied.insert(m_absPos + FHPos{ -1, 0 });
                m_estimatedOccupied.insert(m_absPos + FHPos{ +1, 0 });
            }
        }
        if (type == ObjectGenerator::IObject::Type::Visitable) {
            auto* def = item.m_obj->getDef();

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
            FHPos mainPos = m_absPos;
            for (size_t my = 0; my < def->visitMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->visitMapPlanar.width; ++mx) {
                    if (def->visitMapPlanar.data[my][mx] == 0)
                        continue;
                    FHPos maskBitPos{ (int) mx, (int) my, 0 };
                    mainPos = m_absPos - visitMaskSizePos + maskBitPos;
                }
            }
            if (m_guard) {
                m_guardAbsPos = mainPos;
                m_guardAbsPos.m_y++;

                m_estimatedOccupied.insert(m_guardAbsPos);
            }
        }
    }
}

void ObjectBundle::placeOnMap(std::set<FHPos>& availableCells, Core::IRandomGenerator* const rng)
{
    std::vector<FHPos> allCells(availableCells.begin(), availableCells.end());

    if (allCells.empty()) {
        assert(0);
        return;
    }

    auto tryPlace = [&allCells, &availableCells, rng, this]() -> bool {
        FHPos pos = allCells[rng->gen(allCells.size() - 1)];

        auto tryPlaceInner = [pos, &availableCells, this](FHPos delta) -> bool {
            this->m_absPos = pos + delta;
            this->estimateOccupied();

            for (FHPos posOcc : m_estimatedOccupied) {
                if (!availableCells.contains(posOcc)) {
                    return false;
                }
            }
            return true;
        };
        std::vector<FHPos> deltasToTry{ FHPos{}, FHPos{ -1, 0 }, FHPos{ +1, 0 }, FHPos{ 0, -1 }, FHPos{ 0, +1 } };

        for (FHPos delta : deltasToTry) {
            if (tryPlaceInner(delta)) {
                return true;
            }
        }
        return false;
    };

    for (int i = 0; i < 5; ++i) {
        if (tryPlace()) {
            for (auto& item : m_items) {
                item.m_obj->setPos(this->m_absPos);
                item.m_obj->place();
            }

            for (FHPos posOcc : m_estimatedOccupied) {
                availableCells.erase(posOcc);

                availableCells.erase(posOcc + FHPos{ -1, 0 });
                availableCells.erase(posOcc + FHPos{ +1, 0 });
                availableCells.erase(posOcc + FHPos{ 0, -1 });
                availableCells.erase(posOcc + FHPos{ 0, +1 });
            }

            return;
        }
    }

    std::cerr << "Placement failure!\n";
    // assert(0);
    //std::sort(cells.begin(), cells.end());
}

}
