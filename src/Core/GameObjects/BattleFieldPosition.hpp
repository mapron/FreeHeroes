/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <utility>
#include <vector>
#include <set>
#include <limits>
#include <array>
#include <compare>
#include <unordered_map>

namespace FreeHeroes::Core {

enum class BattleDirection { None = -1, TR = 0, R, BR, BL, L, TL/*, Top, Bottom*/ };
enum class BattleDirectionPrecise { // 16 directions, oredered in clockwise manner.
    None = -1,
    Top   , TopTopRight     , TopRight   , TopRightRight,
    Right , BottomRightRight, BottomRight, BottomBottomRight,
    Bottom, BottomBottomLeft, BottomLeft , BottomLeftLeft,
    Left  , TopLeftLeft     , TopLeft    , TopTopLeft,
};
enum class BattleAttackDirection {
    None = -1, TR = 0, R, BR, BL, L, TL, T, B
};

struct BattlePositionDecart {
    static constexpr const int invalid = std::numeric_limits<int>::min();
    int x = invalid;
    int y = invalid;
};

struct BattlePosition {
    static constexpr const int invalid = std::numeric_limits<int>::min();
    static constexpr const int decartMultiplier = 2;
    int x = invalid;
    int y = invalid;

    constexpr auto asTuple() const noexcept { return std::tie(x, y); }

    // that line is not working for clang, but gcc is ok.
    // friend constexpr auto operator <=>(const T& lh, const T& rh) noexcept { return lh.asTuple() <=> rh.asTuple(); }
    friend constexpr bool operator ==(const BattlePosition& lh, const BattlePosition& rh) noexcept { return lh.asTuple() == rh.asTuple(); }
    friend constexpr bool operator !=(const BattlePosition& lh, const BattlePosition& rh) noexcept { return lh.asTuple() != rh.asTuple(); }
    friend constexpr bool operator < (const BattlePosition& lh, const BattlePosition& rh) noexcept { return lh.asTuple() <  rh.asTuple(); }

    constexpr BattlePosition operator +(const BattlePosition& rh) const noexcept { return {x + rh.x, y + rh.y}; }

    constexpr bool isEmpty() const noexcept { return x == invalid || y == invalid; }

    constexpr BattlePositionDecart toDecartCoordinates() const noexcept {
        const bool evenRow = y % 2 == 0;
        const int pseudoX = x * decartMultiplier + evenRow * (decartMultiplier / 2);
        return {pseudoX, y * decartMultiplier};
    }


    constexpr int hexDistance(const BattlePosition & to) const noexcept {
        const int x1 = x;
        const int x2 = to.x;
        const int y1 = y;
        const int y2 = to.y;

        const int  du = (y2 / 2 + x2) - (y1 / 2 + x1 );
        const int  dv = y2 - y1;
        const int adu = du < 0 ? -du : du;
        const int adv = dv < 0 ? -dv : dv;

         // if du and dv have the same sign: max(|du|, |dv|), by using the diagonals
         // if du and dv have different signs: |du| + |dv|, because the diagonals are unproductive
        return ((du >= 0 && dv >= 0) || (du < 0 && dv < 0)) ? std::max(adu, adv) : (adu + adv);
    }

    constexpr int decartDistanceSqr(const BattlePosition & to) const noexcept {
        const auto decMe = toDecartCoordinates();
        const auto decTo = to.toDecartCoordinates();
        const int dx = decMe.x - decTo.x;
        const int dy = decMe.y - decTo.y;
        return dx  * dx + dy * dy;
    }

