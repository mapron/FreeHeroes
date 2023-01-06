/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTileMap.hpp"

#include <stdexcept>
#include <set>

namespace FreeHeroes {

struct MapCanvas {
    struct Tile {
        bool m_zoned     = false;
        int  m_zoneIndex = -1;

        bool m_exFix = false;
    };

    std::map<FHPos, Tile> m_tiles;

    std::set<int> m_dirtyZones; // zone ids that must be re-read from the map.

    std::set<FHPos> m_edge;

    void init(int width,
              int height,
              int depth)
    {
        for (int z = 0; z < depth; ++z) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    m_tiles[FHPos{ x, y, z }] = {};
                }
                m_edge.insert(FHPos{ 0, y, z });
                m_edge.insert(FHPos{ width - 1, y, z });
            }
            for (int x = 0; x < width; ++x) {
                m_edge.insert(FHPos{ x, 0, z });
                m_edge.insert(FHPos{ x, height - 1, z });
            }
        }
    }

    void checkUnzoned()
    {
        for (auto& [pos, cell] : m_tiles) {
            if (!cell.m_zoned)
                throw std::runtime_error("All tiles must be zoned!");
        }
    }

    void checkAllTerrains(const std::set<FHPos>& posPlaced)
    {
        //std::set<FHPos> allTiles;
        for (auto& [pos, cell] : m_tiles) {
            if (!posPlaced.contains(pos)) {
                throw std::runtime_error("I forget to place tile (" + std::to_string(pos.m_x) + ", " + std::to_string(pos.m_y) + ")");
            }
        }
    }

    bool fixExclaves() // true = nothing to fix
    {
        int fixedCount = 0;
        for (auto& [pos, cell] : m_tiles) {
            auto posT = posNeighbour(pos, +0, -1);
            auto posL = posNeighbour(pos, -1, +0);
            auto posR = posNeighbour(pos, +1, +0);
            auto posB = posNeighbour(pos, +0, +1);

            Tile&     tileX    = cell;
            const int oldIndex = tileX.m_zoneIndex;

            Tile& tileT = m_tiles.contains(posT) ? m_tiles[posT] : cell;
            Tile& tileL = m_tiles.contains(posL) ? m_tiles[posL] : cell;
            Tile& tileR = m_tiles.contains(posR) ? m_tiles[posR] : cell;
            Tile& tileB = m_tiles.contains(posB) ? m_tiles[posB] : cell;

            auto processTile = [&tileX, &tileT, &tileL, &tileR, &tileB]() -> bool { // true == nothing to do
                const bool eT        = tileX.m_zoneIndex == tileT.m_zoneIndex;
                const bool eL        = tileX.m_zoneIndex == tileL.m_zoneIndex;
                const bool eR        = tileX.m_zoneIndex == tileR.m_zoneIndex;
                const bool eB        = tileX.m_zoneIndex == tileB.m_zoneIndex;
                const int  sameCount = eT + eL + eR + eB;
                if (sameCount >= 3) { // normal center / border - do nothing
                    return true;
                }
                if (sameCount == 2) {
                    if ((eT && eL) || (eT && eR) || (eB && eL) || (eB && eR))
                        return true; // corner
                    if (eT && eB) {
                        tileX.m_zoneIndex = tileL.m_zoneIndex;
                        return false;
                    }
                    if (eR && eL) {
                        tileX.m_zoneIndex = tileT.m_zoneIndex;
                        return false;
                    }
                }
                if (sameCount == 1) {
                    if (eT) {
                        tileX.m_zoneIndex = tileB.m_zoneIndex;
                    } else if (eL) {
                        tileX.m_zoneIndex = tileR.m_zoneIndex;
                    } else if (eR) {
                        tileX.m_zoneIndex = tileL.m_zoneIndex;
                    } else if (eB) {
                        tileX.m_zoneIndex = tileT.m_zoneIndex;
                    }
                    return false;
                }
                // 1 tile exclave.
                if (tileT.m_zoneIndex == tileL.m_zoneIndex) {
                    tileX.m_zoneIndex = tileT.m_zoneIndex;
                } else if (tileT.m_zoneIndex == tileR.m_zoneIndex) {
                    tileX.m_zoneIndex = tileT.m_zoneIndex;
                } else if (tileB.m_zoneIndex == tileR.m_zoneIndex) {
                    tileX.m_zoneIndex = tileB.m_zoneIndex;
                } else if (tileB.m_zoneIndex == tileL.m_zoneIndex) {
                    tileX.m_zoneIndex = tileB.m_zoneIndex;
                } else {
                    tileX.m_zoneIndex = tileT.m_zoneIndex;
                }
                return false;
            };
            if (processTile())
                continue;
            fixedCount++;
            tileX.m_exFix = true;
            m_dirtyZones.insert(tileX.m_zoneIndex);
            m_dirtyZones.insert(oldIndex);

            //map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = cell.m_zoneIndex, .m_valueB = 0 });
        }
        return (fixedCount == 0);
    }
};

}
