/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleFieldPathFinder.hpp"

#include <gtest/gtest.h>

using namespace FreeHeroes::Core;

std::ostream& operator<<(std::ostream& os, const BattlePosition& pos)
{
    return os << "(" << pos.x << ", " << pos.y << ")"; // whatever needed to print bar to os
}

class PathFindTest : public ::testing::Test {
public:
    PathFindTest()
        : finder(geometry)
    {
    }
    BattleFieldGeometry   geometry{ 4, 4 };
    BattleFieldPathFinder finder;

    /*
       0,0  1,0  2,0  3,0
     0,1  1,1  2,1  3,1
       0,2  1,2  2,2  3,2
     0,3  1,3  2,3  3,3
    */
};

TEST_F(PathFindTest, CheckDistance)
{
    finder.floodFill({ 0, 0 });

    ASSERT_TRUE(finder.distanceTo({ 0, 0 }) == 0);
    ASSERT_TRUE(finder.distanceTo({ 1, 0 }) == 1);
    ASSERT_TRUE(finder.distanceTo({ 0, 1 }) == 1);
    ASSERT_TRUE(finder.distanceTo({ 0, 2 }) == 2);
    ASSERT_TRUE(finder.distanceTo({ 1, 2 }) == 2);
    ASSERT_TRUE(finder.distanceTo({ 3, 3 }) == 4);

    finder.setObstacles({ { 0, 1 }, { 1, 1 } });
    /*
       0,0  1,0  2,0  3,0
      X    X   2,1  3,1
       0,2  1,2  2,2  3,2
     0,3  1,3  2,3  3,3
    */

    finder.floodFill({ 0, 0 });

    ASSERT_TRUE(finder.distanceTo({ 0, 0 }) == 0);
    ASSERT_TRUE(finder.distanceTo({ 0, 1 }) == -1);
    ASSERT_TRUE(finder.distanceTo({ 1, 1 }) == -1);
    ASSERT_TRUE(finder.distanceTo({ 0, 2 }) == 4);
    ASSERT_TRUE(finder.distanceTo({ 0, 3 }) == 5);

    finder.setObstacles({ { 0, 1 }, { 1, 1 }, { 1, 0 } });
    finder.floodFill({ 0, 0 });

    ASSERT_TRUE(finder.distanceTo({ 0, 0 }) == 0);
    ASSERT_TRUE(finder.distanceTo({ 3, 3 }) == -1);
}

TEST_F(PathFindTest, FindAvailable)
{
    finder.setObstacles({});
    finder.floodFill({ 1, 1 });

    BattlePositionSet all = geometry.getAllPositions();
    all.erase({ 1, 1 });

    // clang-format off
    BattlePositionSet expected { {0,0} , {1,0},
                               {0,1},      {2,1},
                                { 0,2} , {1,2} };
    // clang-format on
    auto av = finder.findAvailable(0);
    ASSERT_TRUE(av == BattlePositionSet());
    av = finder.findAvailable(1);
    ASSERT_TRUE(av == expected);

    av = finder.findAvailable(6);
    ASSERT_TRUE(av == all);
    av = finder.findAvailable(-1);
    ASSERT_TRUE(av == all);
}

