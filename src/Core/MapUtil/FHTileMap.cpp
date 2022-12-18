/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTileMap.hpp"

#include "IRandomGenerator.hpp"

namespace FreeHeroes {

void FHTileMap::Tile::calculateOffsets(Core::LibraryTerrain::BorderType borderType, bool dirtBorder, bool sandBorder)
{
    auto& pp = m_terrain->presentationParams;
    if (dirtBorder && sandBorder) {
        if (pp.sandDirtBorderTilesOffset < 0)
            return calculateOffsets(Core::LibraryTerrain::BorderType::Center, false, false);

        m_tileOffset = pp.sandDirtBorderTilesOffset + pp.borderThreeWayOffsets.at(borderType);
        m_tileCount  = pp.borderThreeWayCounts.at(borderType);
    } else if (dirtBorder) {
        if (pp.dirtBorderTilesOffset < 0)
            return calculateOffsets(Core::LibraryTerrain::BorderType::Center, false, false);

        m_tileOffset = pp.dirtBorderTilesOffset + pp.borderOffsets.at(borderType);
        m_tileCount  = pp.borderCounts.at(borderType);
    } else if (sandBorder) {
        if (pp.sandBorderTilesOffset < 0)
            return calculateOffsets(Core::LibraryTerrain::BorderType::Center, false, false);

        m_tileOffset = pp.sandBorderTilesOffset + pp.borderOffsets.at(borderType);
        m_tileCount  = pp.borderCounts.at(borderType);
    } else {
        m_tileOffset = pp.centerTilesOffset;
        m_tileCount  = pp.centerTilesCount;
    }
}

void FHTileMap::correctTerrainTypes(Core::LibraryTerrainConstPtr dirtTerrain,
                                    Core::LibraryTerrainConstPtr sandTerrain,
                                    Core::LibraryTerrainConstPtr waterTerrain)
{
    // true = pattern found
    auto correctTile = [this, sandTerrain, waterTerrain](const FHPos& pos, bool flipHor, bool flipVert, int order) -> bool {
        /* TL  T  TR
         *  L  X   R
         * BL  B  BR
         */
        const auto& TL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +1 : -1);
        const auto& T  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? +1 : -1);
        const auto& TR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +1 : -1);
        const auto& L  = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +0 : +0);
        auto&       X  = get(pos);
        const auto& R  = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +0 : +0);
        const auto& BL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? -1 : +1);
        const auto& B  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -1 : +1);
        const auto& BR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? -1 : +1);

        const bool waterX = X.m_terrain == waterTerrain;
        const auto dR     = X.m_terrain != R.m_terrain;
        const auto dL     = X.m_terrain != L.m_terrain;
        const auto dT     = X.m_terrain != T.m_terrain;
        const auto dB     = X.m_terrain != B.m_terrain;

        //const auto dTL = X.m_terrain != TL.m_terrain;
        const auto dTR = X.m_terrain != TR.m_terrain;
        const auto dBL = X.m_terrain != BL.m_terrain;
        const auto dBR = X.m_terrain != BR.m_terrain;

        const auto sandR  = R.m_terrain == sandTerrain || (!waterX && R.m_terrain == waterTerrain);
        const auto sandL  = L.m_terrain == sandTerrain || (!waterX && L.m_terrain == waterTerrain);
        const auto sandT  = T.m_terrain == sandTerrain || (!waterX && T.m_terrain == waterTerrain);
        const auto sandB  = B.m_terrain == sandTerrain || (!waterX && B.m_terrain == waterTerrain);
        const auto sandBR = BR.m_terrain == sandTerrain || (!waterX && BR.m_terrain == waterTerrain);
        const auto sandBL = BL.m_terrain == sandTerrain || (!waterX && BL.m_terrain == waterTerrain);
        const auto sandTR = TR.m_terrain == sandTerrain || (!waterX && TR.m_terrain == waterTerrain);

        if (!waterX) {
            X.m_coastal = false
                          || R.m_terrain == waterTerrain
                          || L.m_terrain == waterTerrain
                          || T.m_terrain == waterTerrain
                          || B.m_terrain == waterTerrain
                          || BR.m_terrain == waterTerrain
                          || TR.m_terrain == waterTerrain
                          || TL.m_terrain == waterTerrain
                          || BL.m_terrain == waterTerrain;
        }
        using BT = Core::LibraryTerrain::BorderType;

        auto setView = [&X, flipHor, flipVert, waterTerrain](BT borderType, bool dirt, bool sand) {
            if (dirt && !sand && X.m_terrain == waterTerrain) {
                dirt = false;
                sand = true;
            }
            X.calculateOffsets(borderType, dirt, sand);
            X.m_viewMin  = static_cast<uint8_t>(X.m_tileOffset);
            X.m_viewMax  = static_cast<uint8_t>(X.m_viewMin + X.m_tileCount - 1);
            X.m_flipHor  = flipHor;
            X.m_flipVert = flipVert;
        };

        if (false) {
        } else if (order == 0 && !waterX) {
            if (false) {
                /// @todo:
                setView(BT::ThreeWay_DD, true, true);
                setView(BT::ThreeWay_DS, true, true);
                setView(BT::ThreeWay_SS, true, true);
            } else if (sandBL && dR && !sandR && !sandB) {
                setView(BT::ThreeWay_RD_BLS, true, true);
            } else if (sandTR && dB && !sandB && !sandR) {
                setView(BT::ThreeWay_BD_TRS, true, true); // !<
            } else if (sandR && dB && !sandB) {
                setView(BT::ThreeWay_RS_BD, true, true);
            } else if (sandB && dR && !sandR) {
                setView(BT::ThreeWay_BS_RD, true, true);
            } else if (sandBR && dR && !dT && !sandR) {
                setView(BT::ThreeWay_TRD_BRS, true, true);
            } else if (sandBR && dB && !dR && !sandB) {
                setView(BT::ThreeWay_BRS_BLD, true, true);
            } else {
                return false;
            }
        } else if (order == 1 && dL && dT) {
            setView(BT::TL, !sandL, sandL);
            if (!dTR || !dBL) {
                setView(BT::TLS, !sandL, sandL);
            }
        } else if (order == 2 && dL) {
            setView(BT::L, !sandL, sandL);
        } else if (order == 2 && dT) {
            setView(BT::T, !sandT, sandT);
        } else if (order == 3 && dBR) {
            setView(BT::BR, !sandBR, sandBR);

            const auto& B2  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -2 : +2);
            const auto& R2  = getNeighbour(pos, flipHor ? -2 : +2, flipVert ? +0 : +0);
            const auto  dB2 = X.m_terrain != B2.m_terrain;
            const auto  dR2 = X.m_terrain != R2.m_terrain;
            if (dB2 || dR2) {
                setView(BT::BRS, !sandBR, sandBR);
            }
        } else {
            return false;
        }
        return true;
    };

    for (uint8_t z = 0; z < m_depth; ++z) {
        for (uint32_t y = 0; y < m_height; ++y) {
            for (uint32_t x = 0; x < m_width; ++x) {
                const FHPos pos{ x, y, z };
                auto&       X = get(pos);
                if (X.m_terrain == sandTerrain || X.m_terrain == dirtTerrain)
                    continue;

                const bool tileCorrected = [&correctTile, &pos]() {
                    for (int order = 0; order <= 3; ++order) {
                        for (int flipHor = 0; flipHor <= 1; ++flipHor) {
                            for (int flipVert = 0; flipVert <= 1; ++flipVert) {
                                if (correctTile(pos, flipHor, flipVert, order))
                                    return true;
                            }
                        }
                    }
                    return false;
                }();
                if (tileCorrected)
                    continue;

                const uint8_t offset     = X.m_terrain->presentationParams.centerTilesOffset;
                const auto    centerSize = X.m_terrain->presentationParams.centerTilesCount;
                X.m_viewMin              = offset;
                X.m_viewMax              = offset + (centerSize <= 0 ? 0 : centerSize - 1);
            }
        }
    }
}

