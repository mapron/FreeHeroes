/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHMap.hpp"
#include "TemplateCanvas.hpp"
#include "IRandomGenerator.hpp"
#include "TemplateUtils.hpp"

#include <unordered_set>

namespace FreeHeroes {

struct TileZone {
    int                          m_index = 0;
    std::string                  m_id;
    FHRngZone                    m_rngZoneSettings;
    Core::IRandomGenerator*      m_rng             = nullptr;
    MapCanvas*                   m_mapCanvas       = nullptr;
    Core::LibraryTerrainConstPtr m_terrain         = nullptr;
    Core::LibraryFactionConstPtr m_mainTownFaction = nullptr;
    Core::LibraryFactionConstPtr m_rewardsFaction  = nullptr;
    Core::LibraryPlayerConstPtr  m_player          = nullptr;

    using TileRegion = std::unordered_set<MapCanvas::Tile*>;

    FHPos m_startTile;
    FHPos m_centroid;

    TileRegion m_innerArea;
    TileRegion m_innerEdge;   // subset of innerArea;
    TileRegion m_outsideEdge; // is not subset of inner area.

    TileRegion m_lastGrowed;

    TileRegion m_roadNodes;
    TileRegion m_roadNodesTowns;

    int64_t m_relativeArea   = 0;
    int64_t m_absoluteArea   = 0;
    int64_t m_absoluteRadius = 0;

    int64_t getPlacedArea() const
    {
        return m_innerArea.size();
    }
    int64_t getAreaDeficit() const
    {
        return m_absoluteArea - getPlacedArea();
    }

    int64_t getAreaDeficitPercent() const
    {
        return getAreaDeficit() * 100 / m_absoluteArea;
    }

    inline static FHPos makeCentroid(const TileRegion& region)
    {
        int64_t sumX = 0, sumY = 0;
        int64_t size = region.size();
        for (const auto* cell : region) {
            sumX += cell->m_pos.m_x;
            sumY += cell->m_pos.m_y;
        }
        sumX /= size;
        sumY /= size;
        int z = 0;
        return FHPos{ static_cast<int>(sumX), static_cast<int>(sumY), z };
    }

    void estimateCentroid()
    {
        m_centroid = makeCentroid(m_innerArea);
    }

    void readFromMap()
    {
        m_innerArea.clear();
        m_innerArea.insert(m_mapCanvas->m_tileIndex.at(m_centroid));

        makeEdgeFromInnerArea();

        while (true) {
            const bool growResult = tryGrowOnce([this](MapCanvas::Tile* cell) {
                return cell->m_zone == this;
            });
            if (!growResult)
                break;

            for (MapCanvas::Tile* cell : m_lastGrowed) {
                m_innerEdge.insert(cell);
                m_innerArea.insert(cell);
            }
            removeNonInnerFromInnerEdge();
        }

        for (auto& cell : m_mapCanvas->m_tiles) {
            if (cell.m_zone == this && !m_innerArea.contains(&cell))
                cell.m_zone = nullptr;
        }

        for (auto* cell : m_innerArea)
            cell->m_zone = this;
    }
    void readFromMapIfDirty()
    {
        if (!m_mapCanvas->m_dirtyZones.contains(this))
            return;
        m_mapCanvas->m_dirtyZones.erase(this);
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
        for (MapCanvas::Tile* cell : edge) {
            if (m_innerArea.contains(cell->m_neighborB)
                && m_innerArea.contains(cell->m_neighborT)
                && m_innerArea.contains(cell->m_neighborR)
                && m_innerArea.contains(cell->m_neighborL))
                continue;
            m_innerEdge.insert(cell);
        }

        makeOutsideEdge();
    }

    void makeOutsideEdge()
    {
        m_outsideEdge.clear();
        auto predicate = [this](MapCanvas::Tile* cell) {
            return cell && !m_innerArea.contains(cell);
        };
        for (auto* cell : m_innerEdge) {
            addIf(m_outsideEdge, cell->m_neighborB, predicate);
            addIf(m_outsideEdge, cell->m_neighborT, predicate);
            addIf(m_outsideEdge, cell->m_neighborR, predicate);
            addIf(m_outsideEdge, cell->m_neighborL, predicate);
        }
    }

    void addIf(TileRegion& reg, MapCanvas::Tile* cell, auto&& predicate)
    {
        if (!cell)
            return;
        if (!predicate(cell))
            return;
        reg.insert(cell);
    }

    bool tryGrowOnce(auto&& predicate)
    {
        bool result = false;
        m_lastGrowed.clear();
        for (auto* pos : m_outsideEdge) {
            if (predicate(pos)) {
                result = true;
                m_lastGrowed.insert(pos);
            }
        }
        return result;
    }

    bool tryGrowOnceToNeighbour(size_t limit, TileZone* prioritized)
    {
        const bool result = tryGrowOnce([this](MapCanvas::Tile* cell) {
            return cell->m_zone != this;
        });
        if (!result)
            return false;

        std::vector<MapCanvas::Tile*> nextEdge(m_lastGrowed.cbegin(), m_lastGrowed.cend());
        if (nextEdge.size() > limit && limit > 0) {
            std::nth_element(nextEdge.begin(), nextEdge.begin() + limit, nextEdge.end(), [this, prioritized](MapCanvas::Tile* l, MapCanvas::Tile* r) {
                if (prioritized) {
                    if (l->m_zone == prioritized && r->m_zone != prioritized)
                        return true;
                    if (l->m_zone != prioritized && r->m_zone == prioritized)
                        return false;
                }
                if (!l->m_zone && r->m_zone)
                    return true;
                if (l->m_zone && !r->m_zone)
                    return false;
                return posDistance(l->m_pos, this->m_centroid) < posDistance(r->m_pos, this->m_centroid);
            });
            nextEdge.resize(limit);
        }

        for (MapCanvas::Tile* cell : nextEdge) {
            if (cell->m_zone && cell->m_zone != this) {
                m_mapCanvas->m_dirtyZones.insert(cell->m_zone);
            }
            cell->m_zone = this;

            m_innerEdge.insert(cell);
            m_innerArea.insert(cell);
        }
        removeNonInnerFromInnerEdge();
        return true;
    }

    bool tryGrowOnceToUnzoned()
    {
        const bool result = tryGrowOnce([](MapCanvas::Tile* cell) {
            return cell->m_zone == nullptr;
        });
        if (!result)
            return false;

        for (MapCanvas::Tile* cell : m_lastGrowed) {
            cell->m_zone = this;

            m_innerEdge.insert(cell);
            m_innerArea.insert(cell);
        }
        removeNonInnerFromInnerEdge();
        return true;
    }

    void fillDeficit(int thresholdPercent, TileZone* prioritized)
    {
        while (true) {
            if (getAreaDeficitPercent() < thresholdPercent)
                break;
            if (!tryGrowOnceToNeighbour(10, prioritized))
                break;
        }
    }

    void fillUnzoned()
    {
        while (tryGrowOnceToUnzoned()) {
            ;
        }
    }
};

}
