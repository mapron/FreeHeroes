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
    std::string m_id;
    int         m_width  = 0;
    int         m_height = 0;
    std::string m_object;
    FHPos       m_shift;

    MapTileRegionWithEdge::CollisionResult m_result = MapTileRegionWithEdge::CollisionResult::HasShift;
};

struct FloodTestParams {
    std::string              m_id;
    int                      m_width  = 0;
    int                      m_height = 0;
    std::string              m_object;
    std::vector<std::string> m_parts;
};

struct KmeansTestParams {
    std::string        m_id;
    int                m_width  = 0;
    int                m_height = 0;
    std::string        m_object;
    std::vector<FHPos> m_start;
    std::string        m_parts;
};

std::ostream& operator<<(std::ostream& os, const CollisionTestParams& p)
{
    return os << p.m_id;
}

std::ostream& operator<<(std::ostream& os, const FloodTestParams& p)
{
    return os << p.m_id;
}

std::ostream& operator<<(std::ostream& os, const KmeansTestParams& p)
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

class FloodFillTest : public ::testing::Test
    , public testing::WithParamInterface<FloodTestParams> {
};

class KmeansTest : public ::testing::Test
    , public testing::WithParamInterface<KmeansTestParams> {
};

const std::vector<CollisionTestParams> testParamsCollision{
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

    MapTileRegion::decompose(&tileContainer, object, obstacle, params.m_object, params.m_width, params.m_height);

    MapTileRegion::compose(&tileContainer, object, obstacle, composeCheck);

    ASSERT_EQ(composeCheck, params.m_object);

    const auto [calcResult, calculatedCollision] = MapTileRegionWithEdge::getCollisionShiftForObject(object, obstacle);
    const FHPos expectedCollision                = params.m_shift;

    ASSERT_EQ(calculatedCollision, expectedCollision);
    ASSERT_EQ(calcResult, params.m_result);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         CollisionTest,
                         testing::ValuesIn(testParamsCollision),
                         [](const testing::TestParamInfo<CollisionTestParams>& info) {
                             return info.param.m_id;
                         });

const std::vector<FloodTestParams> testParamsFloodFill{
    FloodTestParams{
        .m_id     = "simple",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..OO."
                    "..OO."
                    "....."
                    ".....",
        .m_parts  = {
            "....."
             "..OO."
             "..OO."
             "....."
             ".....",
        },
    },
    FloodTestParams{
        .m_id     = "two",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "O...."
                    "O.OO."
                    "O.OO."
                    "O...."
                    "O....",
        .m_parts  = {
            "O...."
             "O...."
             "O...."
             "O...."
             "O....",
            "....."
             "..OO."
             "..OO."
             "....."
             ".....",
        },
    },

    FloodTestParams{
        .m_id     = "seven",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "O...."
                    "O.OO."
                    "O.OO."
                    "O...."
                    "O....",
        .m_parts  = {
            "O...."
             "O...."
             "O...."
             "O...."
             "O....",
            "....."
             "..OO."
             "..OO."
             "....."
             ".....",
        },
    },

};

TEST_P(FloodFillTest, Basic)
{
    FloodTestParams params = GetParam();

    MapTileContainer tileContainer;
    tileContainer.init(params.m_width, params.m_height, 1);

    MapTileRegion object, obstacle;

    MapTileRegion::decompose(&tileContainer, object, obstacle, params.m_object, params.m_width, params.m_height);

    MapTileRegionList expectedSegments;
    for (auto& part : params.m_parts) {
        MapTileRegion object2, obstacle2;

        MapTileRegion::decompose(&tileContainer, object2, obstacle2, part, params.m_width, params.m_height);
        expectedSegments.push_back(object2);
    }

    const auto calculatedSegments = object.splitByFloodFill(true);

    ASSERT_EQ(calculatedSegments, expectedSegments);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         FloodFillTest,
                         testing::ValuesIn(testParamsFloodFill),
                         [](const testing::TestParamInfo<FloodTestParams>& info) {
                             return info.param.m_id;
                         });

const std::vector<KmeansTestParams> testParamsKmeans{
    KmeansTestParams{
        .m_id = "seven",

        .m_width  = 18,
        .m_height = 10,
        .m_object = "...OOOOOOOOOOOOOO."
                    "...OOOOOOOOOOOOOO."
                    "....OOOOOOOOOOOOO."
                    "....OOOOOOOOOOOOO."
                    "....OOOOOOOOOOOOO."
                    "....OOOOOOOOOOOOO."
                    ".....OOOOOOOOOOOO."
                    ".....OOOOOOOOOOOO."
                    ".....OOOOOOOOO...."
                    ".....OOO..........",
        .m_start  = {
            FHPos(3, 0, 0),
            FHPos(14, 0, 0),
            FHPos(12, 1, 0),
            FHPos(10, 2, 0),
            FHPos(9, 3, 0),
            FHPos(8, 4, 0),
            FHPos(6, 5, 0),
            FHPos(6, 6, 0),
            FHPos(5, 7, 0),
            FHPos(5, 8, 0),
        },
        .m_parts = "...00003333111111."
                   "...00663333111111."
                   "....6663334411111."
                   "....6663344441111."
                   "....7777444422222."
                   "....7775555222222."
                   ".....885555522222."
                   ".....899555522222."
                   ".....999955522...."
                   ".....999..........",
    },
};

TEST_P(KmeansTest, Basic)
{
    KmeansTestParams params = GetParam();

    MapTileContainer tileContainer;
    tileContainer.init(params.m_width, params.m_height, 1);

    MapTileRegion object, obstacle;
    MapTileRegion::decompose(&tileContainer, object, obstacle, params.m_object, params.m_width, params.m_height);

    MapTileRegionList expectedSegmentsList;
    std::string       expectedSegments = params.m_parts;
    MapTileRegion::decompose(&tileContainer, expectedSegmentsList, expectedSegments, params.m_width, params.m_height);

    MapTileRegion::SplitRegionSettingsList settings;
    settings.resize(params.m_start.size());
    for (size_t i = 0; i < params.m_start.size(); ++i)
        settings[i].m_start = tileContainer.m_tileIndex.at(params.m_start[i]);

    const auto calculatedSegmentsList = object.splitByKExt(settings);

    std::string calculatedSegments;
    MapTileRegion::compose(&tileContainer, calculatedSegmentsList, calculatedSegments, false);

    ASSERT_EQ(calculatedSegments, expectedSegments);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         KmeansTest,
                         testing::ValuesIn(testParamsKmeans),
                         [](const testing::TestParamInfo<KmeansTestParams>& info) {
                             return info.param.m_id;
                         });
