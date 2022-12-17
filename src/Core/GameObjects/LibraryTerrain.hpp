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

    enum class BorderType
    {
        Invalid,
        TL,
        L,
        T,
        BR,
        TLS,
        BRS,
        ThreeWay_DD,      // 0 - D\D
        ThreeWay_DS,      // 1 - D\S
        ThreeWay_SS,      // 2 - S\S
        ThreeWay_RD_BLS,  // 3 - right - D, BL - S
        ThreeWay_BD_TRS,  // 4 - bottom - D, TR - S
        ThreeWay_TRD_BRS, // 5 - TR - D, BR - S
        ThreeWay_BRS_BLD, // 6 - BR - S, BL - D
        ThreeWay_RS_BD,   // 7 - R - S, B - D
        ThreeWay_BS_RD,   // 8 - B - S, R - D
        Center,
    };

    struct Presentation {
        int                      order = 0;
        std::string              music;
        std::vector<std::string> backgroundsBattle;
        std::string              defFile;
        bool                     defFileSplit = false; // HotA terrain like wstlt077
        bool                     isAnimated   = false;

        int dirtBorderTilesOffset     = -1;
        int sandBorderTilesOffset     = -1;
        int sandDirtBorderTilesOffset = -1;
        int centerTilesOffset         = -1;
        int centerTilesCount          = -1;

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

        BTMap borderThreeWayCounts{
            { BorderType::ThreeWay_DD, 1 },
            { BorderType::ThreeWay_DS, 1 },
            { BorderType::ThreeWay_SS, 1 },
            { BorderType::ThreeWay_RD_BLS, 1 },
            { BorderType::ThreeWay_BD_TRS, 1 },
            { BorderType::ThreeWay_TRD_BRS, 1 },
            { BorderType::ThreeWay_BRS_BLD, 1 },
            { BorderType::ThreeWay_RS_BD, 1 },
            { BorderType::ThreeWay_BS_RD, 1 },
        };
        BTMap borderThreeWayOffsets;
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

    Presentation presentationParams;
};

}
