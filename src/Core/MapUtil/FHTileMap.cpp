/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTileMap.hpp"

#include "IRandomGenerator.hpp"

#include "MernelPlatform/Logger.hpp"

#include <array>
#include <functional>
#include <fstream>
#include <cassert>

namespace FreeHeroes {
using namespace Mernel;

namespace {

using BorderType  = Core::LibraryTerrain::BorderType;
using BorderClass = Core::LibraryTerrain::BorderClass;

struct TileInfo {
    bool S = false;
    bool D = false;
    bool N = false;

    bool EQ = false; // equal terrain

    bool SD(bool s) const { return s ? S : D; }

    void set(FHTileMap::SubtileType st)
    {
        S = st == FHTileMap::SubtileType::Sand;
        D = st == FHTileMap::SubtileType::Dirt;
        N = st == FHTileMap::SubtileType::Native;
        assert(st != FHTileMap::SubtileType::Invalid);
    }

    char debugChar() const
    {
        if (S)
            return 'S';
        if (D)
            return 'D';
        if (N)
            return 'N';
        return '?';
    }
};

struct TileNeightbours {
    /*          T2
     *      TL  TC  TR
     *  L2  CL  XX  CR  R2
     *      BL  BC  BR
     *          B2
     */
    TileInfo TL, TR, BL, BR;

    TileInfo TC, CL, CR, BC;

    TileInfo T2, L2, R2, B2;

    bool m_coastal = false;

    bool m_flippedHor  = false;
    bool m_flippedVert = false;

    void read(Core::LibraryTerrainConstPtr dirtTerrain,
              Core::LibraryTerrainConstPtr sandTerrain,
              Core::LibraryTerrainConstPtr waterTerrain,
              const FHTileMap&             map,
              const FHTileMap::Tile&       tileXX,
              const FHPos&                 pos)
    {
        TL.set(tileXX.TL);
        TR.set(tileXX.TR);
        BL.set(tileXX.BL);
        BR.set(tileXX.BR);

        auto* terrTL = map.getNeighbour(pos, -1, -1).m_terrain;
        auto* terrTC = map.getNeighbour(pos, +0, -1).m_terrain;
        auto* terrTR = map.getNeighbour(pos, +1, -1).m_terrain;
        auto* terrCL = map.getNeighbour(pos, -1, +0).m_terrain;
        auto* terrCR = map.getNeighbour(pos, +1, +0).m_terrain;
        auto* terrBL = map.getNeighbour(pos, -1, +1).m_terrain;
        auto* terrBC = map.getNeighbour(pos, +0, +1).m_terrain;
        auto* terrBR = map.getNeighbour(pos, +1, +1).m_terrain;

        auto* terrT2 = map.getNeighbour(pos, +0, -2).m_terrain;
        auto* terrL2 = map.getNeighbour(pos, -2, +0).m_terrain;
        auto* terrR2 = map.getNeighbour(pos, +2, +0).m_terrain;
        auto* terrB2 = map.getNeighbour(pos, +0, +2).m_terrain;

        TL.EQ = terrTL == tileXX.m_terrain;
        TR.EQ = terrTR == tileXX.m_terrain;
        BL.EQ = terrBL == tileXX.m_terrain;
        BR.EQ = terrBR == tileXX.m_terrain;

        TC.EQ = terrTC == tileXX.m_terrain;
        CL.EQ = terrCL == tileXX.m_terrain;
        CR.EQ = terrCR == tileXX.m_terrain;
        BC.EQ = terrBC == tileXX.m_terrain;

        T2.EQ = terrT2 == tileXX.m_terrain;
        L2.EQ = terrL2 == tileXX.m_terrain;
        R2.EQ = terrR2 == tileXX.m_terrain;
        B2.EQ = terrB2 == tileXX.m_terrain;

        {
            const bool waterNeighbour = false
                                        || terrTL == waterTerrain
                                        || terrTC == waterTerrain
                                        || terrTR == waterTerrain
                                        || terrCL == waterTerrain
                                        || terrCR == waterTerrain
                                        || terrBL == waterTerrain
                                        || terrBC == waterTerrain
                                        || terrBR == waterTerrain;
            m_coastal = tileXX.m_terrain != waterTerrain && waterNeighbour;
        }
    }

