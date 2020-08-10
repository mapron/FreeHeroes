/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleField.hpp"

#include <set>
#include <array>

#include <cassert>

namespace FreeHeroes::Core {

static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{0,0}) == 0);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{1,0}) == 1);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{0,1}) == 1);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{1,1}) == 1);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{2,1}) == 2);
static_assert (BattlePosition{2,1}.hexDistance(BattlePosition{0,0}) == 2);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{2,2}) == 3);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{2,0}) == 2);
static_assert (BattlePosition{0,0}.hexDistance(BattlePosition{4,3}) == 5);
static_assert (BattlePosition{1,2}.hexDistance(BattlePosition{3,1}) == 2);
static_assert (BattlePosition{1,2}.hexDistance(BattlePosition{3,2}) == 2);


BattleDirectionPrecise directionMirrorHor(BattleDirectionPrecise direction)
{
    if (direction == BattleDirectionPrecise::None)
        return BattleDirectionPrecise::None;
    if (direction == BattleDirectionPrecise::Top || direction == BattleDirectionPrecise::Bottom)
        return direction;
    // Luckily for this mirror we just inverse index in enum.

    const int index = static_cast<int>(direction);
    const int count = static_cast<int>(BattleDirectionPrecise::TopTopLeft) + 1;

    /// @todo: achtung, UB here with cast to enum class!
    return static_cast<BattleDirectionPrecise>(count - index);
}

BattlePosition BattleFieldGeometry::neighbour(const BattlePosition pos, BattleDirection direction) const noexcept
{
    const bool oddRow = pos.y % 2 == 1;
    const int evenRightStep = oddRow ? 0 : 1;
    const int oddLeftStep   = oddRow ?  -1 : 0;
    switch (direction) {
        case BattleDirection::TR: return pos + BattlePosition{evenRightStep, -1};
        case BattleDirection::R:  return pos + BattlePosition{1,              0};
        case BattleDirection::BR: return pos + BattlePosition{evenRightStep,  1};

        case BattleDirection::TL: return pos + BattlePosition{oddLeftStep, -1};
        case BattleDirection::L:  return pos + BattlePosition{-1,           0};
        case BattleDirection::BL: return pos + BattlePosition{oddLeftStep,  1};
        case BattleDirection::None:
            break;
    }
    assert(!"Unexistent direction");
    return {};
}

BattlePositionExtended BattleFieldGeometry::suggestPositionForAttack(BattlePositionExtended startPos, const BattlePositionExtended target, const BattlePositionExtended::Sub targetInner, BattleAttackDirection direction) const noexcept
{
    BattlePosition targetSpecific = target.specificPos(targetInner);
    switch (direction) {
     // if we attack to the right, we need to be at the left of our target. Seems logical?
        case BattleAttackDirection::R   : startPos.setRightPos( neighbour(targetSpecific , BattleDirection::L) ); break;
        case BattleAttackDirection::L   : startPos.setLeftPos ( neighbour(targetSpecific , BattleDirection::R) ); break;
        case BattleAttackDirection::TR  : startPos.setRightPos( neighbour(targetSpecific , BattleDirection::BL) ); break;
        case BattleAttackDirection::BR  : startPos.setRightPos( neighbour(targetSpecific , BattleDirection::TL) ); break;
        case BattleAttackDirection::TL  : startPos.setLeftPos ( neighbour(targetSpecific , BattleDirection::BR) ); break;
        case BattleAttackDirection::BL  : startPos.setLeftPos ( neighbour(targetSpecific , BattleDirection::TR) ); break;

        case BattleAttackDirection::T   : startPos.setLeftPos ( neighbour(targetSpecific , BattleDirection::BL) ); break;
        case BattleAttackDirection::B   : startPos.setLeftPos ( neighbour(targetSpecific , BattleDirection::TL) ); break;
        case BattleAttackDirection::None  : assert(!"Invalid direction"); break;
    }
    return startPos;
}

