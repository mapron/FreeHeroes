/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <map>

namespace FreeHeroes::Core {

enum class ScoreAttr
{
    Invalid,

    Army,
    ArmyDwelling,
    ArtStat,
    ArtSupport,
    Gold,
    Resource,
    ResourceGen,
    Experience,
    Control,
    Upgrade,
    SpellOffensive,
    SpellCommon,
    SpellAny,
    Support,
};
using MapScore = std::map<ScoreAttr, int64_t>;

}