    constexpr BattleDirectionPrecise preciseDirectionTo(const BattlePosition & to) const noexcept {
        if (*this == to)
            return BattleDirectionPrecise::None;

        const auto decMe = toDecartCoordinates();
        const auto decTo = to.toDecartCoordinates();

        const int dx = decTo.x - decMe.x;
        const int dy = decTo.y - decMe.y;
        const int absDx = dx > 0 ? dx : -dx;
        const int absDy = dy > 0 ? dy : -dy;

        // 1/5 is a approximation for tangent(11.25)
        // 2/3 is a approximation for tangent(33.75)
        const int directionInSquare =
            (absDy * 1 > absDx * 5) ? 0 :
            (absDy * 2 > absDx * 3) ? 1 :
            (absDy * 3 > absDx * 2) ? 2 :
            (absDy * 5 > absDx * 1) ? 3 : 4;

        if (dx >= 0 && dy >= 0) { // Bottom-right square
            return static_cast<BattleDirectionPrecise>(static_cast<int>(BattleDirectionPrecise::Bottom) - directionInSquare);
        } else if (dx <= 0 && dy <= 0) { // Top-left square
            if (directionInSquare == 0)
                return BattleDirectionPrecise::Top;
            return static_cast<BattleDirectionPrecise>(static_cast<int>(BattleDirectionPrecise::TopTopLeft) - directionInSquare + 1);
        } else if (dx <= 0 && dy >= 0) { // Bottom-left square
            return static_cast<BattleDirectionPrecise>(static_cast<int>(BattleDirectionPrecise::Bottom) + directionInSquare);
        } else { // Top-right square
            return static_cast<BattleDirectionPrecise>(static_cast<int>(BattleDirectionPrecise::Top) + directionInSquare);
        }
    }
};

}

namespace std {
template <>
struct hash<FreeHeroes::Core::BattlePosition>
{
    std::size_t operator()(const FreeHeroes::Core::BattlePosition& k) const
    {
        using std::hash;
        return (hash<int>()(k.x)) ^ (hash<int>()(k.y) << 1);
    }
};
}
namespace FreeHeroes::Core {

using BattlePositionPair = std::pair<BattlePosition, BattlePosition>;
using BattlePositionPath = std::vector<BattlePosition>;
using BattlePositionSet = std::set<BattlePosition>; // important not unordered_set - ordering feature is utilized
using BattlePositionDistanceMap = std::unordered_map<BattlePosition, int>;

class BattlePositionExtended {
public:
    constexpr BattlePositionExtended() = default;

    enum class Sub   { Main, Secondary };
    enum class Sight { ToRight, ToLeft };
    enum class Side  { Right, Left };


    constexpr void setMainPos (BattlePosition pos) noexcept { m_pos[(int)Sub::Main] = pos; updateSecondary(); }
    constexpr void setSight   (Sight sight       ) noexcept { m_sight   = sight  ; updateSecondary(); }
    constexpr void setLarge   (bool isLarge      ) noexcept { m_isLarge = isLarge; updateSecondary(); }

    constexpr Sub getRightPosSub(    ) const noexcept { return (m_sight == Sight::ToRight ? Sub::Secondary : Sub::Main); }
    constexpr Sub getLeftPosSub(     ) const noexcept { return (m_sight == Sight::ToLeft  ? Sub::Secondary : Sub::Main); }
    constexpr Sub getPosSub(Side side) const noexcept { return side == Side::Right ? getRightPosSub() : getLeftPosSub();}

    constexpr void setRightPos(BattlePosition pos) noexcept { m_pos[0] = m_isLarge && m_sight == Sight::ToRight ? BattlePosition{pos.x-1, pos.y} : pos; updateSecondary(); }
    constexpr void setLeftPos (BattlePosition pos) noexcept { m_pos[0] = m_isLarge && m_sight == Sight::ToLeft  ? BattlePosition{pos.x+1, pos.y} : pos; updateSecondary(); }

    constexpr bool isEmpty() const noexcept {return m_pos[0].isEmpty();}
    constexpr bool isLarge() const noexcept {return m_isLarge;}

    constexpr Sight sightDirection() const noexcept {return m_sight;}
    constexpr Sight inverseSightDirection() const noexcept {return m_sight == Sight::ToRight ? Sight::ToLeft : Sight::ToRight;}
    constexpr bool  sightDirectionIsRight() const noexcept {return m_sight == Sight::ToRight; }
    constexpr bool  sightDirectionIsLeft () const noexcept {return m_sight == Sight::ToLeft; }

