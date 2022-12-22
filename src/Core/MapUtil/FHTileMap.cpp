/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTileMap.hpp"

#include "IRandomGenerator.hpp"

#include "Logger.hpp"

#include <array>
#include <functional>
#include <fstream>
#include <cassert>

namespace FreeHeroes {

namespace {

using BorderType = Core::LibraryTerrain::BorderType;

struct TileInfo {
    Core::LibraryTerrainConstPtr m_t = nullptr;
    struct {
        bool di = false; // is Dirt
        bool sa = false; // is Sand
        bool wa = false; // is Water
    } info;              // tile exact terrain info

    struct {
        bool di = false; // base is dirt
        bool sa = false; // base is sand
    } base;              // base  info

    struct {
        struct {
            bool equ = false; // terrain is equal
            bool neq = false;
        } i; // margin terrain difference
        struct {
            bool equ = false; // base is equal
            bool neq = false;
        } b; // margin base (sand/dirt)

        bool dirt_ = false; // dirt border
        bool sand_ = false; // sand border
    } m;                    // margin info;

    bool OOB = false;

    bool diff(bool s) const { return s ? m.sand_ : m.dirt_; }

    bool isba(bool s) const { return s ? base.sa : base.di; }

    //bool id = true;  // identical
    //    bool eq = true;  // somewhat equal
    //    bool di = false; // dirt border
    //    bool sa = false; // sand border

    void read(
        const FHTileMap&             map,
        const FHPos&                 pos,
        Core::LibraryTerrainConstPtr dirtTerrain,
        Core::LibraryTerrainConstPtr sandTerrain,
        Core::LibraryTerrainConstPtr waterTerrain,
        const FHTileMap::Tile&       tile)
    {
        m_t     = tile.m_terrain;
        info.sa = m_t == sandTerrain;
        info.di = m_t == dirtTerrain;
        info.wa = m_t == waterTerrain;

        base.di = m_t->tileBase == Core::LibraryTerrain::TileBase::Dirt;
        base.sa = m_t->tileBase == Core::LibraryTerrain::TileBase::Sand;

        assert(base.di || base.sa);
    }

    void read(
        const FHTileMap&             map,
        const FHPos&                 pos,
        Core::LibraryTerrainConstPtr dirtTerrain,
        Core::LibraryTerrainConstPtr sandTerrain,
        Core::LibraryTerrainConstPtr waterTerrain,
        int                          dx,
        int                          dy)
    {
        const FHTileMap::Tile& tile = map.getNeighbour(pos, dx, dy);

        OOB = !map.inBounds(pos.m_x + dx, pos.m_y + dy);

        read(map, pos, dirtTerrain, sandTerrain, waterTerrain, tile);
    }

    void marginFromTile(const TileInfo& XX)
    {
        m.i.equ = m_t == XX.m_t;
        m.i.neq = !m.i.equ;

        m.b.equ = base.di == XX.base.di;
        m.b.neq = !m.b.equ;

        if (m.i.equ)
            return;

        if (XX.info.sa)
            return;

        if (XX.info.di) {
            m.sand_ = m.b.neq;
            return;
        }
        if (XX.base.sa || base.sa) {
            m.sand_ = true;
            return;
        }

        m.dirt_ = true;
    }