void FHTileMap::correctRoads()
{
    // true = pattern found
    auto correctTile = [this](const FHPos& pos, bool flipHor, bool flipVert, int order) -> bool {
        /* TL  T  TR
         *  L  X   R
         * BL  B  BR
         */
        // const auto& TL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +1 : -1);
        const auto& T  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? +1 : -1);
        const auto& TR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +1 : -1);
        const auto& L  = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +0 : +0);
        auto&       X  = get(pos);
        const auto& R  = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +0 : +0);
        const auto& BL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? -1 : +1);
        const auto& B  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -1 : +1);
        // const auto& BR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? -1 : +1);

        const auto eR  = X.m_roadType == R.m_roadType && &X != &R;
        const auto eL  = X.m_roadType == L.m_roadType && &X != &L;
        const auto eT  = X.m_roadType == T.m_roadType && &X != &T;
        const auto eB  = X.m_roadType == B.m_roadType && &X != &B;
        const auto eTR = X.m_roadType == TR.m_roadType && &X != &TR;
        const auto eBL = X.m_roadType == BL.m_roadType && &X != &BL;

        auto setView = [&X, flipHor, flipVert](uint8_t min, uint8_t max) {
            X.m_roadViewMin  = min;
            X.m_roadViewMax  = max;
            X.m_roadFlipHor  = flipHor;
            X.m_roadFlipVert = flipVert;
        };

        if (false) {
        } else if (order == 0 && eR && !eL && !eT && eB && (eTR || eBL)) {
            setView(2, 5);
        } else if (order == 1 && eR && eL && eT && eB) {
            setView(16, 16);
        } else if (order == 1 && eR && eL && !eT && eB) {
            setView(8, 9);
        } else if (order == 1 && eR && !eL && eT && eB) {
            setView(6, 7);
        } else if (order == 1 && !eR && !eL && eT && eB) {
            setView(10, 11);
        } else if (order == 1 && eR && eL && !eT && !eB) {
            setView(12, 13);
        } else if (order == 1 && eR && !eL && !eT && eB) {
            setView(0, 1);
        } else if (order == 2 && !eR && !eL && !eT && eB) {
            setView(14, 14);
        } else if (order == 2 && eR && !eL && !eT && !eB) {
            setView(15, 15);
        } else {
            return false;
        }
        return true;
    };

    for (uint8_t z = 0; z < m_depth; ++z) {
        for (uint32_t y = 0; y < m_height; ++y) {
            for (uint32_t x = 0; x < m_width; ++x) {
                const FHPos pos{ x, y, z };
                auto&       X = get(pos);
                if (X.m_roadType == FHRoadType::Invalid)
                    continue;

                const bool tileCorrected = [&correctTile, &pos]() {
                    for (int order = 0; order <= 2; ++order) {
                        for (int flipHor = 0; flipHor <= 1; ++flipHor) {
                            for (int flipVert = 0; flipVert <= 1; ++flipVert) {
                                if (correctTile(pos, flipHor, flipVert, order))
                                    return true;
                            }
                        }
                    }
                    return false;
                }();
                if (tileCorrected)
                    continue;

                X.m_roadViewMin  = 9;
                X.m_roadViewMax  = 10;
                X.m_roadFlipHor  = false;
                X.m_roadFlipVert = false;
            }
        }
    }
}

