/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTileMap.hpp"

#include <stdexcept>
#include <set>
#include <unordered_map>

namespace FreeHeroes {

struct TileZone;
struct MapCanvas {
    struct Tile {
        FHPos m_pos;

        TileZone* m_zone = nullptr;

        bool m_exFix = false;

        Tile* m_neighborT = nullptr;
        Tile* m_neighborL = nullptr;
        Tile* m_neighborR = nullptr;
        Tile* m_neighborB = nullptr;

        std::vector<Tile*> m_allNeighbours;

        std::string posStr() const
        {
            return "(" + std::to_string(m_pos.m_x) + ", " + std::to_string(m_pos.m_y) + ", " + std::to_string(m_pos.m_z) + ")";
        }
    };
    std::vector<Tile> m_tiles;

    std::unordered_map<FHPos, Tile*> m_tileIndex;

    std::set<TileZone*> m_dirtyZones; // zone ids that must be re-read from the map.

    std::set<Tile*> m_blocked;
    std::set<Tile*> m_needBeBlocked;
    std::set<Tile*> m_tentativeBlocked;

    //std::set<FHPos> m_edge;

    void init(int width,
              int height,
              int depth)
    {
        m_tiles.resize(width * height * depth);
        size_t index = 0;
        for (int z = 0; z < depth; ++z) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    Tile* tile = &m_tiles[index++];
                    FHPos p{ x, y, z };
                    m_tileIndex[p] = tile;
                    tile->m_pos    = p;
                }
            }
        }
        index = 0;
        for (int z = 0; z < depth; ++z) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    Tile* tile = &m_tiles[index++];
                    if (x > 0) {
                        tile->m_neighborL = m_tileIndex.at({ x - 1, y, z });
                        tile->m_allNeighbours.push_back(tile->m_neighborL);
                    }
                    if (y > 0) {
                        tile->m_neighborT = m_tileIndex.at({ x, y - 1, z });
                        tile->m_allNeighbours.push_back(tile->m_neighborT);
                    }
                    if (x < width - 1) {
                        tile->m_neighborR = m_tileIndex.at({ x + 1, y, z });
                        tile->m_allNeighbours.push_back(tile->m_neighborR);
                    }
                    if (y < height - 1) {
                        tile->m_neighborB = m_tileIndex.at({ x, y + 1, z });
                        tile->m_allNeighbours.push_back(tile->m_neighborB);
                    }
                }
            }
        }
    }

    void checkAllTerrains(const std::set<Tile*>& posPlaced)
    {
        //std::set<FHPos> allTiles;
        for (auto& cell : m_tiles) {
            if (!posPlaced.contains(&cell)) {
                throw std::runtime_error("I forget to place tile (" + std::to_string(cell.m_pos.m_x) + ", " + std::to_string(cell.m_pos.m_y) + ")");
            }
        }
    }

    bool fixExclaves() // true = nothing to fix
    {
        int fixedCount = 0;
        for (auto& cell : m_tiles) {
            auto posT = posNeighbour(cell.m_pos, +0, -1);
            auto posL = posNeighbour(cell.m_pos, -1, +0);
            auto posR = posNeighbour(cell.m_pos, +1, +0);
            auto posB = posNeighbour(cell.m_pos, +0, +1);

            Tile&           tileX    = cell;
            TileZone* const oldIndex = tileX.m_zone;

            Tile& tileT = m_tileIndex.contains(posT) ? *m_tileIndex[posT] : cell;
            Tile& tileL = m_tileIndex.contains(posL) ? *m_tileIndex[posL] : cell;
            Tile& tileR = m_tileIndex.contains(posR) ? *m_tileIndex[posR] : cell;
            Tile& tileB = m_tileIndex.contains(posB) ? *m_tileIndex[posB] : cell;

            auto processTile = [&tileX, &tileT, &tileL, &tileR, &tileB]() -> bool { // true == nothing to do
                const bool eT        = tileX.m_zone == tileT.m_zone;
                const bool eL        = tileX.m_zone == tileL.m_zone;
                const bool eR        = tileX.m_zone == tileR.m_zone;
                const bool eB        = tileX.m_zone == tileB.m_zone;
                const int  sameCount = eT + eL + eR + eB;
                if (sameCount >= 3) { // normal center / border - do nothing
                    return true;
                }
                if (sameCount == 2) {
                    if ((eT && eL) || (eT && eR) || (eB && eL) || (eB && eR))
                        return true; // corner
                    if (eT && eB) {
                        tileX.m_zone = tileL.m_zone;
                        return false;
                    }
                    if (eR && eL) {
                        tileX.m_zone = tileT.m_zone;
                        return false;
                    }
                }
                if (sameCount == 1) {
                    if (eT) {
                        tileX.m_zone = tileB.m_zone;
                    } else if (eL) {
                        tileX.m_zone = tileR.m_zone;
                    } else if (eR) {
                        tileX.m_zone = tileL.m_zone;
                    } else if (eB) {
                        tileX.m_zone = tileT.m_zone;
                    }
                    return false;
                }
                // 1 tile exclave.
                if (tileT.m_zone == tileL.m_zone) {
                    tileX.m_zone = tileT.m_zone;
                } else if (tileT.m_zone == tileR.m_zone) {
                    tileX.m_zone = tileT.m_zone;
                } else if (tileB.m_zone == tileR.m_zone) {
                    tileX.m_zone = tileB.m_zone;
                } else if (tileB.m_zone == tileL.m_zone) {
                    tileX.m_zone = tileB.m_zone;
                } else {
                    tileX.m_zone = tileT.m_zone;
                }
                return false;
            };
            if (processTile())
                continue;
            fixedCount++;
            tileX.m_exFix = true;
            m_dirtyZones.insert(tileX.m_zone);
            if (oldIndex)
                m_dirtyZones.insert(oldIndex);

            //map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = cell.m_zone, .m_valueB = 0 });
        }
        return (fixedCount == 0);
    }
};
}
