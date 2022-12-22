/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "Stat.hpp"

#include <string>
#include <vector>
#include <map>

namespace FreeHeroes::Core {

struct LibraryTerrain {
    [[maybe_unused]] static inline constexpr const std::string_view s_terrainDirt  = "sod.terrain.dirt";
    [[maybe_unused]] static inline constexpr const std::string_view s_terrainSand  = "sod.terrain.sand";
    [[maybe_unused]] static inline constexpr const std::string_view s_terrainWater = "sod.terrain.water";

    enum TileBase
    {
        None,
        Sand,
        Dirt,
    };

    enum class BorderType
    {
        Invalid,
        TL = 10,
        L,
        T,
        BR,
        TLS,
        BRS,
        Mixed_DNND = 20,
        Mixed_DNNS,
        Mixed_SNNS,
        Mixed_NDSD,
        Mixed_NSDD,
        Mixed_NDNS,
        Mixed_NNDS,
        Mixed_NSDS,
        Mixed_NDSS,
        Center       = 49,
        Special_DDDD = 73,
        Special_SSSS,
        Special_DDDS,
        Special_SSSD,
        Special_NSSD,
        Special_NDDS,
    };

    enum class BorderClass
    {
        Invalid,
        NormalDirt,
        NormalSand,
        Mixed,
        Center,
        Special,
    };

    struct Presentation {
        int                      order = 0;
        std::string              music;
        std::vector<std::string> backgroundsBattle;
        std::string              defFile;
        bool                     defFileSplit = false; // HotA terrain like wstlt077
        bool                     isAnimated   = false;

        int dirtBorderTilesOffset    = -1;
        int sandBorderTilesOffset    = -1;
        int mixedBorderTilesOffset   = -1;
        int specialBorderTilesOffset = -1;
        int centerTilesOffset        = -1;
        int centerTilesCount         = -1;
        int centerTilesClearCount    = -1; // tiles withour rough parts.

        using BTMap = std::map<BorderType, int>;
        BTMap borderCounts{
            { BorderType::TL, 4 },
            { BorderType::L, 4 },
            { BorderType::T, 4 },
            { BorderType::BR, 4 },
            { BorderType::TLS, 2 },
            { BorderType::BRS, 2 },
        };
        BTMap borderOffsets;

        BTMap borderMixedCounts{
            { BorderType::Mixed_DNND, 1 },
            { BorderType::Mixed_DNNS, 1 },
            { BorderType::Mixed_SNNS, 1 },
            { BorderType::Mixed_NDSD, 1 },
            { BorderType::Mixed_NSDD, 1 },
            { BorderType::Mixed_NDNS, 1 },
            { BorderType::Mixed_NNDS, 1 },
            { BorderType::Mixed_NSDS, 1 },
            { BorderType::Mixed_NDSS, 1 },
        };
        BTMap borderMixedOffsets;

        BTMap borderSpecialCounts{
            { BorderType::Special_DDDD, 1 },
            { BorderType::Special_SSSS, 1 },
            { BorderType::Special_DDDS, 1 },
            { BorderType::Special_SSSD, 1 },
            { BorderType::Special_NSSD, 1 },
            { BorderType::Special_NDDS, 1 },
        };
        BTMap borderSpecialOffsets;

        bool hasRotations = false;
    };
    struct Bonus {
        RngChanceMultiplier rngMult;
    };

    std::string id;
    std::string untranslatedName;
    int         legacyId   = -1;
    int         moveCost   = 100;
    bool        isObstacle = false;
    bool        extraLayer = false;
    Bonus       bonusAll;

    TileBase     tileBase = TileBase::Dirt;
    Presentation presentationParams;
};

}