void FHTileMap::correctRivers()
{
    // true = pattern found
    auto correctTile = [this](const FHPos& pos, bool flipHor, bool flipVert, int order) -> bool {
        /* TL  T  TR
         *  L  X   R
         * BL  B  BR
         */
        //const auto& TL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +1 : -1);
        const auto& T = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? +1 : -1);
        // const auto& TR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +1 : -1);
        const auto& L = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +0 : +0);
        auto&       X = get(pos);
        const auto& R = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +0 : +0);
        //const auto& BL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? -1 : +1);
        const auto& B = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -1 : +1);
        //const auto& BR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? -1 : +1);

        const auto eR = X.m_riverType == R.m_riverType && &X != &R;
        const auto eL = X.m_riverType == L.m_riverType && &X != &L;
        const auto eT = X.m_riverType == T.m_riverType && &X != &T;
        const auto eB = X.m_riverType == B.m_riverType && &X != &B;

        auto setView = [&X, flipHor, flipVert](uint8_t min, uint8_t max) {
            X.m_riverViewMin  = min;
            X.m_riverViewMax  = max;
            X.m_riverFlipHor  = flipHor;
            X.m_riverFlipVert = flipVert;
        };

        if (false) {
        } else if (order == 0 && eR && eL && eT && eB) {
            setView(4, 4);
        } else if (order == 0 && eR && eL && !eT && eB) {
            setView(5, 6);
        } else if (order == 0 && eR && !eL && eT && eB) {
            setView(7, 8);
        } else if (order == 0 && !eR && !eL && eT && eB) {
            setView(9, 10);
        } else if (order == 0 && eR && eL && !eT && !eB) {
            setView(11, 12);
        } else if (order == 0 && eR && !eL && !eT && eB) {
            setView(0, 3);
        } else if (order == 1 && !eR && !eL && !eT && eB) {
            setView(9, 10);
        } else if (order == 1 && !eR && !eL && eT && !eB) {
            setView(9, 10);
        } else if (order == 1 && eR && !eL && !eT && !eB) {
            setView(11, 12);
        } else if (order == 1 && !eR && eL && !eT && !eB) {
            setView(11, 12);
        } else {
            return false;
        }
        return true;
    };

    for (uint8_t z = 0; z < m_depth; ++z) {
        for (uint32_t y = 0; y < m_height; ++y) {
            for (uint32_t x = 0; x < m_width; ++x) {
                const FHPos pos{ x, y, z };
                auto&       X = get(pos);
                if (X.m_riverType == FHRiverType::Invalid)
                    continue;

                const bool tileCorrected = [&correctTile, &pos]() {
                    for (int order = 0; order <= 1; ++order) {
                        for (int flipHor = 0; flipHor <= 1; ++flipHor) {
                            for (int flipVert = 0; flipVert <= 1; ++flipVert) {
                                if (correctTile(pos, flipHor, flipVert, order))
                                    return true;
                            }
                        }
                    }
                    return false;
                }();
                if (tileCorrected)
                    continue;

                X.m_riverViewMin  = 9;
                X.m_riverViewMax  = 10;
                X.m_riverFlipHor  = false;
                X.m_riverFlipVert = false;
            }
        }
    }
}