    char debugChar() const
    {
        //if (id)
        //    return '!';
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
    TileInfo TL,
        TC,
        TR,
        CL,
        XX,
        CR,
        BL,
        BC,
        BR;

    TileInfo T2, L2, R2, B2;

    bool m_flippedHor  = false;
    bool m_flippedVert = false;

    std::string toDebugString() const
    {
        return std::string()
               + TL.debugChar() + TC.debugChar() + TR.debugChar() + ' '
               + L2.debugChar() + CL.debugChar() + '_' + CR.debugChar() + R2.debugChar() + ' '
               + BL.debugChar() + BC.debugChar() + BR.debugChar();
    }

    void read(Core::LibraryTerrainConstPtr dirtTerrain,
              Core::LibraryTerrainConstPtr sandTerrain,
              Core::LibraryTerrainConstPtr waterTerrain,
              const FHTileMap&             map,
              const FHTileMap::Tile&       tileXX,
              const FHPos&                 pos)
    {
        TL.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, -1, -1);
        TC.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +0, -1);
        TR.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +1, -1);
        CL.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, -1, +0);
        XX.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, tileXX);
        CR.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +1, +0);
        BL.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, -1, +1);
        BC.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +0, +1);
        BR.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +1, +1);

        T2.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +0, -2);
        L2.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, -2, +0);
        R2.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +2, +0);
        B2.read(map, pos, dirtTerrain, sandTerrain, waterTerrain, +0, +2);

        TL.marginFromTile(XX);
        TC.marginFromTile(XX);
        TR.marginFromTile(XX);
        CL.marginFromTile(XX);
        XX.marginFromTile(XX);
        CR.marginFromTile(XX);
        BL.marginFromTile(XX);
        BC.marginFromTile(XX);
        BR.marginFromTile(XX);

        T2.marginFromTile(XX);
        L2.marginFromTile(XX);
        R2.marginFromTile(XX);
        B2.marginFromTile(XX);
    }

    bool isCoastal() const
    {
        if (XX.info.wa)
            return false;

        return false
               || TL.info.wa
               || TC.info.wa
               || TR.info.wa
               || CL.info.wa
               || CR.info.wa
               || BL.info.wa
               || BC.info.wa
               || BR.info.wa;
    }

    TileNeightbours flipped(bool vertical, bool horizontal)
    {
        if (!vertical && !horizontal)
            return *this;
        if (vertical && horizontal)
            return TileNeightbours{
                .TL            = this->BR,
                .TC            = this->BC,
                .TR            = this->BL,
                .CL            = this->CR,
                .XX            = this->XX,
                .CR            = this->CL,
                .BL            = this->TR,
                .BC            = this->TC,
                .BR            = this->TL,
                .T2            = this->B2,
                .L2            = this->R2,
                .R2            = this->L2,
                .B2            = this->T2,
                .m_flippedHor  = !this->m_flippedHor,
                .m_flippedVert = !this->m_flippedVert,
            };
        if (vertical)
            return TileNeightbours{
                .TL            = this->BL,
                .TC            = this->BC,
                .TR            = this->BR,
                .CL            = this->CL,
                .XX            = this->XX,
                .CR            = this->CR,
                .BL            = this->TL,
                .BC            = this->TC,
                .BR            = this->TR,
                .T2            = this->B2,
                .L2            = this->L2,
                .R2            = this->R2,
                .B2            = this->T2,
                .m_flippedHor  = false,
                .m_flippedVert = !this->m_flippedVert,
            };
        if (horizontal)
            return TileNeightbours{
                .TL            = this->TR,
                .TC            = this->TC,
                .TR            = this->TL,
                .CL            = this->CR,
                .XX            = this->XX,
                .CR            = this->CL,
                .BL            = this->BR,
                .BC            = this->BC,
                .BR            = this->BL,
                .T2            = this->T2,
                .L2            = this->R2,
                .R2            = this->L2,
                .B2            = this->B2,
                .m_flippedHor  = !this->m_flippedHor,
                .m_flippedVert = false,
            };
        return {};
    }
};

struct PatternMatcher {
    BorderType m_type = BorderType::Invalid;

    bool m_doFlipHor  = true;
    bool m_doFlipVert = true;

    bool m_mixed  = false;
    bool m_center = false;

    bool m_useOnDirt = true;

    std::function<bool(const TileNeightbours& t, bool sand)> m_f;
};

static constexpr const bool I__________I = true;

