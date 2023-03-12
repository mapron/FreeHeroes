/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "MapScore.hpp"
#include "Reward.hpp"

#include "MapUtilExport.hpp"

namespace FreeHeroes {

MAPUTIL_EXPORT Core::MapScore estimateReward(const Core::Reward& reward, Core::ScoreAttr armyAttr = Core::ScoreAttr::ArmyAux);

MAPUTIL_EXPORT void estimateArtScore(Core::LibraryArtifactConstPtr art, Core::MapScore& score);

MAPUTIL_EXPORT void estimateSpellScore(Core::LibrarySpellConstPtr spell, Core::MapScore& score, bool asAnySpell);

MAPUTIL_EXPORT void estimateSpellListScore(const std::vector<Core::LibrarySpellConstPtr>& spells, Core::MapScore& score, bool asAnySpell);

}
