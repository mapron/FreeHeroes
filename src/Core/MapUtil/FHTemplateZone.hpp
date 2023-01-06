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

namespace FreeHeroes {

struct TileZone {
    int                     m_index = 0;
    std::string             m_id;
    FHRngZone               m_rngZoneSettings;
    Core::IRandomGenerator* m_rng       = nullptr;
    MapCanvas*              m_mapCanvas = nullptr;

    using TileRegion = std::unordered_set<MapCanvas::Tile*>;

    FHPos m_startTile;
    FHPos m_centroid;

    TileRegion m_innerArea;
    TileRegion m_innerEdge;   // subset of innerArea;
    TileRegion m_outsideEdge; // is not subset of inner area.

    TileRegion m_lastGrowed;

    TileRegion m_roadNodes;

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

    void readFromMap()
    {
        m_innerArea.clear();
        m_innerArea.insert(m_mapCanvas->m_tileIndex.at(m_startTile));

        makeEdgeFromInnerArea();

        while (true) {
            const bool growResult = tryGrowOnce([this](MapCanvas::Tile* cell) {
                return cell->m_zone == this;
            });
            if (!growResult)
                break;
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
        const bool result = tryGrowOnce([this, allowConsumingNeighbours](MapCanvas::Tile* cell) {
            return !cell->m_zone || (allowConsumingNeighbours && cell->m_zone != this);
        });
        if (!result)
            return false;

        for (MapCanvas::Tile* cell : m_lastGrowed) {
            if (cell->m_zone && cell->m_zone != this) {
                m_mapCanvas->m_dirtyZones.insert(cell->m_zone);
            }
            cell->m_zone = this;
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
    }

    void fillTheRest()
    {
        while (tryGrowOnceToUnzoned(false)) {
            ;
        }
    }
};

}