const std::vector<PatternMatcher> g_matchers{
    PatternMatcher{
        .m_type       = BorderType::Center,
        .m_doFlipHor  = false,
        .m_doFlipVert = false,
        .m_center     = true,
        .m_f          = [](const TileNeightbours& t, bool) {
            return (true
                    && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.i.equ
                    && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ
                    && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.m.i.equ)
                   || (true
                       && t.TL.base.di && t.TC.base.di && t.TR.base.di
                       && t.CL.base.di && t.XX.info.di && t.CR.base.di
                       && t.BL.base.di && t.BC.base.di && t.BR.base.di);
        } },

    // clang-format off
    PatternMatcher{
        .m_type       = BorderType::ThreeWay_DD,
        .m_doFlipVert = false,
        .m_mixed      = true,
        .m_useOnDirt  = false,
        .m_f          = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.m.dirt_ && t.TC.m.i.equ && t.TR.m.i.equ
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ
                   && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.m.dirt_;
        } 
    },
    PatternMatcher{
        .m_type       = BorderType::ThreeWay_DS,
        .m_mixed      = true,
        .m_useOnDirt  = false,
        .m_f          = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.m.dirt_ && t.TC.m.i.equ && t.TR.m.i.equ
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ
                   && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.m.sand_;
        } 
    },
    // clang-format on
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_SS,
        .m_doFlipHor = false,
        .m_mixed     = true,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.m.sand_ && t.TC.m.i.equ && t.TR.m.i.equ
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ
                   && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.m.sand_;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_RD_BLS,
        .m_mixed     = true,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            const bool precond = t.BC.m.dirt_ || t.BC.m.i.equ;
            return precond
                   && t.TL.m.i.equ && t.TC.m.i.equ && t.TL.base.di
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.dirt_
                   && t.BL.m.sand_ && I__________I && I__________I;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_BD_TRS,
        .m_mixed     = true,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            const bool precond = t.CR.m.dirt_ || t.CR.m.i.equ;
            return precond
                   && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.sand_
                   && t.CL.m.i.equ && t.XX.m.i.equ && I__________I
                   && t.BL.base.di && t.BC.m.dirt_ && I__________I;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_TRD_BRS,
        .m_mixed     = true,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.dirt_
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.dirt_
                   && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.m.sand_;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_BRS_BLD,
        .m_mixed     = true,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            return true
                   && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.i.equ
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ
                   && t.BL.m.dirt_ && t.BC.m.dirt_ && t.BR.m.sand_;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_RS_BD,
        .m_mixed     = true,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            const bool precond = t.BC.m.dirt_ || (t.BC.m.i.equ);
            return precond
                   && t.TL.m.i.equ && t.TC.m.i.equ && I__________I
                   && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.sand_
                   && t.BL.m.dirt_ && I__________I && I__________I;
        },
    },
    PatternMatcher{
        .m_type      = BorderType::ThreeWay_BS_RD,
        .m_mixed     = true,
        .m_useOnDirt = false,
        .m_f         = [](const TileNeightbours& t, bool) {
            const bool precond = t.CR.m.dirt_ || (t.CR.m.i.equ);
            return precond
                   && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.dirt_
                   && t.CL.m.i.equ && t.XX.m.i.equ && I__________I
                   && I__________I && t.BC.m.sand_ && I__________I;
        },
    },

    PatternMatcher{
        .m_type = BorderType::TL,
        .m_f    = [](const TileNeightbours& t, bool s) {
            const bool sandCond      = s && (t.TR.diff(s) || t.TR.m.dirt_) && (t.BL.diff(s) || t.BL.m.dirt_);
            const bool irregularCond = (t.CL.diff(s) && t.TC.m.i.equ) || (t.TC.diff(s) && t.CL.m.i.equ);
            return (true
                    && t.TL.diff(s) && t.TC.diff(s) && t.TR.diff(s)
                    && t.CL.diff(s) && t.XX.m.i.equ && t.CR.m.i.equ
                    && t.BL.diff(s) && t.BC.m.i.equ && t.BR.m.i.equ)
                   || (irregularCond
                       && t.TL.m.i.equ && I__________I && t.TR.diff(s)
                       && I__________I && t.XX.m.i.equ && t.CR.m.i.equ
                       && t.BL.diff(s) && t.BC.m.i.equ && t.BR.m.i.equ)
                   || (sandCond
                       && I__________I && t.TC.diff(s) && I__________I
                       && t.CL.diff(s) && t.XX.m.i.equ && t.CR.m.i.equ
                       && I__________I && t.BC.m.i.equ && t.BR.m.i.equ);
        },
    },
    PatternMatcher{
        .m_type = BorderType::TLS,
        .m_f    = [](const TileNeightbours& t, bool s) {
            bool precond = false;
            if (t.XX.base.sa)
                precond = t.TR.m.i.equ || t.BL.m.i.equ;
            else if (s)
                precond = t.TR.m.i.equ || t.BL.m.i.equ;
            else
                precond = (t.TR.m.i.equ && t.BL.m.sand_ == s) || (t.BL.m.i.equ && t.TR.m.sand_ == s);

            return (precond)
                   && (true
                       && t.TL.diff(s) && t.TC.diff(s) && I__________I
                       && t.CL.diff(s) && t.XX.m.i.equ && t.CR.m.i.equ
                       && I__________I && t.BC.m.i.equ && t.BR.m.i.equ);
        },
    },

    PatternMatcher{
        .m_type       = BorderType::L,
        .m_doFlipVert = false,
        .m_f          = [](const TileNeightbours& t, bool s) {
            const bool precond = s || (t.TL.base.sa == t.CL.base.sa && t.BL.base.sa == t.CL.base.sa);
            return (precond
                    && I__________I && t.TC.m.i.equ && t.TR.m.i.equ
                    && t.CL.diff(s) && t.XX.m.i.equ && t.CR.m.i.equ
                    && I__________I && t.BC.m.i.equ && t.BR.m.i.equ)
                   || (s
                       && I__________I && t.TC.base.di && t.TR.base.di
                       && t.CL.diff(s) && t.XX.info.di && t.CR.base.di
                       && I__________I && t.BC.base.di && t.BR.base.di);
        },
    },

    PatternMatcher{
        .m_type      = BorderType::T,
        .m_doFlipHor = false,
        .m_f         = [](const TileNeightbours& t, bool s) {
            const bool precond = s || (t.TL.base.sa == t.TC.base.sa && t.TR.base.sa == t.TC.base.sa);
            return (precond
                    && I__________I && t.TC.diff(s) && I__________I
                    && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ
                    && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.m.i.equ)
                   || (s
                       && I__________I && t.TC.diff(s) && I__________I
                       && t.CL.base.di && t.XX.info.di && t.CR.base.di
                       && t.BL.base.di && t.BC.base.di && t.BR.base.di);
        },
    },

    PatternMatcher{
        .m_type = BorderType::BR,
        .m_f    = [](const TileNeightbours& t, bool s) {
            return (true
                    && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.i.equ
                    && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ && t.R2.m.i.equ
                    && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.diff(s)
                    && I__________I && t.B2.m.i.equ)
                   || (s
                       && t.TL.base.di && t.TC.base.di && t.TR.base.di
                       && t.CL.base.di && t.XX.m.i.equ && t.CR.m.i.equ && t.R2.m.i.equ
                       && t.BL.base.di && t.BC.m.i.equ && t.BR.diff(s)
                       && I__________I && t.B2.m.i.equ);
        },
    },
    PatternMatcher{
        .m_type = BorderType::BRS,
        .m_f    = [](const TileNeightbours& t, bool s) {
            //if (1)
            //    return false;
            return (true
                    && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.i.equ
                    && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ && (t.R2.m.i.neq && !t.R2.OOB)
                    && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.diff(s)
                    && I__________I && I__________I)
                   || (true
                       && t.TL.m.i.equ && t.TC.m.i.equ && t.TR.m.i.equ
                       && t.CL.m.i.equ && t.XX.m.i.equ && t.CR.m.i.equ && I__________I
                       && t.BL.m.i.equ && t.BC.m.i.equ && t.BR.diff(s)
                       && I__________I && (t.B2.m.i.neq && !t.B2.OOB))
                   || (s
                       && t.TL.base.di && t.TC.base.di && t.TR.base.di
                       && t.CL.base.di && t.XX.info.di && t.CR.base.di && I__________I
                       && t.BL.base.di && t.BC.base.di && t.BR.diff(s)
                       && I__________I && (t.B2.m.i.neq && !t.B2.OOB))
                   || (s
                       && t.TL.base.di && t.TC.base.di && t.TR.base.di
                       && t.CL.base.di && t.XX.info.di && t.CR.base.di && (t.R2.m.i.neq && !t.R2.OOB)
                       && t.BL.base.di && t.BC.base.di && t.BR.diff(s)
                       && I__________I && I__________I);
        },
    },

};
}