    TileNeightbours flipped(bool vertical, bool horizontal)
    {
        if (!vertical && !horizontal)
            return *this;
        if (vertical && horizontal)
            return TileNeightbours{
                .TL = this->BR,
                .TR = this->BL,
                .BL = this->TR,
                .BR = this->TL,
                .TC = this->BC,
                .CL = this->CR,
                .CR = this->CL,
                .BC = this->TC,
                .T2 = this->B2,
                .L2 = this->R2,
                .R2 = this->L2,
                .B2 = this->T2,

                .m_flippedHor  = !this->m_flippedHor,
                .m_flippedVert = !this->m_flippedVert,
            };
        if (vertical)
            return TileNeightbours{
                .TL = this->BL,
                .TR = this->BR,
                .BL = this->TL,
                .BR = this->TR,
                .TC = this->BC,
                .CL = this->CL,
                .CR = this->CR,
                .BC = this->TC,
                .T2 = this->B2,
                .L2 = this->L2,
                .R2 = this->R2,
                .B2 = this->T2,

                .m_flippedHor  = false,
                .m_flippedVert = !this->m_flippedVert,
            };
        if (horizontal)
            return TileNeightbours{
                .TL = this->TR,
                .TR = this->TL,
                .BL = this->BR,
                .BR = this->BL,
                .TC = this->TC,
                .CL = this->CR,
                .CR = this->CL,
                .BC = this->BC,
                .T2 = this->T2,
                .L2 = this->R2,
                .R2 = this->L2,
                .B2 = this->B2,

                .m_flippedHor  = !this->m_flippedHor,
                .m_flippedVert = false,
            };
        return {};
    }

    std::string debugString() const
    {
        return std::string() + TL.debugChar() + TR.debugChar() + BL.debugChar() + BR.debugChar();
    }
};

struct PatternMatcher {
    BorderType  m_type  = BorderType::Invalid;
    BorderClass m_class = BorderClass::Invalid;

    bool m_doFlipHor  = true;
    bool m_doFlipVert = true;

    bool m_useOnDirt = true;

