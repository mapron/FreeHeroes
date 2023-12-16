/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "RmgUtil/MapTileContainer.hpp"
#include "RmgUtil/MapTileRegionWithEdge.hpp"
#include "RmgUtil/MapTileRegionSegmentation.hpp"

#include <gtest/gtest.h>

using namespace FreeHeroes;

struct CommonSegmentationParams {
    std::string m_id;
    int         m_width  = 0;
    int         m_height = 0;
    std::string m_object;
    std::string m_parts;
};

struct CollisionTestParams {
    std::string m_id;
    int         m_width  = 0;
    int         m_height = 0;
    std::string m_object;
    FHPos       m_shift;

    MapTileRegionWithEdge::CollisionResult m_result = MapTileRegionWithEdge::CollisionResult::HasShift;
};

struct FloodTestParams : public CommonSegmentationParams {
};

struct KmeansTestParams : public CommonSegmentationParams {
    std::string m_startPoints;
};

struct GridTestParams : public CommonSegmentationParams {
    int  m_gridWidth  = 0;
    int  m_gridHeight = 0;
    int  m_threshold  = 0;
    bool m_uniteCheck = false;
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

std::ostream& operator<<(std::ostream& os, const GridTestParams& p)
{
    return os << p.m_id;
}

namespace FreeHeroes {
std::ostream& operator<<(std::ostream& os, const FHPos& p)
{
    return os << p.toPrintableString();
}
}

class CommonSegmentationTest {
public:
    MapTileContainer m_tileContainer;
    MergedRegion     m_reg;
    MapTileRegion    m_objectRegion;

    std::string       m_expectedSegmentsStr;
    MapTileRegionList m_expectedSegments;

    std::string       m_calculatedSegmentsStr;
    MapTileRegionList m_calculatedSegments;
    MapTileRegion     m_calculatedSegmentsUnited;

    void prepare(const CommonSegmentationParams& params)
    {
        m_tileContainer = MapTileContainer();
        m_tileContainer.init(params.m_width, params.m_height, 1);

        m_reg = MergedRegion();
        m_reg.initFromTileContainer(&m_tileContainer, 0);

        m_reg.load(params.m_object);
        m_objectRegion = m_reg.m_regions['O'];

        m_reg.load(params.m_parts);
        m_expectedSegmentsStr = MergedRegion::makePrintable(params.m_parts, m_reg.m_width);
        m_expectedSegments    = m_reg.getList();
    }

    void makeExpectedStr()
    {
        m_reg.setList(m_calculatedSegments);
        m_calculatedSegmentsStr = m_reg.dump();
    }

