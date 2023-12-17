#include "MapTileRegionSegmentation.hpp"

#include "MapTile.hpp"
#include "MapTileContainer.hpp"

#include "BonusRatio.hpp"

#include <cassert>
#include <iostream>

namespace FreeHeroes {

struct KMeansData {
    struct Cluster {
        KMeansSegmentationSettings::Item m_settings;

        FHPos m_extraMassPoint;

        FHPos                m_centroid;
        FHPos                m_centerMass;
        MapTilePtrSortedList m_points;
        size_t               m_pointsCount    = 0;
        int64_t              m_radiusPromille = 0;

        std::string toPrintableStringPoints() const
        {
            if (m_points.size() > 5)
                return std::to_string(m_pointsCount);
            std::string result;
            for (auto* tile : m_points)
                result += tile->toPrintableString() + ", ";
            return "[ " + result + "]";
        }

        std::string toPrintableString() const
        {
            return "{centroid: " + m_centroid.toPrintableString() + ","
                   + " mass: " + m_centerMass.toPrintableString() + ","
                   + " points: " + toPrintableStringPoints() + "}";
        }

        int64_t distanceTo(MapTilePtr point) const
        {
            const auto dx = int64_t(m_centroid.m_x - point->m_pos.m_x) * 1000; // max 20bit
            const auto dy = int64_t(m_centroid.m_y - point->m_pos.m_y) * 1000;

            // prevent branching as much as possible
            const auto arg = dx * dx + dy * dy; // max 40bit

            const int64_t linearDistancePromille  = intSqrt(arg);
            const int64_t distanceToCircumference = linearDistancePromille - m_radiusPromille;
            const bool    isInside                = (distanceToCircumference <= 0);
            const bool    isOutside               = !isInside;
            const int64_t innerDistance           = isInside * linearDistancePromille + isOutside * m_radiusPromille;
            const int64_t outerDistance           = isOutside * distanceToCircumference;

            const int64_t innerWeighted = innerDistance * m_settings.m_insideWeight;
            const int64_t outerWeighted = outerDistance * m_settings.m_outsideWeight;

            return innerWeighted + outerWeighted;
        }

        void updateCentroid()
        {
            m_centroid = m_centerMass;
        }

        void clearMass()
        {
            m_centerMass.m_x = m_settings.m_extraMassWeight * m_extraMassPoint.m_x;
            m_centerMass.m_y = m_settings.m_extraMassWeight * m_extraMassPoint.m_y;
            m_pointsCount    = m_settings.m_extraMassWeight;
            m_points.clear();
        }
        void addToMass(FHPos pos)
        {
            m_centerMass = m_centerMass + pos;
            m_pointsCount++;
        }
        void finalizeMass()
        {
            if (m_pointsCount) {
                m_centerMass.m_x /= m_pointsCount;
                m_centerMass.m_y /= m_pointsCount;
            } else {
                throw std::runtime_error("no points");
            }
        }
    };

    void clearMass()
    {
        for (auto& cluster : m_clusters)
            cluster.clearMass();
    }
    void assignPoints(bool isDone)
    {
        for (size_t i = 0; MapTilePtr tile : *m_region) {
            const size_t clusterId = m_nearestIndex[i];
            m_clusters[clusterId].addToMass(tile->m_pos);
            if (isDone) {
                m_clusters[clusterId].m_points.push_back(tile);
            }
            i++;
        }
    }
    void finalizeMass()
    {
        for (auto& cluster : m_clusters)
            cluster.finalizeMass();
    }
    void updateCentroids()
    {
        for (auto& cluster : m_clusters) {
            cluster.updateCentroid();
        }
    }

    void checkCentroids()
    {
        bool            hasIntersections = false;
        std::set<FHPos> used;
        for (auto& cluster : m_clusters) {
            if (used.contains(cluster.m_centroid)) {
                hasIntersections = true;
                break;
            }
            if (!m_region->contains(m_container->m_tileIndex.at(cluster.m_centroid))) {
                hasIntersections = true;
                break;
            }

            used.insert(cluster.m_centroid);
        }
        if (hasIntersections) {
            used.clear();
            MapTileRegion region = *m_region;
            for (auto& cluster : m_clusters) {
                auto tile = region.findClosestPoint(cluster.m_centroid);
                region.erase(tile);
                cluster.m_centroid = tile->m_pos;
            }
        }
    }

    size_t getNearestClusterId(MapTilePtr point) const
    {
        auto   minDist          = m_clusters[0].distanceTo(point);
        size_t nearestClusterId = 0;

        for (size_t i = 1; i < m_clusters.size(); i++) {
            const auto dist = m_clusters[i].distanceTo(point);
            if (dist < minDist) {
                minDist          = dist;
                nearestClusterId = i;
            }
        }

        return nearestClusterId;
    }

