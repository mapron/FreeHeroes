/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "RmgUtil/MapTileContainer.hpp"

#include <gtest/gtest.h>

using namespace FreeHeroes;

struct CollisionTestParams {
    std::string                  m_id;
    int                          m_width  = 0;
    int                          m_height = 0;
    std::string                  m_object;
    std::string                  m_obstac;
    FHPos                        m_shift;
    MapTileArea::CollisionResult m_result = MapTileArea::CollisionResult::HasShift;
};

std::ostream& operator<<(std::ostream& os, const CollisionTestParams& p)
{
    return os << p.m_id;
}

namespace FreeHeroes {
std::ostream& operator<<(std::ostream& os, const FHPos& p)
{
    return os << p.toPrintableString();
}
}

class CollisionTest : public ::testing::Test
    , public testing::WithParamInterface<CollisionTestParams> {
};

const std::vector<CollisionTestParams> testParams{
    CollisionTestParams{
        .m_id     = "invalid",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..XX."
                    "..XX."
                    "....."
                    ".....",
        .m_obstac = "....."
                    "....."
                    "....."
                    "....."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileArea::CollisionResult::InvalidInputs,
    },
    CollisionTestParams{
        .m_id     = "simple1",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..XX."
                    "..XX."
                    "....."
                    ".....",
        .m_obstac = "....."
                    "....."
                    "..X.."
                    "....."
                    ".....",
        .m_shift  = { 1, -1 },
    },
    CollisionTestParams{
        .m_id     = "simple2",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....X"
                    "..XX."
                    "..XX."
                    "....."
                    ".....",
        .m_obstac = "....."
                    "....."
                    "..X.."
                    "....."
                    ".....",
        .m_shift  = { 1, -1 },
    },
    CollisionTestParams{
        .m_id     = "no_collide",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..XX."
                    "...X."
                    "....."
                    ".....",
        .m_obstac = "....."
                    "....."
                    "..X.."
                    "....."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileArea::CollisionResult::NoCollision,
    },
    CollisionTestParams{
        .m_id     = "compensated1",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    ".XXX."
                    ".XXX."
                    ".XXX."
                    ".....",
        .m_obstac = "....."
                    "....."
                    "..X.."
                    "....."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileArea::CollisionResult::ImpossibleShift,
    },
    CollisionTestParams{
        .m_id     = "compensated2",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    ".XXX."
                    ".XXX."
                    ".XXX."
                    ".....",
        .m_obstac = "....."
                    "....."
                    ".X.X."
                    "....."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileArea::CollisionResult::ImpossibleShift,
    },
    CollisionTestParams{
        .m_id     = "compensated3",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    ".XXX."
                    ".XXX."
                    ".XXX."
                    ".....",
        .m_obstac = "....."
                    ".XX.."
                    "....."
                    "..XX."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileArea::CollisionResult::ImpossibleShift,
    },

    CollisionTestParams{
        .m_id     = "large1",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".XXX."
                    "XXXXX"
                    "XXXXX"
                    "XXXXX"
                    ".XXX.",
        .m_obstac = "....."
                    "....."
                    "X...."
                    "....."
                    ".....",
        .m_shift  = { 1, 0 },
    },
    CollisionTestParams{
        .m_id     = "large2",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".XXX."
                    "XXXXX"
                    "XXXXX"
                    "XXXXX"
                    ".XXX.",
        .m_obstac = "....."
                    "....."
                    ".X..."
                    "....."
                    ".....",
        .m_shift  = { 2, 0 },
    },
    CollisionTestParams{
        .m_id     = "large3",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".XXX."
                    "XXXXX"
                    "XXXXX"
                    "XXXXX"
                    ".XXX.",
        .m_obstac = ".X.X."
                    "....."
                    ".X..."
                    "....."
                    ".....",
        .m_shift  = { 2, 1 },
    },
    CollisionTestParams{
        .m_id     = "large4",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".XXX."
                    "XXXXX"
                    "XXXXX"
                    "XXXXX"
                    ".XXX.",
        .m_obstac = "....."
                    "...X."
                    "....."
                    "....."
                    ".....",
        .m_shift  = { -2, 2 },
    },

    CollisionTestParams{
        .m_id     = "partial_compensate",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".XXX."
                    "XXXXX"
                    "XXXXX"
                    "XXXXX"
                    ".XXX.",
        .m_obstac = "...X."
                    "....."
                    ".X..X"
                    "....."
                    ".....",
        .m_shift  = { -2, 2 },
    },
};

TEST_P(CollisionTest, Basic)
{
    CollisionTestParams params = GetParam();

    MapTileContainer tileContainer;
    tileContainer.init(params.m_width, params.m_height, 1);

    MapTileRegion object, obstacle;

    for (int y = 0; y < params.m_height; ++y) {
        for (int x = 0; x < params.m_width; ++x) {
            auto*        tile             = tileContainer.m_tileIndex.at(FHPos{ x, y, 0 });
            const size_t testOffset       = x + params.m_width * y;
            const bool   objectOccupied   = params.m_object[testOffset] != '.';
            const bool   obstacleOccupied = params.m_obstac[testOffset] != '.';
            if (objectOccupied)
                object.insert(tile);
            if (obstacleOccupied)
                obstacle.insert(tile);
        }
    }

    const auto [calcResult, calculatedCollision] = MapTileArea::getCollisionShiftForObject(object, obstacle);
    const FHPos expectedCollision                = params.m_shift;

    ASSERT_EQ(calculatedCollision, expectedCollision);
    ASSERT_EQ(calcResult, params.m_result);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         CollisionTest,
                         testing::ValuesIn(testParams),
                         [](const testing::TestParamInfo<CollisionTestParams>& info) {
                             return info.param.m_id;
                         });