    std::function<bool(const TileNeightbours& t, bool sand)> m_f;
};

const std::vector<PatternMatcher> g_matchers{

    PatternMatcher{
        .m_type  = BorderType::TL,
        .m_class = BorderClass::NormalDirt,
        .m_f     = [](const TileNeightbours& t, bool s) {
            return (!t.BL.EQ && !t.TR.EQ
                    && t.TL.SD(s) && t.TR.SD(s)
                    && t.BL.SD(s) && t.BR.N);
        },
    },
    PatternMatcher{
        .m_type  = BorderType::TLS,
        .m_class = BorderClass::NormalDirt,
        .m_f     = [](const TileNeightbours& t, bool s) {
            return ((t.BL.EQ || t.TR.EQ)
                    && t.TL.SD(s) && t.TR.SD(s)
                    && t.BL.SD(s) && t.BR.N);
        },
    },

    PatternMatcher{
        .m_type       = BorderType::L,
        .m_class      = BorderClass::NormalDirt,
        .m_doFlipVert = false,
        .m_f          = [](const TileNeightbours& t, bool s) {
            return true
                   && t.TL.SD(s) && t.TR.N
                   && t.BL.SD(s) && t.BR.N;
        },
    },

    PatternMatcher{
        .m_type      = BorderType::T,
        .m_class     = BorderClass::NormalDirt,
        .m_doFlipHor = false,
        .m_f         = [](const TileNeightbours& t, bool s) {
            return true
                   && t.TL.SD(s) && t.TR.SD(s)
                   && t.BL.N && t.BR.N;
        },
    },

    PatternMatcher{
        .m_type  = BorderType::BR,
        .m_class = BorderClass::NormalDirt,
        .m_f     = [](const TileNeightbours& t, bool s) {
            return true
                   && t.TL.N && t.TR.N
                   && t.BL.N && t.BR.SD(s)
                   && t.R2.EQ && t.B2.EQ;
        },
    },
    PatternMatcher{
        .m_type  = BorderType::BRS,
        .m_class = BorderClass::NormalDirt,
        .m_f     = [](const TileNeightbours& t, bool s) {
            return true
                   && t.TL.N && t.TR.N
                   && t.BL.N && t.BR.SD(s)
                   && (!t.R2.EQ || !t.B2.EQ);
        },
    },
    // clang-format off
    PatternMatcher{
        .m_type       = BorderType::Mixed_DNND,
        .m_class      = BorderClass::Mixed,
        .m_doFlipHor  = false,
        .m_useOnDirt  = false,
        .m_f          = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.D && t.TR.N
                   && t.BL.N && t.BR.D;
        } 
    },
    PatternMatcher{
        .m_type       = BorderType::Mixed_DNNS,
        .m_class      = BorderClass::Mixed,
        .m_useOnDirt  = false,
        .m_f          = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.D && t.TR.N
                   && t.BL.N && t.BR.S;
        } 
    },
    // clang-format on
    PatternMatcher{
        .m_type      = BorderType::Mixed_SNNS,
        .m_class     = BorderClass::Mixed,
        .m_doFlipHor = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.S && t.TR.N
                   && t.BL.N && t.BR.S;
        },
    },

    PatternMatcher{
        .m_type      = BorderType::Mixed_NDSD,
        .m_class     = BorderClass::Mixed,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.D
                   && t.BL.S && t.BR.D;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Mixed_NSDD,
        .m_class     = BorderClass::Mixed,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.S
                   && t.BL.D && t.BR.D;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Mixed_NDNS,
        .m_class     = BorderClass::Mixed,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.D
                   && t.BL.N && t.BR.S;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Mixed_NNDS,
        .m_class     = BorderClass::Mixed,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.N
                   && t.BL.D && t.BR.S;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Mixed_NSDS,
        .m_class     = BorderClass::Mixed,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.S
                   && t.BL.D && t.BR.S;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Mixed_NDSS,
        .m_class     = BorderClass::Mixed,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.D
                   && t.BL.S && t.BR.S;
        },
    },
    // clang-format off
    PatternMatcher{ 
        .m_type       = BorderType::Center,
        .m_class      = BorderClass::Center,
        .m_doFlipHor  = false,
        .m_doFlipVert = false,
        .m_f = [](const TileNeightbours& t, bool) {
               return true
                      && t.TL.N && t.TR.N
                      && t.BL.N && t.BR.N;
           } 
    },
    // clang-format on
    PatternMatcher{
        .m_type      = BorderType::Special_DDDD,
        .m_class     = BorderClass::Special,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.D && t.TR.D
                   && t.BL.D && t.BR.D
                   && t.CR.EQ && t.BC.EQ;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Special_SSSS,
        .m_class     = BorderClass::Special,
        .m_useOnDirt = true,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.S && t.TR.S
                   && t.BL.S && t.BR.S
                   && t.CR.EQ && t.BC.EQ;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Special_DDDS,
        .m_class     = BorderClass::Special,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.D && t.TR.D
                   && t.BL.D && t.BR.S;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Special_SSSD,
        .m_class     = BorderClass::Special,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.S && t.TR.S
                   && t.BL.S && t.BR.D;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Special_NSSD,
        .m_class     = BorderClass::Special,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.S
                   && t.BL.S && t.BR.D;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::Special_NDDS,
        .m_class     = BorderClass::Special,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.N && t.TR.D
                   && t.BL.D && t.BR.S;
        },
    },
};
}