    bool runIter(bool last)
    {
        bool done = true;

        // fix repeated centroids
        checkCentroids();

        {
            Mernel::ProfilerScope scope("getNearestClusterId");
            // Add all points to their nearest cluster
            for (size_t i = 0; MapTilePtr tile : *m_region) {
                size_t&      currentClusterId = m_nearestIndex[i];
                const size_t nearestClusterId = getNearestClusterId(tile);

                if (currentClusterId != nearestClusterId) {
                    currentClusterId = nearestClusterId;
                    done             = false;
                }
                i++;
            }
        }

        // clear all existing clusters
        clearMass();

        // reassign points to their new clusters
        assignPoints(done || last);

        // calculate new mass points
        finalizeMass();

        updateCentroids();

        return done;
    }

    const MapTileContainer* m_container = nullptr;
    const MapTileRegion*    m_region    = nullptr;
    std::vector<size_t>     m_nearestIndex;
    std::vector<Cluster>    m_clusters;
};

MapTileRegionList MapTileRegionSegmentation::splitByFloodFill(const MapTileRegion& region, bool useDiag, MapTilePtr hint)
{
    if (region.empty())
        return {};

    MapTileRegionList result;
    MapTilePtrList    currentBuffer;

    MapTileRegion  visited;
    MapTilePtrList currentEdge;
    auto           addToCurrent = [&currentBuffer, &visited, &currentEdge, &region](MapTilePtr cell) {
        if (visited.contains(cell))
            return;
        if (!region.contains(cell))
            return;
        visited.insert(cell);
        currentBuffer.push_back(cell);
        currentEdge.push_back(cell);
    };
    MapTileRegion remain = region;
    if (hint) {
        if (!remain.contains(hint))
            throw std::runtime_error("Invalid tile hint provided");
    }

    while (!remain.empty()) {
        MapTilePtr startCell = hint ? hint : *remain.begin();
        hint                 = nullptr;
        addToCurrent(startCell);

        while (!currentEdge.empty()) {
            MapTilePtrList nextEdge = std::move(currentEdge);

            for (MapTilePtr edgeCell : nextEdge) {
                const auto& neighbours = edgeCell->neighboursList(useDiag);
                for (auto growedCell : neighbours) {
                    addToCurrent(growedCell);
                }
            }
        }
        MapTileRegion current = MapTileRegion(std::move(currentBuffer));
        remain.erase(current);
        result.push_back(std::move(current));
    }

    return result;
}

MapTileRegionList MapTileRegionSegmentation::splitByMaxArea(const MapTileRegion& region, size_t maxArea, size_t iterLimit)
{
    MapTileRegionList result;
    size_t            zoneArea = region.size();
    if (!zoneArea)
        return result;

    const size_t k = (zoneArea + maxArea + 1) / maxArea;
    if (k == 1)
        return { region };

    KMeansSegmentationSettings settings;
    settings.m_items.resize(k);
    size_t s = region.size();
    for (size_t i = 0; i < k; ++i) {
        settings.m_items[i].m_initialCentroid = region[i * s / k];
        settings.m_items[i].m_areaHint        = zoneArea / k;
    }
    return splitByKExt(region, settings, iterLimit);
}

MapTileRegionList MapTileRegionSegmentation::splitByK(const MapTileRegion& region, size_t k, size_t iterLimit)
{
    size_t zoneArea = region.size();
    if (!zoneArea)
        return {};

    if (k == 1)
        return { region };

    KMeansSegmentationSettings settings;
    settings.m_items.resize(k);
    size_t s = region.size();
    for (size_t i = 0; i < k; ++i) {
        settings.m_items[i].m_initialCentroid = region[i * s / k];
        settings.m_items[i].m_areaHint        = zoneArea / k;
    }
    return splitByKExt(region, settings, iterLimit);
}

MapTileRegionList MapTileRegionSegmentation::splitByKExt(const MapTileRegion& region, const KMeansSegmentationSettings& settingsList, size_t iterLimit)
{
    if (region.empty())
        return {};

    if (settingsList.m_items.size() == 1)
        return { region };

    Mernel::ProfilerScope scope("K-Means seg.");

    KMeansData   kmeans;
    const size_t K = settingsList.m_items.size();
    kmeans.m_clusters.resize(K);
    kmeans.m_region    = &region;
    kmeans.m_container = region[0]->m_container;
    kmeans.m_nearestIndex.resize(region.size());
    for (size_t i = 0; i < K; i++) {
        auto& c       = kmeans.m_clusters[i];
        auto& setting = settingsList.m_items[i];
        c.m_settings  = setting;
        if (setting.m_extraMassPoint) {
            c.m_extraMassPoint = setting.m_extraMassPoint->m_pos;
            assert(setting.m_extraMassWeight);
        }
        assert(setting.m_initialCentroid);
        assert(setting.m_areaHint > 0);
        c.m_centroid       = setting.m_initialCentroid->m_pos;
        c.m_radiusPromille = getRadiusPromille(setting.m_areaHint);

        c.m_radiusPromille /= 2; // absolutely arbitrary number.

        c.m_points.reserve(region.size() / K);
    }

    for (size_t i = 0; i < kmeans.m_nearestIndex.size(); i++) {
        kmeans.m_nearestIndex[i] = size_t(-1);
    }

    for (size_t iter = 0; iter < iterLimit; ++iter) {
        //        std::cout << "clusters:\n";
        //        for (size_t i = 0; i < K; i++) {
        //            std::cout << "[" << i << "]=" << kmeans.m_clusters[i].toPrintableString() << "\n";
        //        }
        const bool last = iter == iterLimit - 1;
        try {
            if (kmeans.runIter(last))
                break;
        }
        catch (...) {
            if (1) {
                std::cout << "exceptionThrown, settings:\n";
                for (size_t i = 0; i < K; i++) {
                    std::cout << "[" << i << "]=" << settingsList.m_items[i].m_initialCentroid->m_pos.toPrintableString() << " A=" << settingsList.m_items[i].m_areaHint << " \n";
                }
                MergedRegion reg;
                reg.initFromTile(region[0]);
                reg.m_regions['O'] = region;
                std::cout << reg.dump();
            }

            throw;
        }
    }

    MapTileRegionList result;
    result.resize(K);
    for (size_t i = 0; i < K; i++) {
        result[i] = MapTileRegion(kmeans.m_clusters[i].m_points);
    }
    return result;
}

MapTileRegionSegmentation::Grid MapTileRegionSegmentation::splitByGrid(const MapTileRegion& region, int width, int height)
{
    if (region.empty())
        return {};
    const auto boundary = getBoundary(region);

    const size_t cellCountX = (boundary.m_width + width - 1) / width;
    const size_t cellCountY = (boundary.m_height + height - 1) / height;

    Grid regionGrid;
    regionGrid.resize(cellCountY);
    for (auto& row : regionGrid)
        row.resize(cellCountX);

    for (auto* tile : region) {
        const FHPos offset = tile->m_pos - boundary.m_topLeft;
        const int   gridX  = offset.m_x / width;
        const int   gridY  = offset.m_y / height;
        regionGrid[gridY][gridX].insert(tile);
    }

    return regionGrid;
}

MapTileRegionList MapTileRegionSegmentation::reduceGrid(Grid grid, size_t threshold)
{
    if (grid.empty())
        return {};

    MapTileRegionList result;
    for (auto& row : grid) {
        for (auto& region : row) {
            if (region.size() > 0 && region.size() >= threshold) {
                result.push_back(std::move(region));
            }
        }
    }
    return result;
}

KMeansSegmentationSettings MapTileRegionSegmentation::guessKMeansByGrid(const MapTileRegion& region, size_t k)
{
    if (region.empty() || !k)
        return {};

    const auto boundary = getBoundary(region);
    size_t     gridSide = static_cast<size_t>(intSqrt(int64_t(k)));
    if (gridSide * gridSide < k) // k is not perfect square
        gridSide++;

    size_t gridSideX = gridSide;
    size_t gridSideY = gridSide;

    auto calcCells = [&region, &boundary](int width, int height) {
        std::set<std::pair<int, int>> cellCoords;
        for (auto* tile : region) {
            const FHPos offset = tile->m_pos - boundary.m_topLeft;
            const int   gridX  = offset.m_x / width;
            const int   gridY  = offset.m_y / height;
            cellCoords.insert({ gridX, gridY });
        }
        return cellCoords.size();
    };

    int cellWidthX = (boundary.m_width + gridSide - 1) / gridSideX;
    int cellWidthY = (boundary.m_height + gridSide - 1) / gridSideY;

    size_t cellCount = calcCells(cellWidthX, cellWidthY);
    while (cellCount < k) {
        if (gridSideX == boundary.m_width && gridSideY == boundary.m_height) {
            throw std::runtime_error("Invalid logic in guessKMeansByGrid");
        }
        if (gridSideX < boundary.m_width)
            gridSideX++;
        if (gridSideY < boundary.m_height)
            gridSideY++;
        cellWidthX = (boundary.m_width + gridSide - 1) / gridSideX;
        cellWidthY = (boundary.m_height + gridSide - 1) / gridSideY;
        assert(cellWidthX > 0);
        assert(cellWidthY > 0);
        cellCount = calcCells(cellWidthX, cellWidthY);
    }

    auto grid        = splitByGrid(region, cellWidthX, cellWidthY);
    auto reducedGrid = reduceGrid(grid, 0);

    assert(reducedGrid.size() == cellCount);

    KMeansSegmentationSettings settings;
    settings.m_items.resize(k);

    for (size_t i = 0; i < k; ++i) {
        size_t gridIndex                      = i * reducedGrid.size() / k;
        settings.m_items[i].m_initialCentroid = reducedGrid[gridIndex].makeCentroid(true);
    }
    return settings;
}

MapTileRegionSegmentation::Rect MapTileRegionSegmentation::getBoundary(const MapTileRegion& region)
{
    if (region.empty())
        return {};

    auto firstTile = region[0];

    FHPos topLeft     = firstTile->m_pos;
    FHPos bottomRight = firstTile->m_pos;
    for (auto* tile : region) {
        topLeft.m_x = std::min(topLeft.m_x, tile->m_pos.m_x);
        topLeft.m_y = std::min(topLeft.m_y, tile->m_pos.m_y);

        bottomRight.m_x = std::max(bottomRight.m_x, tile->m_pos.m_x);
        bottomRight.m_y = std::max(bottomRight.m_y, tile->m_pos.m_y);
    }
    const size_t boundaryWidth  = 1 + bottomRight.m_x - topLeft.m_x;
    const size_t boundaryHeight = 1 + bottomRight.m_y - topLeft.m_y;
    return { topLeft, bottomRight, boundaryWidth, boundaryHeight };
}

int64_t MapTileRegionSegmentation::getRadiusPromille(int64_t area)
{
    const int64_t veryBadPi = 314;
    return intSqrt(area * 1'000'000 * 100 / veryBadPi);
}

int64_t MapTileRegionSegmentation::getArea(int64_t radiusPromille)
{
    const int64_t veryBadPi = 314;
    return radiusPromille * radiusPromille * veryBadPi / 100 / 1'000'000;
}

void MergedRegion::initFromTileContainer(const MapTileContainer* tileContainer, int z)
{
    m_topLeft = tileContainer->m_tileIndex.at(FHPos{ 0, 0, z });
    m_width   = tileContainer->m_width;
    m_height  = tileContainer->m_height;
}

void MergedRegion::initFromTile(MapTilePtr tile)
{
    initFromTileContainer(tile->m_container, tile->m_pos.m_z);
}

void MergedRegion::load(const std::string& serialized, char background)
{
    [[maybe_unused]] const size_t size = serialized.size();
    assert(m_height * m_width == (int) size);
    m_regions.clear();
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            FHPos offset{ x, y, 0 };
            auto* tile = m_topLeft->neighbourByOffset(offset);
            if (!tile)
                throw std::runtime_error("Invalid offset: " + offset.toPrintableString());
            const size_t testOffset = x + m_width * y;
            const char   c          = serialized.at(testOffset);
            if (c == background)
                continue;

            m_regions[c].insert(tile);
        }
    }
}

