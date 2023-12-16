/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTileRegion.hpp"
#include "FHPos.hpp"

#include "MapUtilExport.hpp"

#include <map>

namespace FreeHeroes {

struct KMeansSegmentationSettings {
    struct Item {
        MapTilePtr m_initialCentroid = nullptr;
        int64_t    m_areaHint        = 100;

        int64_t m_outsideWeight = 3;
        int64_t m_insideWeight  = 2;

        MapTilePtr m_extraMassPoint  = nullptr;
        size_t     m_extraMassWeight = 0; // in tiles.
    };
    std::vector<Item> m_items;
};

struct MAPUTIL_EXPORT MapTileRegionSegmentation {
    using Grid = std::vector<MapTileRegionList>;

    struct Rect {
        FHPos  m_topLeft;
        FHPos  m_bottomRight;
        size_t m_width;
        size_t m_height;
    };

    static MapTileRegionList splitByFloodFill(const MapTileRegion& region, bool useDiag, MapTilePtr hint = nullptr);
    static MapTileRegionList splitByMaxArea(const MapTileRegion& region, size_t maxArea, size_t iterLimit = 100);
    static MapTileRegionList splitByK(const MapTileRegion& region, size_t k, size_t iterLimit = 100);
    static MapTileRegionList splitByKExt(const MapTileRegion& region, const KMeansSegmentationSettings& settingsList, size_t iterLimit = 100);
    static Grid              splitByGrid(const MapTileRegion& region, int width, int height);
    static MapTileRegionList reduceGrid(Grid grid, size_t threshold);

    static KMeansSegmentationSettings guessKMeansByGrid(const MapTileRegion& region, size_t k);
    static Rect                       getBoundary(const MapTileRegion& region);

    static int64_t getRadiusPromille(int64_t area);
    static int64_t getArea(int64_t radiusPromille);
};

using CharMappedRegions = std::map<char, MapTileRegion>;

class MapTileContainer;
// multiple regions occupying same rectangular area without intersection
struct MAPUTIL_EXPORT MergedRegion {
    CharMappedRegions m_regions;
    MapTilePtr        m_topLeft = nullptr;
    int               m_width   = 0;
    int               m_height  = 0;

    void initFromTileContainer(const MapTileContainer* tileContainer, int z);
    void initFromTile(MapTilePtr tile);

    void        load(const std::string& serialized, char background = '.');
    std::string save(char background = '.') const;
    std::string dump(char background = '.') const;

    static std::string       makePrintable(const std::string& serialized, int width);
    static CharMappedRegions makeRegionsFromList(const MapTileRegionList& objects);
    static MapTileRegionList makeListFromRegions(const CharMappedRegions& objects);

    void              setList(const MapTileRegionList& objects) { m_regions = makeRegionsFromList(objects); }
    MapTileRegionList getList() const { return makeListFromRegions(m_regions); }
};

}
