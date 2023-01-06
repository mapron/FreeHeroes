/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMap.hpp"
#include "FHTemplateCanvas.hpp"
#include "IRandomGenerator.hpp"

#include <unordered_set>

namespace std {
template<>
struct hash<FreeHeroes::FHPos> {
    constexpr std::size_t operator()(const FreeHeroes::FHPos& p) const noexcept
    {
        return p.getHash();
    }
};
}

namespace FreeHeroes {

struct TileZone {
    int                     m_index = 0;
    std::string             m_id;
    FHRngZone               m_rngZoneSettings;
    Core::IRandomGenerator* m_rng       = nullptr;
    MapCanvas*              m_mapCanvas = nullptr;

    using TileRegion = std::unordered_set<FHPos>;

    FHPos      m_startTile;
    TileRegion m_innerArea;
    TileRegion m_innerEdge;   // subset of innerArea;
    TileRegion m_outsideEdge; // is not subset of inner area.

    TileRegion m_lastGrowed;

    int64_t m_relativeArea   = 0;
    int64_t m_absoluteArea   = 0;
    int64_t m_absoluteRadius = 0;
    //int64_t    m_areaDeficit    = 0;

    int64_t getPlacedArea() const
    {
        return m_innerArea.size();
    }
    int64_t getAreaDeficit() const
    {
        return m_absoluteArea - getPlacedArea();
    }

    void readFromMap()
    {
        m_innerArea.clear();
        m_innerArea.insert(m_startTile);

        makeEdgeFromInnerArea();

        while (true) {
            const bool growResult = tryGrowOnce([this](const FHPos& pos) {
                auto& cell = m_mapCanvas->m_tiles[pos];
                return cell.m_zoned && cell.m_zoneIndex == m_index;
            });
            if (!growResult)
                break;
        }

        for (auto& [pos, cell] : m_mapCanvas->m_tiles) {
            if (cell.m_zoneIndex == m_index && !m_innerArea.contains(pos))
                cell.m_zoned = false;
        }
        //makeEdgeFromInnerArea();
    }
    void readFromMapIfDirty()
    {
        if (!m_mapCanvas->m_dirtyZones.contains(m_index))
            return;
        m_mapCanvas->m_dirtyZones.erase(m_index);
        readFromMap();
    }

    void makeEdgeFromInnerArea()
    {
        m_innerEdge = m_innerArea;
        removeNonInnerFromInnerEdge();
    }
    void removeNonInnerFromInnerEdge()
    {
        auto edge = m_innerEdge;
        m_innerEdge.clear();
        for (const FHPos& pos : edge) {
            if (m_innerArea.contains(posNeighbour(pos, +1, +0))
                && m_innerArea.contains(posNeighbour(pos, -1, +0))
                && m_innerArea.contains(posNeighbour(pos, +0, +1))
                && m_innerArea.contains(posNeighbour(pos, +0, -1)))
                continue;
            m_innerEdge.insert(pos);
        }

        makeOutsideEdge();
    }

    void makeOutsideEdge()
    {
        m_outsideEdge.clear();
        auto predicate = [this](const FHPos& pos) {
            return m_mapCanvas->m_tiles.contains(pos) && !m_innerArea.contains(pos);
        };
        for (auto pos : m_innerEdge) {
            addIf(m_outsideEdge, posNeighbour(pos, +1, +0), predicate);
            addIf(m_outsideEdge, posNeighbour(pos, -1, +0), predicate);
            addIf(m_outsideEdge, posNeighbour(pos, +0, +1), predicate);
            addIf(m_outsideEdge, posNeighbour(pos, +0, -1), predicate);
        }
    }

    void writeToMap()
    {
        for (auto&& pos : m_innerArea) {
            auto& cell       = m_mapCanvas->m_tiles[pos];
            cell.m_zoneIndex = m_index;
            cell.m_zoned     = true;
        }
    }

    bool isInBounds(const FHPos& point)
    {
        return m_mapCanvas->m_tiles.contains(point);
    }

    void addIf(TileRegion& reg, FHPos point, auto&& predicate)
    {
        if (!predicate(point))
            return;
        reg.insert(point);
    }

    bool tryGrowOnce(auto&& predicate)
    {
        bool result = false;
        m_lastGrowed.clear();
        for (auto pos : m_outsideEdge) {
            if (predicate(pos)) {
                result = true;
                m_lastGrowed.insert(pos);

                m_innerEdge.insert(pos);
                m_innerArea.insert(pos);
            }
        }
        if (result) {
            removeNonInnerFromInnerEdge();
        }
        return result;
    }

    bool tryGrowOnceToUnzoned(bool allowConsumingNeighbours)
    {
        const bool result = tryGrowOnce([this, allowConsumingNeighbours](const FHPos& pos) {
            auto& cell = m_mapCanvas->m_tiles[pos];
            return !cell.m_zoned || (allowConsumingNeighbours && cell.m_zoneIndex != m_index);
        });
        if (!result)
            return false;

        for (auto pos : m_lastGrowed) {
            auto& cell = m_mapCanvas->m_tiles[pos];
            if (cell.m_zoned && cell.m_zoneIndex != m_index) {
                m_mapCanvas->m_dirtyZones.insert(cell.m_zoneIndex);
            }
            cell.m_zoned     = true;
            cell.m_zoneIndex = m_index;
        }
        return true;
    }

    void fillDeficit(int thresholdPercent, bool allowConsumingNeighbours)
    {
        const int64_t allowedDeficitThreshold = m_absoluteArea * thresholdPercent / 100;
        while (true) {
            if (getAreaDeficit() < allowedDeficitThreshold)
                break;
            if (!tryGrowOnceToUnzoned(allowConsumingNeighbours))
                break;
        }

        // makeEdgeFromInnerArea();
    }

    void fillTheRest()
    {
        while (tryGrowOnceToUnzoned(false)) {
            ;
        }
        //  makeEdgeFromInnerArea();
    }
};

}