BattleFieldGeometry::AdjacentMap BattleFieldGeometry::getAdjacent(const BattlePosition pos) const
{
    AdjacentMap result;
    for (auto direction : {BattleDirection::TR, BattleDirection::R, BattleDirection::BR, BattleDirection::TL, BattleDirection::L, BattleDirection::BL}) {
        const BattlePosition neighbourPos = neighbour(pos, direction);
        if (isValid(neighbourPos))
            result[direction] = neighbourPos;
    }
    return result;
}

BattlePositionSet BattleFieldGeometry::getAdjacentSet(const BattlePosition pos) const
{
    auto mapping = getAdjacent(pos);
    BattlePositionSet result;
    for (const auto & item : mapping)
        result.insert(item.second);
    return result;
}

BattlePositionSet BattleFieldGeometry::getAdjacentSet(const BattlePositionExtended pos) const
{
    BattlePositionSet result = this->getAdjacentSet(pos.mainPos());
    if (pos.isLarge()) {
        auto extra = this->getAdjacentSet(pos.secondaryPos());
        for (auto && pos : extra)
            result.insert(pos);
        result.erase(pos.mainPos());
        result.erase(pos.secondaryPos());
    }
    return result;
}

BattlePositionSet BattleFieldGeometry::getAllPositions() const
{
    BattlePositionSet result;
    for (int w = 0; w < width; ++w) {
        for (int h = 0; h < height; ++h) {
            result.insert({w, h});
        }
    }
    return result;
}

BattlePositionSet BattleFieldGeometry::getFloodFillFrom(const BattlePosition pos, int iterations) const
{
    std::set<BattlePosition> result;
    std::set<BattlePosition> resultIter;
    std::set<BattlePosition> prevIter;
    result.insert(pos);
    prevIter = result;
    for (int i = 0; i < iterations; ++i) { // @todo: not optimal; a lot of search in inner area.
        resultIter.clear();
        for (const auto edgePos : prevIter) {
            BattlePositionSet nextEdgeTmp = getAdjacentSet(edgePos);
            for (const auto edgePosNext : nextEdgeTmp) {
                resultIter.insert(edgePosNext);
            }
        }
        for (auto pos : resultIter)
            result.insert(pos);
        prevIter = resultIter;
    }
    return result;
}

BattlePositionSet BattleFieldGeometry::closestTo(const BattlePosition pos, const BattlePositionSet& candidates) const
{
    if (candidates.empty())
        return {};

    std::vector<BattlePosition> all(candidates.cbegin(), candidates.cend());

    std::sort(all.begin(), all.end(), [pos](const BattlePosition & l, const BattlePosition & r){
        const int decartL = pos.decartDistanceSqr(l);
        const int decartR = pos.decartDistanceSqr(r);
        return decartL < decartR;
    });
    BattlePositionSet result;
    BattlePosition firstClosest = all[0];
    const int decartFirst = pos.decartDistanceSqr(firstClosest);
    for (auto candidate : all) {
        const int decartCandidate = pos.decartDistanceSqr(candidate);
        if (decartCandidate != decartFirst)
            break;
        result.insert(candidate);
    }
    assert(!result.empty());
    return result;
}