    constexpr BattlePosition mainPos             () const noexcept { return getPosBySub(Sub::Main       );}
    constexpr BattlePosition rightPos            () const noexcept { return getPosBySub(getRightPosSub());}
    constexpr BattlePosition leftPos             () const noexcept { return getPosBySub(getLeftPosSub ());}
    constexpr BattlePosition secondaryPos        () const noexcept { return getPosBySub(Sub::Secondary  );}
    constexpr BattlePosition specificPos(Sub sub  ) const noexcept { return getPosBySub(sub             ); }
    constexpr BattlePosition specificPos(Side side) const noexcept { return getPosBySub(getPosSub(side) ); }

    constexpr BattlePositionExtended moveMainTo(BattlePosition pos) const noexcept {
        BattlePositionExtended copy = *this;
        copy.setMainPos(pos);
        return copy;
    }

    constexpr bool contains(BattlePosition pos) const noexcept { return getPosBySub(Sub::Main) == pos || getPosBySub(Sub::Secondary) == pos;}

    constexpr auto asTuple() const noexcept { return std::tie(m_pos, m_sight, m_isLarge);}
    constexpr bool operator == (const BattlePositionExtended& rh) const noexcept { return asTuple() == rh.asTuple();}
    constexpr bool operator != (const BattlePositionExtended& rh) const noexcept { return asTuple() != rh.asTuple();}

    constexpr std::array<BattlePositionPair, 4> possibleLinesTo(const BattlePositionExtended& to)  const noexcept {
        return {{
            { rightPos() , to.rightPos()},
            { rightPos() , to.leftPos ()},
            { leftPos () , to.rightPos()},
            { leftPos () , to.leftPos ()},
        }};
    }
    constexpr std::array<BattlePositionPair, 2> possibleLinesTo(const BattlePosition& to)  const noexcept {
        return {{
            { rightPos() , to},
            { leftPos () , to},
        }};
    }

    constexpr std::pair<int, BattlePositionPair> shortestHexDistance(const BattlePositionExtended & to) const noexcept {
        const auto variants = possibleLinesTo(to);
        auto it = std::min_element(variants.cbegin(), variants.cend(), [](const BattlePositionPair &lh, const BattlePositionPair &rh){
            return lh.first.hexDistance(lh.second) < rh.first.hexDistance(rh.second);
        });
        return {it->first.hexDistance(it->second), *it};
    }
    constexpr std::pair<int, BattlePositionPair> shortestHexDistance(const BattlePosition & to) const noexcept {
        const auto variants = possibleLinesTo(to);
        auto it = std::min_element(variants.cbegin(), variants.cend(), [](const BattlePositionPair &lh, const BattlePositionPair &rh){
            return lh.first.hexDistance(lh.second) < rh.first.hexDistance(rh.second);
        });
        return {it->first.hexDistance(it->second), *it};
    }
    constexpr std::pair<int, BattlePositionPair> shortestDecartDistanceSqr(const BattlePositionExtended & to) const noexcept {
        const auto variants = possibleLinesTo(to);
        auto it = std::min_element(variants.cbegin(), variants.cend(), [](const BattlePositionPair &lh, const BattlePositionPair &rh){
            return lh.first.decartDistanceSqr(lh.second) < rh.first.decartDistanceSqr(rh.second);
        });
        return {it->first.decartDistanceSqr(it->second), *it};
    }