void FHTileMap::rngTiles(Core::IRandomGenerator* rng)
{
    auto rngView = [&rng](uint8_t min, uint8_t max) -> uint8_t {
        if (min == max)
            return min;
        uint8_t diff   = max - min;
        uint8_t result = rng->genSmall(diff);
        if (result >= 20)
            result = rng->genSmall(diff);
        return min + result;
    };

    for (uint8_t z = 0; z < m_depth; ++z) {
        for (uint32_t y = 0; y < m_height; ++y) {
            for (uint32_t x = 0; x < m_width; ++x) {
                auto& X = get(x, y, z);
                if (!(X.m_view >= X.m_viewMin && X.m_view <= X.m_viewMax))
                    X.m_view = rngView(X.m_viewMin, X.m_viewMax);
                if (!(X.m_roadView >= X.m_roadViewMin && X.m_roadView <= X.m_roadViewMax))
                    X.m_roadView = rngView(X.m_roadViewMin, X.m_roadViewMax);
                if (!(X.m_riverView >= X.m_riverViewMin && X.m_riverView <= X.m_riverViewMax))
                    X.m_riverView = rngView(X.m_riverViewMin, X.m_riverViewMax);
            }
        }
    }
}

void FHZone::placeOnMap(FHTileMap& map) const
{
    if (m_rect.has_value()) {
        auto& rect = m_rect.value();
        for (uint32_t x = 0; x < rect.m_width; ++x) {
            for (uint32_t y = 0; y < rect.m_height; ++y) {
                map.get(FHPos{ .m_x = x + rect.m_pos.m_x,
                               .m_y = y + rect.m_pos.m_y,
                               .m_z = rect.m_pos.m_z })
                    .m_terrain
                    = m_terrainId;
            }
        }
        return;
    }
    if (!m_tiles.empty() && m_tilesVariants.size() == m_tiles.size()) {
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            auto& tile     = map.get(m_tiles[i]);
            tile.m_terrain = m_terrainId;
            tile.m_view    = m_tilesVariants[i];
        }
        return;
    }
    if (!m_tiles.empty()) {
        for (auto& pos : m_tiles) {
            map.get(pos).m_terrain = m_terrainId;
        }
    }
}

void FHRiver::placeOnMap(FHTileMap& map) const
{
    if (!m_tiles.empty() && m_tilesVariants.size() == m_tiles.size()) {
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            auto& tile       = map.get(m_tiles[i]);
            tile.m_riverType = m_type;
            tile.m_riverView = m_tilesVariants[i];
        }
        return;
    }
    if (!m_tiles.empty()) {
        for (auto& pos : m_tiles) {
            map.get(pos).m_riverType = m_type;
        }
    }
}

void FHRoad::placeOnMap(FHTileMap& map) const
{
    if (!m_tiles.empty() && m_tilesVariants.size() == m_tiles.size()) {
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            auto& tile      = map.get(m_tiles[i]);
            tile.m_roadType = m_type;
            tile.m_roadView = m_tilesVariants[i];
        }
        return;
    }
    if (!m_tiles.empty()) {
        for (auto& pos : m_tiles) {
            map.get(pos).m_roadType = m_type;
        }
    }
}

}