std::string MergedRegion::save(char background) const
{
    std::string serialized;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            auto* tile = m_topLeft->neighbourByOffset(FHPos{ x, y, 0 });
            if (!tile)
                throw std::runtime_error("Invalid offset: " + FHPos{ x, y, 0 }.toPrintableString());

            char c = background;
            for (const auto& [regc, region] : m_regions) {
                if (region.contains(tile)) {
                    if (c != background)
                        throw std::runtime_error("Regions have intersection!");
                    c = regc;
                }
            }
            serialized += c;
        }
    }
    return serialized;
}

std::string MergedRegion::dump(char background) const
{
    std::string result = save(background);
    return makePrintable(result, m_width);
}

std::string MergedRegion::makePrintable(const std::string& serialized, int width)
{
    std::string result;
    for (size_t i = 0; i < serialized.size(); i += width) {
        size_t len = i + width < serialized.size() ? width : serialized.size() - i;
        result += serialized.substr(i, len) + '\n';
    }
    return result;
}

CharMappedRegions MergedRegion::makeRegionsFromList(const MapTileRegionList& objects)
{
    if (objects.size() > 61) {
        throw std::runtime_error("Too many segments to pack to string");
    }
    CharMappedRegions result;
    for (size_t i = 0; i < objects.size(); ++i) {
        char c = 0;
        if (i >= 0 && i <= 9) {
            c = '0' + i;
        } else if (i >= 10 && i <= 35) {
            c = 'A' + (i - 10);
        } else if (i >= 36 && i <= 61) {
            c = 'a' + (i - 36);
        }
        result[c] = objects[i];
    }
    return result;
}

MapTileRegionList MergedRegion::makeListFromRegions(const CharMappedRegions& objects)
{
    MapTileRegionList result;
    result.resize(objects.size());
    for (const auto& [c, region] : objects) {
        size_t i = size_t(-1);
        if (c >= '0' && c <= '9') {
            i = c - '0';
        } else if (c >= 'A' && c <= 'Z') {
            i = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'z') {
            i = c - 'a' + 36;
        }
        result.at(i) = region;
    }
    return result;
}

}
