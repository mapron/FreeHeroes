/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "RmgUtil/MapTileContainer.hpp"
#include "RmgUtil/MapTileRegionWithEdge.hpp"

#include <gtest/gtest.h>

using namespace FreeHeroes;

struct CollisionTestParams {
    std::string                            m_id;
    int                                    m_width  = 0;
    int                                    m_height = 0;
    std::string                            m_object;
    FHPos                                  m_shift;
    MapTileRegionWithEdge::CollisionResult m_result = MapTileRegionWithEdge::CollisionResult::HasShift;
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
                    "..OO."
                    "..OO."
                    "....."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileRegionWithEdge::CollisionResult::InvalidInputs,
    },
    CollisionTestParams{
        .m_id     = "simple1",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..OO."
                    "..XO."
                    "....."
                    ".....",
        .m_shift  = { 1, -1 },
    },
    CollisionTestParams{
        .m_id     = "simple2",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....O"
                    "..OO."
                    "..XO."
                    "....."
                    ".....",
        .m_shift  = { 1, -1 },
    },
    CollisionTestParams{
        .m_id     = "no_collide",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..OO."
                    "..-O."
                    "....."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileRegionWithEdge::CollisionResult::NoCollision,
    },
    CollisionTestParams{
        .m_id     = "compensated1",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    ".OOO."
                    ".OXO."
                    ".OOO."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileRegionWithEdge::CollisionResult::ImpossibleShift,
    },
    CollisionTestParams{
        .m_id     = "compensated2",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    ".OOO."
                    ".XOX."
                    ".OOO."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileRegionWithEdge::CollisionResult::ImpossibleShift,
    },
    CollisionTestParams{
        .m_id     = "compensated3",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    ".XXO."
                    ".OOO."
                    ".OXX."
                    ".....",
        .m_shift  = FHPos{},
        .m_result = MapTileRegionWithEdge::CollisionResult::ImpossibleShift,
    },

    CollisionTestParams{
        .m_id     = "large1",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".OOO."
                    "OOOOO"
                    "XOOOO"
                    "OOOOO"
                    ".OOO.",
        .m_shift  = { 1, 0 },
    },
    CollisionTestParams{
        .m_id     = "large2",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".OOO."
                    "OOOOO"
                    "OXOOO"
                    "OOOOO"
                    ".OOO.",
        .m_shift  = { 2, 0 },
    },
    CollisionTestParams{
        .m_id     = "large3",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".XOX."
                    "OOOOO"
                    "OXOOO"
                    "OOOOO"
                    ".OOO.",
        .m_shift  = { 2, 1 },
    },
    CollisionTestParams{
        .m_id     = "large4",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".OOO."
                    "OOOXO"
                    "OOOOO"
                    "OOOOO"
                    ".OOO.",
        .m_shift  = { -2, 2 },
    },

    CollisionTestParams{
        .m_id     = "partial_compensate",
        .m_width  = 5,
        .m_height = 5,
        .m_object = ".OOX."
                    "OOOOO"
                    "OXOOX"
                    "OOOOO"
                    ".OOO.",
        .m_shift  = { -2, 2 },
    },
};

TEST_P(CollisionTest, Basic)
{
    CollisionTestParams params = GetParam();

    MapTileContainer tileContainer;
    tileContainer.init(params.m_width, params.m_height, 1);

    MapTileRegion object, obstacle;
    std::string   composeCheck;

    MapTileRegionWithEdge::decompose(&tileContainer, object, obstacle, params.m_object, params.m_width, params.m_height);

    MapTileRegionWithEdge::compose(object, obstacle, composeCheck);

    ASSERT_EQ(composeCheck, params.m_object);

    const auto [calcResult, calculatedCollision] = MapTileRegionWithEdge::getCollisionShiftForObject(object, obstacle);
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
