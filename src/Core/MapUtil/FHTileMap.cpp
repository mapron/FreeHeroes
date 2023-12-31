/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTileMap.hpp"

#include "IRandomGenerator.hpp"
#include "IGameDatabase.hpp"

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

        auto* terrTL = map.getNeighbour(pos, -1, -1).m_terrainId;
        auto* terrTC = map.getNeighbour(pos, +0, -1).m_terrainId;
        auto* terrTR = map.getNeighbour(pos, +1, -1).m_terrainId;
        auto* terrCL = map.getNeighbour(pos, -1, +0).m_terrainId;
        auto* terrCR = map.getNeighbour(pos, +1, +0).m_terrainId;
        auto* terrBL = map.getNeighbour(pos, -1, +1).m_terrainId;
        auto* terrBC = map.getNeighbour(pos, +0, +1).m_terrainId;
        auto* terrBR = map.getNeighbour(pos, +1, +1).m_terrainId;

        auto* terrT2 = map.getNeighbour(pos, +0, -2).m_terrainId;
        auto* terrL2 = map.getNeighbour(pos, -2, +0).m_terrainId;
        auto* terrR2 = map.getNeighbour(pos, +2, +0).m_terrainId;
        auto* terrB2 = map.getNeighbour(pos, +0, +2).m_terrainId;

        TL.EQ = terrTL == tileXX.m_terrainId;
        TR.EQ = terrTR == tileXX.m_terrainId;
        BL.EQ = terrBL == tileXX.m_terrainId;
        BR.EQ = terrBR == tileXX.m_terrainId;

        TC.EQ = terrTC == tileXX.m_terrainId;
        CL.EQ = terrCL == tileXX.m_terrainId;
        CR.EQ = terrCR == tileXX.m_terrainId;
        BC.EQ = terrBC == tileXX.m_terrainId;

        T2.EQ = terrT2 == tileXX.m_terrainId;
        L2.EQ = terrL2 == tileXX.m_terrainId;
        R2.EQ = terrR2 == tileXX.m_terrainId;
        B2.EQ = terrB2 == tileXX.m_terrainId;

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
            m_coastal = tileXX.m_terrainId != waterTerrain && waterNeighbour;
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
        .m_type       = BorderType::Special_DDDD,
        .m_class      = BorderClass::Special,
        .m_doFlipHor  = false,
        .m_doFlipVert = false,
        .m_useOnDirt  = false,
        .m_f          = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.D && t.TR.D
                   && t.BL.D && t.BR.D;
        },
    },
    PatternMatcher{
        .m_type       = BorderType::Special_SSSS,
        .m_class      = BorderClass::Special,
        .m_doFlipHor  = false,
        .m_doFlipVert = false,
        .m_useOnDirt  = true,
        .m_f          = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.S && t.TR.S
                   && t.BL.S && t.BR.S;
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
    auto& pp = m_terrainId->presentationParams;

    if (pp.specialBorderTilesOffset < 0)
        return setViewCenter();

    m_tileOffset = pp.specialBorderTilesOffset + pp.borderSpecialOffsets.at(borderType);
    m_tileCount  = pp.borderSpecialCounts.at(borderType);
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setViewBorderMixed(Core::LibraryTerrain::BorderType borderType)
{
    auto& pp = m_terrainId->presentationParams;

    if (pp.mixedBorderTilesOffset < 0)
        return setViewCenter();

    m_tileOffset = pp.mixedBorderTilesOffset + pp.borderMixedOffsets.at(borderType);
    m_tileCount  = pp.borderMixedCounts.at(borderType);
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setViewBorderSandOrDirt(Core::LibraryTerrain::BorderType borderType, bool sandBorder)
{
    auto& pp = m_terrainId->presentationParams;

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
    if (m_terrainId->presentationParams.hasRotations) {
        if (borderType == BorderType::L)
            m_tileOffset += m_tileCount * m_terrainView.m_recommendedFlipHor;
        else if (borderType == BorderType::T)
            m_tileOffset += m_tileCount * m_terrainView.m_recommendedFlipVert;
        else
            m_tileOffset += m_tileCount * 2 * m_terrainView.m_recommendedFlipVert + m_tileCount * m_terrainView.m_recommendedFlipHor;

        m_terrainView.m_recommendedFlipHor  = false;
        m_terrainView.m_recommendedFlipVert = false;
    }
    updateMinMax();
    return true;
}

bool FHTileMap::Tile::setViewCenter()
{
    if (m_terrainId) {
        auto& pp         = m_terrainId->presentationParams;
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
    m_terrainView.m_viewMin = static_cast<uint8_t>(m_tileOffset);
    const int clearCount    = m_tileCountClear > 0 ? m_tileCountClear : m_tileCount;
    m_terrainView.m_viewMid = static_cast<uint8_t>(m_terrainView.m_viewMin + clearCount - 1);
    m_terrainView.m_viewMax = static_cast<uint8_t>(m_terrainView.m_viewMin + m_tileCount - 1);
}

void FHTileMap::determineTerrainViewRotation(Core::LibraryTerrainConstPtr dirtTerrain,
                                             Core::LibraryTerrainConstPtr sandTerrain,
                                             Core::LibraryTerrainConstPtr waterTerrain)
{
    //std::ofstream ofs("E:/debug.txt");
    //int           row = 0;

    eachPosTile([](const FHPos& pos, FHTileMap::Tile& XX, size_t) {
        XX.TL = SubtileType::Invalid;
        XX.TR = SubtileType::Invalid;
        XX.BL = SubtileType::Invalid;
        XX.BR = SubtileType::Invalid;
    });

    eachPosTile([this,
                 dirtTerrain,
                 sandTerrain](const FHPos& pos, FHTileMap::Tile& XX, size_t) {
        auto makest = [this,
                       dirtTerrain,
                       sandTerrain](const FHPos&     pos,
                                    FHTileMap::Tile& tileXX,
                                    SubtileType&     sub,
                                    int              dx,
                                    int              dy) {
            // fo sand, it contains only 4 native 'sand' subtiles.
            if (tileXX.m_terrainId == sandTerrain) {
                sub = SubtileType::Native;
                return;
            }

            const FHTileMap::Tile& tileDX  = getNeighbour(pos, dx, 0);
            const FHTileMap::Tile& tileDY  = getNeighbour(pos, 0, dy);
            const FHTileMap::Tile& tileDXY = getNeighbour(pos, dx, dy);

            const bool eqDX  = tileDX.m_terrainId == tileXX.m_terrainId;
            const bool eqDY  = tileDY.m_terrainId == tileXX.m_terrainId;
            const bool eqDXY = tileDXY.m_terrainId == tileXX.m_terrainId;

            const bool allEq = eqDX && eqDY && eqDXY;
            if (allEq) {
                sub = SubtileType::Native;
                return;
            }

            const bool sandDX  = tileDX.m_terrainId->tileBase == Core::LibraryTerrain::TileBase::Sand;
            const bool sandDY  = tileDY.m_terrainId->tileBase == Core::LibraryTerrain::TileBase::Sand;
            const bool sandDXY = tileDXY.m_terrainId->tileBase == Core::LibraryTerrain::TileBase::Sand;

            const bool anySand = sandDX || sandDY || sandDXY;
            if (anySand) {
                sub = SubtileType::Sand;
                return;
            }
            if (tileXX.m_terrainId->tileBase == Core::LibraryTerrain::TileBase::Sand) {
                sub = SubtileType::Sand;
                return;
            }

            // dirt can contain mix of native 'dirt' and sand tiles.
            if (tileXX.m_terrainId == dirtTerrain) {
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

    eachPosTile([](const FHPos& pos, FHTileMap::Tile& XX, size_t) {
        assert(XX.TL != SubtileType::Invalid);
        assert(XX.TR != SubtileType::Invalid);
        assert(XX.BL != SubtileType::Invalid);
        assert(XX.BR != SubtileType::Invalid);
    });

    eachPosTile([this,
                 dirtTerrain,
                 sandTerrain,
                 //&ofs,
                 //&row,
                 waterTerrain](const FHPos& pos, FHTileMap::Tile& XX, size_t) {
        TileNeightbours tilen;
        tilen.read(dirtTerrain,
                   sandTerrain,
                   waterTerrain,
                   *this,
                   XX,
                   pos);
        XX.m_coastal = tilen.m_coastal;

        /*
        if (1) {
            if (row != pos.m_y)
                ofs << '\n';

            row = pos.m_y;
            ofs << tilen.debugString() << '\t';
        }*/

        if (XX.m_terrainId == sandTerrain) {
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
                    if (XX.m_terrainId == dirtTerrain && !matcher.m_useOnDirt) {
                        continue;
                    }

                    if (matcher.m_f(t, false)) {
                        XX.m_terrainView.m_recommendedFlipHor  = t.m_flippedHor;
                        XX.m_terrainView.m_recommendedFlipVert = t.m_flippedVert;
                        XX.setView(matcher.m_class, matcher.m_type);
                        matchedBts.push_back(static_cast<int>(matcher.m_type));
                    }
                    if (matcher.m_class == BorderClass::NormalDirt && matcher.m_f(t, true)) {
                        XX.m_terrainView.m_recommendedFlipHor  = t.m_flippedHor;
                        XX.m_terrainView.m_recommendedFlipVert = t.m_flippedVert;
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

void FHTileMap::determineRoadViewRotation()
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

        const auto eR  = (X.m_roadType == FHRoadType::None) == (R.m_roadType == FHRoadType::None);
        const auto eL  = (X.m_roadType == FHRoadType::None) == (L.m_roadType == FHRoadType::None);
        const auto eT  = (X.m_roadType == FHRoadType::None) == (T.m_roadType == FHRoadType::None);
        const auto eB  = (X.m_roadType == FHRoadType::None) == (B.m_roadType == FHRoadType::None);
        const auto eTR = (X.m_roadType == FHRoadType::None) == (TR.m_roadType == FHRoadType::None);
        const auto eBL = (X.m_roadType == FHRoadType::None) == (BL.m_roadType == FHRoadType::None);

        auto setView = [&X, flipHor, flipVert](uint8_t min, uint8_t max) {
            X.m_roadView.m_viewMin             = min;
            X.m_roadView.m_viewMax             = max;
            X.m_roadView.m_recommendedFlipHor  = flipHor;
            X.m_roadView.m_recommendedFlipVert = flipVert;
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
            X.m_roadView.m_recommendedFlipHor  = false;
            X.m_roadView.m_recommendedFlipVert = true;
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
                if (X.m_roadType == FHRoadType::None)
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

                X.m_roadView.m_viewMin             = 9;
                X.m_roadView.m_viewMax             = 10;
                X.m_roadView.m_recommendedFlipHor  = false;
                X.m_roadView.m_recommendedFlipVert = false;
            }
        }
    }
}

void FHTileMap::determineRiverViewRotation()
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
            X.m_riverView.m_viewMin             = min;
            X.m_riverView.m_viewMax             = max;
            X.m_riverView.m_recommendedFlipHor  = flipHor;
            X.m_riverView.m_recommendedFlipVert = flipVert;
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
                if (X.m_riverType == FHRiverType::None)
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

                X.m_riverView.m_viewMin             = 9;
                X.m_riverView.m_viewMax             = 10;
                X.m_riverView.m_recommendedFlipHor  = false;
                X.m_riverView.m_recommendedFlipVert = false;
            }
        }
    }
}

void FHTileMap::determineViewRotation(const Core::IGameDatabase* database)
{
    const auto* dirtTerrain  = database->terrains()->find(std::string(Core::LibraryTerrain::s_terrainDirt));
    const auto* sandTerrain  = database->terrains()->find(std::string(Core::LibraryTerrain::s_terrainSand));
    const auto* waterTerrain = database->terrains()->find(std::string(Core::LibraryTerrain::s_terrainWater));

    bool valid = true;
    this->eachPosTile([&valid](const FHPos& pos, const FHTileMap::Tile& tile, size_t) {
        valid = valid && tile.m_terrainId;
        if (!tile.m_terrainId)
            Logger(Logger::Err) << "No terrain at: " << pos.toPrintableString();
    });
    if (!valid)
        throw std::runtime_error("some terrain tiles are missing");

    determineTerrainViewRotation(dirtTerrain, sandTerrain, waterTerrain);
    determineRoadViewRotation();
    determineRiverViewRotation();
}

void FHTileMap::makeRecommendedRotation()
{
    this->eachPosTile([](const FHPos&, FHTileMap::Tile& tile, size_t) {
        tile.m_terrainView.makeRecommendedRotation();
        tile.m_roadView.makeRecommendedRotation();
        tile.m_riverView.makeRecommendedRotation();
    });
}

void FHTileMap::makeRngView(Core::IRandomGenerator* rng, int roughTileChancePercent)
{
    this->eachPosTile([rng, roughTileChancePercent](const FHPos&, FHTileMap::Tile& tile, size_t) {
        tile.m_terrainView.makeRngView(rng, roughTileChancePercent, true);
        tile.m_roadView.makeRngView(rng, roughTileChancePercent, false);
        tile.m_riverView.makeRngView(rng, roughTileChancePercent, false);
    });
}

void FHPackedTileMap::unpackToMap(FHTileMap& map) const
{
    if (m_tileTerrianIndexes.empty())
        return;
    map.eachPosTile([this](const FHPos& pos, FHTileMap::Tile& tile, size_t i) {
        tile.m_terrainId              = m_terrains[m_tileTerrianIndexes[i]];
        tile.m_terrainView.m_view     = m_tileViews[i];
        tile.m_terrainView.m_flipHor  = m_terrainFlipHor[i];
        tile.m_terrainView.m_flipVert = m_terrainFlipVert[i];
        tile.m_coastal                = m_coastal[i];
    });
    for (auto& road : m_roads) {
        for (size_t i = 0; i < road.m_tiles.size(); ++i) {
            auto& tile                 = map.get(road.m_tiles[i]);
            tile.m_roadType            = road.m_type;
            tile.m_roadView.m_view     = road.m_views[i];
            tile.m_roadView.m_flipHor  = road.m_flipHor[i];
            tile.m_roadView.m_flipVert = road.m_flipVert[i];
        }
    }
    for (auto& river : m_rivers) {
        for (size_t i = 0; i < river.m_tiles.size(); ++i) {
            auto& tile                  = map.get(river.m_tiles[i]);
            tile.m_riverType            = river.m_type;
            tile.m_riverView.m_view     = river.m_views[i];
            tile.m_riverView.m_flipHor  = river.m_flipHor[i];
            tile.m_riverView.m_flipVert = river.m_flipVert[i];
        }
    }
}

void FHPackedTileMap::packFromMap(const FHTileMap& map)
{
    size_t tilesSize = map.totalSize();
    m_tileTerrianIndexes.resize(tilesSize);
    m_tileViews.resize(tilesSize);
    m_terrainFlipHor.resize(tilesSize);
    m_terrainFlipVert.resize(tilesSize);
    m_coastal.resize(tilesSize);
    m_rivers.resize(4);
    m_roads.resize(3);
    m_roads[0].m_type = FHRoadType::Dirt;
    m_roads[1].m_type = FHRoadType::Gravel;
    m_roads[2].m_type = FHRoadType::Cobblestone;

    m_rivers[0].m_type = FHRiverType::Water;
    m_rivers[1].m_type = FHRiverType::Ice;
    m_rivers[2].m_type = FHRiverType::Mud;
    m_rivers[3].m_type = FHRiverType::Lava;

    auto makeTerrainIndex = [this](Core::LibraryTerrainConstPtr terrain) -> uint8_t {
        for (size_t i = 0; i < m_terrains.size(); ++i) {
            if (m_terrains[i] == terrain)
                return static_cast<uint8_t>(i);
        }
        m_terrains.push_back(terrain);
        return static_cast<uint8_t>(m_terrains.size() - 1);
    };
    map.eachPosTile([&makeTerrainIndex, this](const FHPos& pos, const FHTileMap::Tile& tile, size_t i) {
        m_tileTerrianIndexes[i] = makeTerrainIndex(tile.m_terrainId);
        m_tileViews[i]          = tile.m_terrainView.m_view;
        m_terrainFlipHor[i]     = tile.m_terrainView.m_flipHor;
        m_terrainFlipVert[i]    = tile.m_terrainView.m_flipVert;
        m_coastal[i]            = tile.m_coastal;
        if (tile.m_roadType != FHRoadType::None) {
            auto& road = m_roads[static_cast<size_t>(tile.m_roadType) - 1];
            road.m_tiles.push_back(pos);
            road.m_views.push_back(tile.m_roadView.m_view);
            road.m_flipHor.push_back(tile.m_roadView.m_flipHor);
            road.m_flipVert.push_back(tile.m_roadView.m_flipVert);
        }
        if (tile.m_riverType != FHRiverType::None) {
            auto& river = m_rivers[static_cast<size_t>(tile.m_riverType) - 1];
            river.m_tiles.push_back(pos);
            river.m_views.push_back(tile.m_riverView.m_view);
            river.m_flipHor.push_back(tile.m_riverView.m_flipHor);
            river.m_flipVert.push_back(tile.m_riverView.m_flipVert);
        }
    });
}

void FHTileMap::TileView::makeRngView(Core::IRandomGenerator* rng, int roughTileChancePercent, bool useMid)
{
    if (m_viewMin == m_viewMax) {
        m_view = m_viewMin;
        return;
    }
    auto rngView = [&rng](uint8_t min, uint8_t max) -> uint8_t {
        if (min == max)
            return min;
        uint8_t diff   = max - min;
        uint8_t result = rng->genSmall(diff);
        return min + result;
    };
    if (useMid && m_viewMid != m_viewMax) {
        if (rng->genSmall(100) >= roughTileChancePercent) {
            m_view = rngView(m_viewMin, m_viewMid);
            return;
        }
    }
    m_view = rngView(m_viewMin, m_viewMax);
}

}
