/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Stat.hpp"
#include "LibraryFwd.hpp"

#include <string>
#include <map>
#include <vector>

namespace FreeHeroes::Core {

struct LibraryFactionHeroClass {
    using SkillWeights   = std::map<LibrarySecondarySkillConstPtr, int>;
    using PrimaryWeights = std::map<HeroPrimaryParamType, int>;
    struct Presentation {
        std::string battleSpriteMale;
        std::string battleSpriteFemale;
        std::string adventureSpriteMale;
        std::string adventureSpriteFemale;
    };

    std::string       id;
    std::string       untranslatedName;
    HeroPrimaryParams startParams;
    SkillWeights      secondarySkillWeights;
    PrimaryWeights    lowLevelIncrease;
    PrimaryWeights    highLevelIncrease;

    Presentation presentationParams;
};

struct LibraryFaction {
    enum class Alignment
    {
        Good,
        Evil,
        Neutral,
        Independent,
        Special
    };
    struct Presentation {
        enum class TownIndex
        {
            Village,
            Fort,
            Citadel,
            Castle,
            Capitol,
        };

        std::string unitBackground;
        std::string goesAfterId;

        std::vector<std::string> townAnimations;
    };

    std::string             id;
    std::string             untranslatedName;
    int                     legacyId       = -1;
    Alignment               alignment      = Alignment::Neutral;
    int                     generatedOrder = 0;
    LibraryFactionHeroClass warriorClass;
    LibraryFactionHeroClass mageClass;
    LibraryTerrainConstPtr  nativeTerrain = nullptr;

    std::vector<LibraryUnitConstPtr> units;
    std::vector<LibraryHeroConstPtr> heroes;

    LibraryObjectDefConstPtr mapObjectDef = nullptr;
    Presentation             presentationParams;
};

}
