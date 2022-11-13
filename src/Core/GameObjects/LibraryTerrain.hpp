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

namespace FreeHeroes::Core {

struct LibraryTerrain {
    struct Presentation {
        int                      order = 0;
        std::string              music;
        std::vector<std::string> backgroundsBattle;
        std::string              icon;
        //std::string tileset;
        int                      centerTilesOffset = 0;
        std::vector<std::string> centerTiles;
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