bool FHTileMap::Tile::setViewBorderSpecial(Core::LibraryTerrain::BorderType borderType)
{
    auto& pp = m_terrain->presentationParams;

    if (pp.specialBorderTilesOffset < 0)
        return setViewCenter();

    m_tileOffset = pp.specialBorderTilesOffset + pp.borderSpecialOffsets.at(borderType);
    m_tileCount  = pp.borderSpecialCounts.at(borderType);
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setViewBorderMixed(Core::LibraryTerrain::BorderType borderType)
{
    auto& pp = m_terrain->presentationParams;

    if (pp.mixedBorderTilesOffset < 0)
        return setViewCenter();

    m_tileOffset = pp.mixedBorderTilesOffset + pp.borderMixedOffsets.at(borderType);
    m_tileCount  = pp.borderMixedCounts.at(borderType);
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setViewBorderSandOrDirt(Core::LibraryTerrain::BorderType borderType, bool sandBorder)
{
    auto& pp = m_terrain->presentationParams;

    if (!sandBorder) {
        if (pp.dirtBorderTilesOffset < 0)
            return setViewCenter();

        m_tileOffset = pp.dirtBorderTilesOffset + pp.borderOffsets.at(borderType);
        m_tileCount  = pp.borderCounts.at(borderType);
    } else {
        if (pp.sandBorderTilesOffset < 0)
            return setViewCenter();

        m_tileOffset = pp.sandBorderTilesOffset + pp.borderOffsets.at(borderType);
        m_tileCount  = pp.borderCounts.at(borderType);
    }
    if (m_terrain->presentationParams.hasRotations) {
        if (borderType == BorderType::L)
            m_tileOffset += m_tileCount * m_flipHor;
        else if (borderType == BorderType::T)
            m_tileOffset += m_tileCount * m_flipVert;
        else
            m_tileOffset += m_tileCount * 2 * m_flipVert + m_tileCount * m_flipHor;

        m_flipHor  = false;
        m_flipVert = false;
    }
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setViewCenter()
{
    if (m_terrain) {
        auto& pp         = m_terrain->presentationParams;
        m_tileOffset     = pp.centerTilesOffset;
        m_tileCount      = pp.centerTilesCount;
        m_tileCountClear = pp.centerTilesClearCount;
    }
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setView(Core::LibraryTerrain::BorderClass bc, Core::LibraryTerrain::BorderType borderType)
{
    switch (bc) {
        case Core::LibraryTerrain::BorderClass::NormalDirt:
            return setViewBorderSandOrDirt(borderType, false);
        case Core::LibraryTerrain::BorderClass::NormalSand:
            return setViewBorderSandOrDirt(borderType, true);
        case Core::LibraryTerrain::BorderClass::Mixed:
            return setViewBorderMixed(borderType);
        case Core::LibraryTerrain::BorderClass::Center:
            return setViewCenter();
        case Core::LibraryTerrain::BorderClass::Special:
            return setViewBorderSpecial(borderType);
        case Core::LibraryTerrain::BorderClass::Invalid:
        default:
            assert(0);
            return false;
    }
}

void FHTileMap::Tile::updateMinMax()
{
    m_viewMin            = static_cast<uint8_t>(m_tileOffset);
    const int clearCount = m_tileCountClear > 0 ? m_tileCountClear : m_tileCount;
    m_viewMid            = static_cast<uint8_t>(m_viewMin + clearCount - 1);
    m_viewMax            = static_cast<uint8_t>(m_viewMin + m_tileCount - 1);
}

void FHTileMap::correctTerrainTypes(Core::LibraryTerrainConstPtr dirtTerrain,
                                    Core::LibraryTerrainConstPtr sandTerrain,
                                    Core::LibraryTerrainConstPtr waterTerrain)
{
    //std::ofstream ofs("E:/debug.txt");
    //int           row = 0;

    eachPos([this](const FHPos& pos) {
        FHTileMap::Tile& XX = get(pos);
        XX.TL               = SubtileType::Invalid;
        XX.TR               = SubtileType::Invalid;
        XX.BL               = SubtileType::Invalid;
        XX.BR               = SubtileType::Invalid;
    });

    eachPos([this,
             dirtTerrain,
             sandTerrain](const FHPos& pos) {
        FHTileMap::Tile& XX     = get(pos);
        auto             makest = [this,
                       dirtTerrain,
                       sandTerrain](const FHPos&     pos,
                                    FHTileMap::Tile& tileXX,
                                    SubtileType&     sub,
                                    int              dx,
                                    int              dy) {
            // fo sand, it contains only 4 native 'sand' subtiles.
            if (tileXX.m_terrain == sandTerrain) {
                sub = SubtileType::Native;
                return;
            }

            const FHTileMap::Tile& tileDX  = getNeighbour(pos, dx, 0);
            const FHTileMap::Tile& tileDY  = getNeighbour(pos, 0, dy);
            const FHTileMap::Tile& tileDXY = getNeighbour(pos, dx, dy);

            const bool eqDX  = tileDX.m_terrain == tileXX.m_terrain;
            const bool eqDY  = tileDY.m_terrain == tileXX.m_terrain;
            const bool eqDXY = tileDXY.m_terrain == tileXX.m_terrain;

            const bool allEq = eqDX && eqDY && eqDXY;
            if (allEq) {
                sub = SubtileType::Native;
                return;
            }

            const bool sandDX  = tileDX.m_terrain->tileBase == Core::LibraryTerrain::TileBase::Sand;
            const bool sandDY  = tileDY.m_terrain->tileBase == Core::LibraryTerrain::TileBase::Sand;
            const bool sandDXY = tileDXY.m_terrain->tileBase == Core::LibraryTerrain::TileBase::Sand;

            const bool anySand = sandDX || sandDY || sandDXY;
            if (anySand) {
                sub = SubtileType::Sand;
                return;
            }
            if (tileXX.m_terrain->tileBase == Core::LibraryTerrain::TileBase::Sand) {
                sub = SubtileType::Sand;
                return;
            }

            // dirt can contain mix of native 'dirt' and sand tiles.
            if (tileXX.m_terrain == dirtTerrain) {
                sub = SubtileType::Native;
                return;
            }
            sub = SubtileType::Dirt;
        };
        makest(pos, XX, XX.TL, -1, -1);
        makest(pos, XX, XX.TR, +1, -1);
        makest(pos, XX, XX.BL, -1, +1);
        makest(pos, XX, XX.BR, +1, +1);
    });

    eachPos([this](const FHPos& pos) {
        [[maybe_unused]] FHTileMap::Tile& XX = get(pos);

        assert(XX.TL != SubtileType::Invalid);
        assert(XX.TR != SubtileType::Invalid);
        assert(XX.BL != SubtileType::Invalid);
        assert(XX.BR != SubtileType::Invalid);
    });

    eachPos([this,
             dirtTerrain,
             sandTerrain,
             waterTerrain](const FHPos& pos) {
        auto& XX = get(pos);

        TileNeightbours tilen;
        tilen.read(dirtTerrain,
                   sandTerrain,
                   waterTerrain,
                   *this,
                   XX,
                   pos);
        XX.m_coastal = tilen.m_coastal;

        /*if (1) {
            if (row != pos.m_y)
                ofs << '\n';

            row = pos.m_y;
            ofs << tilen.debugString() << '\t';
        }*/

        if (XX.m_terrain == sandTerrain) {
            XX.setViewCenter();
            return;
        }

        std::array<TileNeightbours, 4> flipConfigs{
            { tilen, tilen.flipped(true, false), tilen.flipped(false, true), tilen.flipped(true, true) }
        };

        [[maybe_unused]] const bool isCorrected = [&flipConfigs, &XX, dirtTerrain, pos]() {
            std::vector<int> matchedBts;
            for (const TileNeightbours& t : flipConfigs) {
                for (const auto& matcher : g_matchers) {
                    if (!matcher.m_doFlipHor && t.m_flippedHor)
                        continue;
                    if (!matcher.m_doFlipVert && t.m_flippedVert)
                        continue;
                    if (XX.m_terrain == dirtTerrain && !matcher.m_useOnDirt) {
                        continue;
                    }

                    if (matcher.m_f(t, false)) {
                        XX.m_flipHor  = t.m_flippedHor;
                        XX.m_flipVert = t.m_flippedVert;
                        XX.setView(matcher.m_class, matcher.m_type);
                        matchedBts.push_back(static_cast<int>(matcher.m_type));
                    }
                    if (matcher.m_class == BorderClass::NormalDirt && matcher.m_f(t, true)) {
                        XX.m_flipHor  = t.m_flippedHor;
                        XX.m_flipVert = t.m_flippedVert;
                        XX.setView(BorderClass::NormalSand, matcher.m_type);
                        matchedBts.push_back(static_cast<int>(matcher.m_type));
                    }
                }
            }
            if (matchedBts.size() == 0) {
                XX.setViewCenter();
                Logger(Logger::Warning) << "failed to detect pattern at (" << pos.m_x << ',' << pos.m_y << ',' << pos.m_z << ")";
                return false;
            }
            if (matchedBts.size() > 1) {
                Logger(Logger::Warning) << "correctTerrainTypes: found [" << matchedBts.size() << "] patterns at (" << pos.m_x << ',' << pos.m_y << ',' << pos.m_z << "): " << matchedBts;
            }

            return true;
        }();
    });
}

void FHTileMap::correctRoads()
{
    // true = pattern found
    auto correctTile = [this](const FHPos& pos, bool flipHor, bool flipVert, int order) -> bool {
        /* TL  T  TR
         *  L  X   R
         * BL  B  BR
         */
        const Tile def;
        // const auto& TL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +1 : -1);
        const auto& T  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? +1 : -1, def);
        const auto& TR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +1 : -1, def);
        const auto& L  = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +0 : +0, def);
        auto&       X  = get(pos);
        const auto& R  = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +0 : +0, def);
        const auto& BL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? -1 : +1, def);
        const auto& B  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -1 : +1, def);
        // const auto& BR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? -1 : +1);

        const auto eR  = X.m_roadType == R.m_roadType;
        const auto eL  = X.m_roadType == L.m_roadType;
        const auto eT  = X.m_roadType == T.m_roadType;
        const auto eB  = X.m_roadType == B.m_roadType;
        const auto eTR = X.m_roadType == TR.m_roadType;
        const auto eBL = X.m_roadType == BL.m_roadType;

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
        } else if (order == 2 && !eR && !eL && !eT && !eB) {
            setView(14, 14);
            X.m_roadFlipHor  = false;
            X.m_roadFlipVert = true;
        } else {
            return false;
        }
        return true;
    };

    for (int z = 0; z < m_depth; ++z) {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
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
        const Tile def;
        //const auto& TL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +1 : -1);
        const auto& T = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? +1 : -1, def);
        // const auto& TR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +1 : -1);
        const auto& L = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +0 : +0, def);
        auto&       X = get(pos);
        const auto& R = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +0 : +0, def);
        //const auto& BL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? -1 : +1);
        const auto& B = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -1 : +1, def);
        //const auto& BR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? -1 : +1);

        const auto eR = X.m_riverType == R.m_riverType;
        const auto eL = X.m_riverType == L.m_riverType;
        const auto eT = X.m_riverType == T.m_riverType;
        const auto eB = X.m_riverType == B.m_riverType;

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

    for (int z = 0; z < m_depth; ++z) {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
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

void FHTileMap::rngTiles(Core::IRandomGenerator* rng, int roughTileChancePercent)
{
    auto rngView = [&rng](uint8_t min, uint8_t max) -> uint8_t {
        if (min == max)
            return min;
        uint8_t diff   = max - min;
        uint8_t result = rng->genSmall(diff);
        //if (result >= 20)
        //    result = rng->genSmall(diff);
        return min + result;
    };

    for (int z = 0; z < m_depth; ++z) {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                auto& X = get(x, y, z);
                if (!(X.m_view >= X.m_viewMin && X.m_view <= X.m_viewMax)) {
                    if (X.m_view != 0xffU)
                        Logger(Logger::Warning) << "Change view at (" << x << "," << y << "," << int(z) << ") " << int(X.m_view) << " -> [" << int(X.m_viewMin) << " .. " << int(X.m_viewMax) << "]";
                    if (rng->genSmall(100) < roughTileChancePercent)
                        X.m_view = rngView(X.m_viewMin, X.m_viewMax);
                    else
                        X.m_view = rngView(X.m_viewMin, X.m_viewMid);
                }
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
        for (int x = 0; x < rect.m_width; ++x) {
            for (int y = 0; y < rect.m_height; ++y) {
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
