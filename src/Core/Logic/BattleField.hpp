/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CoreLogicExport.hpp"

#include "BattleFieldPosition.hpp"

#include "LibraryMapObject.hpp"

#include <map>
#include <optional>
#include <functional>
#include <algorithm>
#include <array>

#include <cstdint>

namespace FreeHeroes::Core {

CORELOGIC_EXPORT BattleDirectionPrecise directionMirrorHor(BattleDirectionPrecise direction);

CORELOGIC_EXPORT BattleAttackDirection attackDirectionInverse(BattleAttackDirection direction);

struct CORELOGIC_EXPORT BattleFieldGeometry {
    int width  = std::numeric_limits<int>::min();
    int height = std::numeric_limits<int>::min();

    using AdjacentMap = std::map<BattleDirection, BattlePosition>;

    bool isValid(const BattlePosition pos) const noexcept
    {
        return !pos.isEmpty() && pos.x >= 0 && pos.y >= 0 && pos.x < width && pos.y < height;
    }

    BattlePosition         neighbour(const BattlePosition pos, BattleDirection direction) const noexcept;
    BattlePositionSet      validNeighbours(const BattlePosition pos, const std::vector<BattleDirection>& directions) const noexcept;
    BattlePositionExtended suggestPositionForAttack(BattlePositionExtended startPos, const BattlePositionExtended target, const BattlePositionExtended::Sub targetInner, BattleAttackDirection direction) const noexcept;

    AdjacentMap       getAdjacent(const BattlePosition pos) const;
    BattlePositionSet getAdjacentSet(const BattlePosition pos) const;
    BattlePositionSet getAdjacentSet(const BattlePositionExtended pos) const;
    BattlePositionSet getAllPositions() const;
    BattlePositionSet getFloodFillFrom(const BattlePosition pos, int iterations) const;
    BattlePositionSet closestTo(const BattlePosition pos, const BattlePositionSet& candidates) const;
};

struct CORELOGIC_EXPORT BattleFieldPreset {
    std::vector<BattlePosition> obstacles;
    BattleFieldGeometry         field;
    FieldLayout                 layout;

    BattlePosition calcPosition(bool attacker, int orderIndex, int totalCount, bool compactPositioning) const;
};

}