TEST_F(PathFindTest, PathFind)
{
    finder.setObstacles({});
    finder.floodFill({ 0, 0 });

    auto path1exp = BattlePositionPath{ { 1, 1 }, { 0, 2 }, { 0, 3 } };
    auto path1    = finder.fromStartTo({ 0, 3 });
    ASSERT_TRUE(path1 == path1exp);

    finder.setObstacles({});
    finder.floodFill({ 0, 3 });

    auto path2exp = BattlePositionPath{ { 0, 2 }, { 1, 1 }, { 0, 0 } };
    auto path2    = finder.fromStartTo({ 0, 0 });
    ASSERT_TRUE(path2 == path2exp);

    auto path3 = finder.fromStartTo({ 0, 0 }, 2);
    ASSERT_TRUE(path3 == BattlePositionPath{});

    auto path4 = finder.fromStartTo({ 0, 0 }, 3);
    ASSERT_TRUE(path4 == path2exp);

    finder.setObstacles({ { 0, 1 }, { 1, 1 }, { 1, 0 } });
    finder.floodFill({ 3, 3 });

    auto path5 = finder.fromStartTo({ 0, 0 });
    ASSERT_TRUE(path5 == BattlePositionPath{});
}
TEST_F(PathFindTest, FloodFill)
{
    std::set<BattlePosition> result = geometry.getFloodFillFrom({ 1, 1 }, 1);
    // clang-format off
    std::set<BattlePosition> resultCheck {
        {0,0},  {1,0},
      {0,1},  {1,1},  {2,1},
        {0,2},  {1,2},
    };
    // clang-format on
    ASSERT_EQ(result, resultCheck);

    std::set<BattlePosition> result2 = geometry.getFloodFillFrom({ 0, 0 }, 2);
    // clang-format off
    std::set<BattlePosition> resultCheck2 {
        {0,0},  {1,0},  {2,0},
      {0,1},  {1,1},  {2,1},
        {0,2},  {1,2},
    };
    // clang-format on
    ASSERT_EQ(result2, resultCheck2);
}

TEST_F(PathFindTest, ClosestPos)
{
    /*
       ___  ___  ___  ___
     0,1   X  2,1  ___
       ___  ___  ___  3,2
     ___  ___  ___  3,3
    */
    std::set<BattlePosition> result = geometry.closestTo({ 1, 1 },
                                                         std::set<BattlePosition>{ { 0, 1 }, { 2, 1 }, { 3, 2 }, { 3, 3 } });
    std::set<BattlePosition> resultCheck{
        { 0, 1 }, { 2, 1 }
    };
    ASSERT_EQ(result, resultCheck);

    const int hex1 = BattlePosition{ 1, 2 }.hexDistance(BattlePosition{ 3, 1 });
    const int hex2 = BattlePosition{ 1, 2 }.hexDistance(BattlePosition{ 3, 2 });
    ASSERT_EQ(hex1, 2);
    ASSERT_EQ(hex2, 2);

    int decart2 = BattlePosition{ 1, 2 }.decartDistanceSqr({ 3, 2 });
    ASSERT_EQ(decart2, 16);
    int decart1 = BattlePosition{ 1, 2 }.decartDistanceSqr({ 3, 1 });
    ASSERT_EQ(decart1, 13);
    /*
       ___  ___  ___  ___
     ___  ___  ___  3,1
       ___  X   ___  3,2
     ___  ___  ___  ___
    */
    {
        std::set<BattlePosition> result = geometry.closestTo({ 1, 2 },
                                                             std::set<BattlePosition>{ { 3, 1 }, { 3, 2 } });
        std::set<BattlePosition> resultCheck{
            { 3, 1 }
        };
        ASSERT_EQ(result, resultCheck);
    }
}
using DistanceTestParams = std::pair<BattlePosition, BattlePosition>;
class DistanceTest : public PathFindTest
    , public testing::WithParamInterface<DistanceTestParams> {
};

TEST_P(DistanceTest, EqualToFlood)
{
    BattlePosition from = GetParam().first;
    BattlePosition to   = GetParam().second;

    finder.setObstacles({});
    int hexDistance = from.hexDistance(to);
    finder.floodFill(from);
    int floodDistance = finder.distanceTo(to);

    ASSERT_EQ(hexDistance, floodDistance);
}

const std::vector<DistanceTestParams> testParams = []() -> std::vector<DistanceTestParams> {
    std::vector<DistanceTestParams> result;
    int                             d = 4;
    for (int x1 = 0; x1 < d; ++x1) {
        for (int y1 = 0; y1 < d; ++y1) {
            for (int x2 = 0; x2 < d; ++x2) {
                for (int y2 = 0; y2 < d; ++y2) {
                    BattlePosition from{ x1, y1 };
                    BattlePosition to{ x2, y2 };
                    result.push_back({ from, to });
                }
            }
        }
    }
    return result;
}();

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         DistanceTest,
                         testing::ValuesIn(testParams));