BattlePosition BattleFieldPreset::calcPosition(bool attacker, int orderIndex, int totalCount, bool compactPositioning) const
{
    assert(orderIndex >= 0);
    static const std::vector<BattlePosition> objAtt {
        BattlePosition{5, 3},
        BattlePosition{9, 3},
        BattlePosition{4, 5},
        BattlePosition{7, 5},
        BattlePosition{10, 5},
        BattlePosition{5, 7},
        BattlePosition{9, 7},
    };
    static const std::vector<std::vector<int>> sparsePositions {
        {},
        {         5          },
        {   2,          8    },
        {   2,    5,    8    },
        {0,    4,    6,    10},
        {0, 2,    5,    8, 10},
        {0, 2, 4,    6, 8, 10},
        {0, 2, 4, 5, 6, 8, 10},
    };
    static const std::vector<std::vector<int>> compactPositions {
        {},
        {         5          },
        {      4,    6       }, // !
        {      4, 5, 6       }, // !
        {   2, 4,    6, 8    }, // !
        {   2, 4, 5, 6, 8    }, // !
        {0, 2, 4,    6, 8, 10},
        {0, 2, 4, 5, 6, 8, 10},
    };
    static const std::map<FieldLayout, std::vector<FieldLayoutPos>> objDef {
        {FieldLayout::Object,{
            FieldLayoutPos::TR,
            FieldLayoutPos::BR,
            FieldLayoutPos::BL,
            FieldLayoutPos::TL,
            FieldLayoutPos::R,
            }
        },
        {FieldLayout::Churchyard1,{
            FieldLayoutPos::TR,
            FieldLayoutPos::BR,
            FieldLayoutPos::BL,
            FieldLayoutPos::TL,
            FieldLayoutPos::R,
            FieldLayoutPos::L,
            }
        },
        {FieldLayout::Churchyard2,{
            FieldLayoutPos::TR,
            FieldLayoutPos::BR,
            FieldLayoutPos::BL,
            FieldLayoutPos::TL,
            FieldLayoutPos::T,
            FieldLayoutPos::B,
            }
        },
        {FieldLayout::Ruins,{
            FieldLayoutPos::TR,
            FieldLayoutPos::BR,
            FieldLayoutPos::BL,
            FieldLayoutPos::TL,
            FieldLayoutPos::T,
            }
        },
        {FieldLayout::Spit,{
            FieldLayoutPos::R,
            FieldLayoutPos::L,
            FieldLayoutPos::B,
            FieldLayoutPos::T,
            }
        },
    };
    auto calcDefaultY = [](int orderIndex, int totalCount, bool compactPositioning) {
        const auto & mapping = compactPositioning ? compactPositions : sparsePositions;
        assert(totalCount <= 7);
        assert(orderIndex <= 7);
        return mapping[totalCount][orderIndex];
    };
    auto calcRealPos = [this](const BattlePosition & pseudo) {
       if (pseudo.x == -1)
           return BattlePosition{field.width - 1, pseudo.y};
       if (pseudo.x == -2)
           return BattlePosition{field.width / 2, pseudo.y};

       return pseudo;
    };
    auto dirToPseudo = [](FieldLayoutPos dir) -> BattlePosition {
      switch(dir) {
          case FieldLayoutPos::TR    : return {-1, 0};
          case FieldLayoutPos::R     : return {-1, 5};
          case FieldLayoutPos::BR    : return {-1, 10};
          case FieldLayoutPos::TL    : return {1, 0};
          case FieldLayoutPos::L     : return {1, 5};
          case FieldLayoutPos::BL    : return {1, 10};
          case FieldLayoutPos::T     : return {-2, 0};
          case FieldLayoutPos::B     : return {-2, 10};
      default: break;
      }
      return {};
    };
    BattlePosition pos;
    if (attacker) {
        if (layout == FieldLayout::Standard) {
            const int y = calcDefaultY(orderIndex, totalCount, compactPositioning);
            pos = {0, y};
        } else {
            pos = objAtt.at(orderIndex);
        }
        pos = calcRealPos(pos);

    } else {
        if (layout == FieldLayout::Standard) {
            const int y = calcDefaultY(orderIndex, totalCount, compactPositioning);
            pos = {-1, y};
        } else {
            const auto & dirVector = objDef.at(layout);
            if (orderIndex >= static_cast<int>(dirVector.size()))
                return {};

            pos = dirToPseudo(dirVector[orderIndex]);
        }
        pos = calcRealPos(pos);
    }

    return pos;
}



}
