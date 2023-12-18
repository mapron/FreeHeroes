/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MapTileContainer.hpp"
#include "MapTileRegion.hpp"

namespace FreeHeroes {

void MapTileContainer::init(int width, int height, int depth)
{
    m_width  = width;
    m_height = height;
    m_depth  = depth;

    m_tiles.resize(width * height * depth);
    size_t index = 0;
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                MapTilePtr tile = &m_tiles[index++];
                FHPos      p{ x, y, z };
                m_tileIndex[p]    = tile;
                tile->m_pos       = p;
                tile->m_container = this;
            }
        }
    }
    index = 0;
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                MapTilePtr tile = &m_tiles[index++];
                if (x > 0) {
                    tile->m_neighborL = m_tileIndex.at({ x - 1, y, z });
                    tile->m_orthogonalNeighbours.push_back(tile->m_neighborL);
                }
                if (y > 0) {
                    tile->m_neighborT = m_tileIndex.at({ x, y - 1, z });
                    tile->m_orthogonalNeighbours.push_back(tile->m_neighborT);
                }
                if (x < width - 1) {
                    tile->m_neighborR = m_tileIndex.at({ x + 1, y, z });
                    tile->m_orthogonalNeighbours.push_back(tile->m_neighborR);
                }
                if (y < height - 1) {
                    tile->m_neighborB = m_tileIndex.at({ x, y + 1, z });
                    tile->m_orthogonalNeighbours.push_back(tile->m_neighborB);
                }
                tile->m_allNeighboursWithDiag = tile->m_orthogonalNeighbours;
            }
        }
    }
    MapTilePtrSortedList allSorted;
    allSorted.reserve(m_tiles.size());
    index = 0;
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                MapTilePtr tile = &m_tiles[index++];
                if (tile->m_neighborT)
                    tile->m_neighborTL = tile->m_neighborT->m_neighborL;
                if (tile->m_neighborT)
                    tile->m_neighborTR = tile->m_neighborT->m_neighborR;
                if (tile->m_neighborB)
                    tile->m_neighborBL = tile->m_neighborB->m_neighborL;
                if (tile->m_neighborB)
                    tile->m_neighborBR = tile->m_neighborB->m_neighborR;

                if (tile->m_neighborTL) {
                    tile->m_allNeighboursWithDiag.push_back(tile->m_neighborTL);
                    tile->m_diagNeighbours.push_back(tile->m_neighborTL);
                }
                if (tile->m_neighborTR) {
                    tile->m_allNeighboursWithDiag.push_back(tile->m_neighborTR);
                    tile->m_diagNeighbours.push_back(tile->m_neighborTR);
                }
                if (tile->m_neighborBL) {
                    tile->m_allNeighboursWithDiag.push_back(tile->m_neighborBL);
                    tile->m_diagNeighbours.push_back(tile->m_neighborBL);
                }
                if (tile->m_neighborBR) {
                    tile->m_allNeighboursWithDiag.push_back(tile->m_neighborBR);
                    tile->m_diagNeighbours.push_back(tile->m_neighborBR);
                }
                allSorted.push_back(tile);
            }
        }
    }
    m_all       = MapTileRegion(std::move(allSorted));
    m_innerEdge = m_all.makeInnerEdge(true); // not optimal, but it runs once
    for (auto& cell : m_tiles) {
        cell.m_self = &cell;
        std::sort(cell.m_orthogonalNeighbours.begin(), cell.m_orthogonalNeighbours.end());
        std::sort(cell.m_diagNeighbours.begin(), cell.m_diagNeighbours.end());
        std::sort(cell.m_allNeighboursWithDiag.begin(), cell.m_allNeighboursWithDiag.end());
    }

    m_centerTile = m_tileIndex[FHPos{ width / 2, height / 2, 0 }];
}

}