    void uniteSegemnts()
    {
        m_calculatedSegmentsUnited.clear();
        for (const auto& seg : m_calculatedSegments)
            m_calculatedSegmentsUnited.insert(seg);
    }
};

class CollisionTest : public ::testing::Test
    , public testing::WithParamInterface<CollisionTestParams>
    , public CommonSegmentationTest {
};

class FloodFillTest : public ::testing::Test
    , public testing::WithParamInterface<FloodTestParams>
    , public CommonSegmentationTest {
};

class KmeansTest : public ::testing::Test
    , public testing::WithParamInterface<KmeansTestParams>
    , public CommonSegmentationTest {
};

class GridTest : public ::testing::Test
    , public testing::WithParamInterface<GridTestParams>
    , public CommonSegmentationTest {
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

    //MapTileRegion object, obstacle;
    std::string composeCheck;

    MergedRegion reg;
    reg.initFromTileContainer(&tileContainer, 0);
    reg.load(params.m_object);

    const std::string saveCheck = reg.save();

    ASSERT_EQ(saveCheck, params.m_object);

    MapTileRegion objectOnly        = reg.m_regions['O'];
    MapTileRegion obstacleAndObject = reg.m_regions['X'];
    MapTileRegion obstacleOnly      = reg.m_regions['-'];
    MapTileRegion object            = objectOnly.unionWith(obstacleAndObject);
    MapTileRegion obstacle          = obstacleOnly.unionWith(obstacleAndObject);

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
    FloodTestParams{ {
        .m_id     = "simple",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "....."
                    "..OO."
                    "..OO."
                    "....."
                    ".....",
        .m_parts  = "....."
                    "..00."
                    "..00."
                    "....."
                    ".....",
    } },
    FloodTestParams{ {
        .m_id     = "two",
        .m_width  = 5,
        .m_height = 5,
        .m_object = "O...."
                    "O.OO."
                    "O.OO."
                    "O...."
                    "O....",
        .m_parts  = "0...."
                    "0.11."
                    "0.11."
                    "0...."
                    "0....",
    } },

};

TEST_P(FloodFillTest, BasicSegmentation)
{
    const auto& params = GetParam();
    this->prepare(params);

    m_calculatedSegments = m_objectRegion.splitByFloodFill(true);

    makeExpectedStr();

    ASSERT_EQ(m_calculatedSegmentsStr, m_expectedSegmentsStr);
    ASSERT_EQ(m_calculatedSegments, m_expectedSegments);

    uniteSegemnts();
    ASSERT_EQ(m_objectRegion, m_calculatedSegmentsUnited);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         FloodFillTest,
                         testing::ValuesIn(testParamsFloodFill),
                         [](const testing::TestParamInfo<FloodTestParams>& info) {
                             return info.param.m_id;
                         });

const std::vector<KmeansTestParams> testParamsKmeans{
    KmeansTestParams{
        {
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
            .m_parts  = "...00003333111111."
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
        {
            "...0..........1..."
            "............2....."
            "..........3......."
            ".........4........"
            "........5........."
            "......6..........."
            "......7..........."
            ".....8............"
            ".....9............"
            "..................",
        },
    },
};

TEST_P(KmeansTest, BasicSegmentation)
{
    const auto& params = GetParam();
    this->prepare(params);

    m_reg.load(params.m_startPoints);
    auto startPointRegions = m_reg.getList();

    KMeansSegmentationSettings settings;
    settings.m_items.resize(startPointRegions.size());
    for (size_t i = 0; i < startPointRegions.size(); ++i) {
        settings.m_items[i].m_initialCentroid = startPointRegions[i][0];
        //std::cout << settings.m_items[i].m_start->toPrintableString() << "\n";
    }
    m_calculatedSegments = m_objectRegion.splitByKExt(settings);

    makeExpectedStr();

    ASSERT_EQ(m_calculatedSegmentsStr, m_expectedSegmentsStr);
    ASSERT_EQ(m_calculatedSegments, m_expectedSegments);

    uniteSegemnts();
    ASSERT_EQ(m_objectRegion, m_calculatedSegmentsUnited);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         KmeansTest,
                         testing::ValuesIn(testParamsKmeans),
                         [](const testing::TestParamInfo<KmeansTestParams>& info) {
                             return info.param.m_id;
                         });

const std::vector<GridTestParams> testParamsGrid{
    GridTestParams{
        {
            .m_id = "3x3_small",

            .m_width  = 5,
            .m_height = 5,
            .m_object = ".OOO."
                        "OOOOO"
                        "OOOOO"
                        "OOOOO"
                        ".OOO.",

            .m_parts = ".001."
                       "00011"
                       "00011"
                       "22233"
                       ".223.",
        },
        3,
        3,
        0,
        true,
    },
    GridTestParams{
        {
            .m_id = "3x3_thresholded",

            .m_width  = 5,
            .m_height = 5,
            .m_object = ".OOO."
                        "OOOOO"
                        "OOOOO"
                        "OOOOO"
                        ".OOO.",

            .m_parts = ".001."
                       "00011"
                       "00011"
                       "222.."
                       ".22..",
        },
        3,
        3,
        5,
    },

    GridTestParams{
        {
            .m_id = "3x3_big",

            .m_width  = 18,
            .m_height = 10,
            .m_object = "...OOOOOOOOOO....."
                        "...OOOOOOOOOOO...."
                        "....OOOOOOOOOOO..."
                        "....OOOOOOOOOOOO.."
                        "....OOOOOOOOOOOOO."
                        "....OOOOOOOOOOOOO."
                        ".....OOOOOOOOOOOO."
                        ".....OOOOOOOOOOOO."
                        ".....OOOOOOOOO...."
                        ".....OOO..........",

            .m_parts = "...0001112223....."
                       "...00011122233...."
                       "....00111222333..."
                       "....445556667778.."
                       "....4455566677788."
                       "....4455566677788."
                       ".....9AAABBBCCCDD."
                       ".....9AAABBBCCCDD."
                       ".....9AAABBBCC...."
                       ".....EFF..........",

        },
        3,
        3,
        0,
        true,
    },
    GridTestParams{
        {
            .m_id = "4x4_big_thresholded",

            .m_width  = 18,
            .m_height = 10,
            .m_object = "...OOOOOOOOOO....."
                        "...OOOOOOOOOOO...."
                        "....OOOOOOOOOOO..."
                        "....OOOOOOOOOOOO.."
                        "....OOOOOOOOOOOOO."
                        "....OOOOOOOOOOOOO."
                        ".....OOOOOOOOOOOO."
                        ".....OOOOOOOOOOOO."
                        ".....OOOOOOOOO...."
                        ".....OOO..........",

            .m_parts = "...0000111122....."
                       "...00001111222...."
                       "....00011112222..."
                       "....00011112222..."
                       "....3334444555566."
                       "....3334444555566."
                       ".....334444555566."
                       ".....334444555566."
                       ".................."
                       "..................",
        },
        4,
        4,
        8,
    },
};

TEST_P(GridTest, BasicSegmentation)
{
    const auto& params = GetParam();
    this->prepare(params);

    m_calculatedSegments = m_objectRegion.splitByGrid(params.m_gridWidth, params.m_gridHeight, params.m_threshold);

    makeExpectedStr();

    ASSERT_EQ(m_calculatedSegmentsStr, m_expectedSegmentsStr);
    ASSERT_EQ(m_calculatedSegments, m_expectedSegments);

    if (params.m_uniteCheck) {
        uniteSegemnts();
        ASSERT_EQ(m_objectRegion, m_calculatedSegmentsUnited);
    }
}

INSTANTIATE_TEST_SUITE_P(InstantiationName,
                         GridTest,
                         testing::ValuesIn(testParamsGrid),
                         [](const testing::TestParamInfo<GridTestParams>& info) {
                             return info.param.m_id;
                         });

GTEST_TEST(GridDetection, Basic)
{
    /*
0000011111222.
0000011111222.
0000011111222.
3333344444555.
3333344444555.
3333344444555.
6666677777888.
6666677777888.
..............

..............
..0....1...2..
..............
..............
..3....4...5..
..............
..6....7......
..............
..............
*/
    MapTileContainer tileContainer;
    tileContainer.init(13, 8, 1);
    auto settings = MapTileRegionSegmentation::guessKMeansByGrid(tileContainer.m_all, 8);

    std::vector<FHPos> expected{ FHPos(2, 1), FHPos(7, 1), FHPos(11, 1), FHPos(2, 4), FHPos(7, 4), FHPos(11, 4), FHPos(2, 6), FHPos(7, 6) };
    std::vector<FHPos> actual;
    for (auto& item : settings.m_items)
        actual.push_back(item.m_initialCentroid->m_pos);

    EXPECT_EQ(expected, actual);

    std::vector<int> radiuses{ 36, 37, 41, 37, 37, 42, 30, 22 };
    for (size_t i = 0; i < settings.m_items.size(); i++)
        settings.m_items[i].m_areaHint = radiuses[i];

    ASSERT_NO_THROW(tileContainer.m_all.splitByKExt(settings));
}

GTEST_TEST(GridDetection, Basic2)
{
    MapTileContainer tileContainer;
    tileContainer.init(20, 18, 1);

    MergedRegion reg;
    reg.initFromTileContainer(&tileContainer, 0);

    const std::string object = ".........OO........."
                               ".OOOOOOOOOOO........"
                               "OOOOOOOOOOOOO......."
                               "OOOOOOOOOOOOOO......"
                               "OOOOOOOOOOOOOO......"
                               "OOOOOOOOOOOOOOO....."
                               "OOOOOOOOOOOOOOOO...."
                               "OOOOOOOOOOOOOOOOO..."
                               "OOOOOOOOOOOOOOOOOO.."
                               "OOOOOOOOOOOOOOOOOO.."
                               "OOOOOOOOOOOOOOOOO..."
                               "OOOOOOOOOOOOOOOOO..."
                               "...OOOOOOOOOOOOOO..."
                               ".....OOOOOOOOOOO...."
                               ".......OOOOOOOOO...."
                               ".........OOOOOO....."
                               "...........OOOO....."
                               "....................";

    reg.load(object);
    auto objectRegion = reg.m_regions['O'];

    auto settings = MapTileRegionSegmentation::guessKMeansByGrid(objectRegion, 14);

    std::vector<FHPos> actual;
    for (auto& item : settings.m_items)
        actual.push_back(item.m_initialCentroid->m_pos);

    std::vector<int> radiuses{ 37, 37, 37, 42, 42, 42, 41, 36, 37, 36, 42, 42, 46, 22 };
    for (size_t i = 0; i < settings.m_items.size(); i++)
        settings.m_items[i].m_areaHint = radiuses[i];

    ASSERT_NO_THROW(objectRegion.splitByKExt(settings));

    settings.m_items[1].m_initialCentroid = settings.m_items[0].m_initialCentroid;

    ASSERT_NO_THROW(objectRegion.splitByKExt(settings));
}