    constexpr bool isInversePositionRelatedTo(const BattlePositionExtended& rh) const noexcept {
        if (mainPos().y == rh.mainPos().y) {
            if (sightDirection() == Sight::ToRight)
                return mainPos().x > rh.mainPos().x;
            else
                return mainPos().x < rh.mainPos().x;
        }
        if (sightDirection() == Sight::ToRight) {
            const auto pseudoFrom = leftPos().toDecartCoordinates();
            const auto pseudoTo   = rh.rightPos().toDecartCoordinates();
            return pseudoFrom.x > pseudoTo.x;
        } else {
            const auto pseudoFrom = rightPos().toDecartCoordinates();
            const auto pseudoTo   = rh.leftPos().toDecartCoordinates();
            return pseudoFrom.x < pseudoTo.x;
        }
    }
    using AttackVariant = std::pair<BattleAttackDirection, Side>;
    using AttackVariantList = std::vector<AttackVariant>;
    static const AttackVariantList & getAttackSuggestions(bool sourceIsWide, bool targetIsWide) {
        static const AttackVariantList commonToCommonVariants {
            {BattleAttackDirection::TR, Side::Right},
            {BattleAttackDirection::R , Side::Right},
            {BattleAttackDirection::BR, Side::Right},
            {BattleAttackDirection::BL, Side::Right},
            {BattleAttackDirection::L , Side::Right},
            {BattleAttackDirection::TL, Side::Right},
        };

        static const AttackVariantList commonToWideVariants {
            {BattleAttackDirection::TR, Side::Right},
            {BattleAttackDirection::R , Side::Right},
            {BattleAttackDirection::BR, Side::Right},
            {BattleAttackDirection::BL, Side::Right},
    //      {BattleAttackDirection::L , Side::Right}, // blocked by other cell, you cannot attack right side of wide unit from its center (from left)
            {BattleAttackDirection::TL, Side::Right},
            {BattleAttackDirection::TR, Side::Left},
    //      {BattleAttackDirection::R , Side::Left}, // blocked by other cell, same
            {BattleAttackDirection::BR, Side::Left},
            {BattleAttackDirection::BL, Side::Left},
            {BattleAttackDirection::L , Side::Left},
            {BattleAttackDirection::TL, Side::Left},
        };

        static const AttackVariantList wideToCommonVariants {
            {BattleAttackDirection::TR  , Side::Right },
            {BattleAttackDirection::R   , Side::Right },
            {BattleAttackDirection::BR  , Side::Right },
            {BattleAttackDirection::BL  , Side::Right },
            {BattleAttackDirection::L   , Side::Right },
            {BattleAttackDirection::TL  , Side::Right },
            {BattleAttackDirection::B   , Side::Right },
            {BattleAttackDirection::T   , Side::Right },
        };

        static const AttackVariantList wideToWideVariants {
            {BattleAttackDirection::TR    , Side::Right },
            {BattleAttackDirection::R     , Side::Right },
            {BattleAttackDirection::BR    , Side::Right },
            {BattleAttackDirection::BL    , Side::Right },
    //      {BattleAttackDirection::L     , Side::Right }, // blocked by other cell, same
            {BattleAttackDirection::TL    , Side::Right },
            {BattleAttackDirection::B     , Side::Right },
            {BattleAttackDirection::T     , Side::Right },
            {BattleAttackDirection::TR    , Side::Left  },
    //      {BattleAttackDirection::R     , Side::Left  }, // blocked by other cell, same
            {BattleAttackDirection::BR    , Side::Left  },
            {BattleAttackDirection::BL    , Side::Left  },
            {BattleAttackDirection::L     , Side::Left  },
            {BattleAttackDirection::TL    , Side::Left  },
            {BattleAttackDirection::B     , Side::Left  },
            {BattleAttackDirection::T     , Side::Left  },
        };
        return sourceIsWide ? (targetIsWide ? wideToWideVariants : wideToCommonVariants) : (targetIsWide ? commonToWideVariants : commonToCommonVariants);
    }
private:
    std::array<BattlePosition, 2> m_pos;
    Sight m_sight = Sight::ToRight;
    bool m_isLarge = false;
private:
    constexpr BattlePosition & getPosBySub(Sub sub) noexcept { return m_pos[m_isLarge ? static_cast<int>(sub) : 0]; }
    constexpr const BattlePosition & getPosBySub(Sub sub) const noexcept { return m_pos[m_isLarge ? static_cast<int>(sub) : 0]; }
    constexpr void updateSecondary()  noexcept {
        m_pos[1] = m_pos[0];
        if (m_isLarge)
            m_pos[1] =  m_sight == Sight::ToRight ? BattlePosition{m_pos[0].x+1, m_pos[0].y} : BattlePosition{m_pos[0].x-1, m_pos[0].y} ;
    }

};

}