bool FHTileMap::Tile::setViewBorderMixed(Core::LibraryTerrain::BorderType borderType)
{
    auto& pp = m_terrain->presentationParams;

    if (pp.sandDirtBorderTilesOffset < 0)
        return setViewCenter();

    m_tileOffset = pp.sandDirtBorderTilesOffset + pp.borderThreeWayOffsets.at(borderType);
    m_tileCount  = pp.borderThreeWayCounts.at(borderType);
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
    auto& pp         = m_terrain->presentationParams;
    m_tileOffset     = pp.centerTilesOffset;
    m_tileCount      = pp.centerTilesCount;
    m_tileCountClear = pp.centerTilesClearCount <= 0 ? m_tileCount : pp.centerTilesClearCount;
    updateMinMax();
    return true;
}

void FHTileMap::Tile::updateMinMax()
{
    m_viewMin = static_cast<uint8_t>(m_tileOffset);
    m_viewMid = static_cast<uint8_t>(m_viewMin + m_tileCountClear - 1);
    m_viewMax = static_cast<uint8_t>(m_viewMin + m_tileCount - 1);
}

void FHTileMap::correctTerrainTypes(Core::LibraryTerrainConstPtr dirtTerrain,
                                    Core::LibraryTerrainConstPtr sandTerrain,
                                    Core::LibraryTerrainConstPtr waterTerrain)
{
    //std::ofstream ofs("E:/debug.txt");
    for (int z = 0; z < m_depth; ++z) {
        //ofs << "\n\n";
        for (int y = 0; y < m_height; ++y) {
            //ofs << "\n";
            for (int x = 0; x < m_width; ++x) {
                const FHPos pos{ x, y, z };
                auto&       XX = get(pos);

                TileNeightbours tilen;
                tilen.read(dirtTerrain,
                           sandTerrain,
                           waterTerrain,
                           *this,
                           XX,
                           pos);
                //ofs << t.toDebugString() << '\t';

                XX.m_coastal = tilen.isCoastal();

                if (XX.m_terrain == sandTerrain) {
                    XX.setViewCenter();
                    continue;
                }

                std::array<TileNeightbours, 4> flipConfigs{
                    { tilen, tilen.flipped(true, false), tilen.flipped(false, true), tilen.flipped(true, true) }
                };

                const bool isCorrected = [&flipConfigs, &XX, x, y, z]() {
                    std::vector<int> matchedBts;
                    for (const TileNeightbours& t : flipConfigs) {
                        for (const auto& matcher : g_matchers) {
                            if (!matcher.m_doFlipHor && t.m_flippedHor)
                                continue;
                            if (!matcher.m_doFlipVert && t.m_flippedVert)
                                continue;
                            if (t.XX.info.di && !matcher.m_useOnDirt) {
                                continue;
                            }

                            if (matcher.m_f(t, false)) {
                                XX.m_flipHor  = t.m_flippedHor;
                                XX.m_flipVert = t.m_flippedVert;
                                if (matcher.m_center)
                                    XX.setViewCenter();
                                else if (matcher.m_mixed)
                                    XX.setViewBorderMixed(matcher.m_type);
                                else
                                    XX.setViewBorderSandOrDirt(matcher.m_type, false);
                                matchedBts.push_back(static_cast<int>(matcher.m_type));
                            }
                            if (!matcher.m_mixed && !matcher.m_center && matcher.m_f(t, true)) {
                                XX.m_flipHor  = t.m_flippedHor;
                                XX.m_flipVert = t.m_flippedVert;
                                XX.setViewBorderSandOrDirt(matcher.m_type, true);
                                matchedBts.push_back(static_cast<int>(matcher.m_type));
                            }
                        }
                    }
                    if (matchedBts.size() == 0) {
                        XX.setViewCenter();
                        Logger(Logger::Warning) << "failed to detect pattern at (" << x << ',' << y << ',' << int(z) << ")";
                        return false;
                    }
                    if (matchedBts.size() > 1) {
                        Logger(Logger::Warning) << "correctTerrainTypes: found [" << matchedBts.size() << "] patterns at (" << x << ',' << y << ',' << int(z) << "): " << matchedBts;
                    }

                    return true;
                }();
                if (!isCorrected)
                    continue;
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

    for (int z = 0; z < m_depth; ++z) {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                auto& X = get(x, y, z);
                if (!(X.m_view >= X.m_viewMin && X.m_view <= X.m_viewMax)) {
                    if (X.m_view != 0xffU)
                        Logger(Logger::Warning) << "Change view at (" << x << "," << y << "," << int(z) << ") " << int(X.m_view) << " -> [" << int(X.m_viewMin) << " .. " << int(X.m_viewMax) << "]";
                    X.m_view = rngView(X.m_viewMin, X.m_viewMax);
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
