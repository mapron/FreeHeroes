/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "RoadHelper.hpp"
#include "AstarGenerator.hpp"
#include "../FHMap.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>

namespace FreeHeroes {

RoadHelper::RoadHelper(FHMap&                        map,
                       MapTileContainer&             tileContainer,
                       Core::IRandomGenerator* const rng,
                       std::ostream&                 logOutput,
                       bool                          extraLogging)
    : m_map(map)
    , m_tileContainer(tileContainer)
    , m_rng(rng)
    , m_logOutput(logOutput)
    , m_extraLogging(extraLogging)
{
    (void) m_extraLogging;
}

void RoadHelper::placeRoads(TileZone& tileZone)
{
    Mernel::ProfilerScope topScope("placeRoads");

    if (tileZone.m_nodes.m_all.size() <= 1)
        return;

    MapTileRegion           roadRegion;
    std::vector<MapTilePtr> connected;
    auto                    connectCell = [&connected, &tileZone, &roadRegion, this](MapTilePtr cell, RoadLevel level) {
        Mernel::ProfilerScope   scope2("connectCell");
        std::vector<MapTilePtr> connectedTmp = connected;
        const size_t            partialSize  = std::min(size_t(4), connectedTmp.size());

        std::partial_sort(connectedTmp.begin(), connectedTmp.begin() + partialSize, connectedTmp.end(), [cell, &tileZone](MapTilePtr l, MapTilePtr r) {
            const RoadLevel roadLevelL  = tileZone.m_nodes.getLevel(l);
            const RoadLevel roadLevelR  = tileZone.m_nodes.getLevel(r);
            const int64_t   lBorderMult = static_cast<int>(roadLevelL) + 1;
            const int64_t   rBorderMult = static_cast<int>(roadLevelR) + 1;
            const auto      lDistance   = posDistance(cell->m_pos, l->m_pos, lBorderMult);
            const auto      rDistance   = posDistance(cell->m_pos, r->m_pos, rBorderMult);

            return std::tuple{ lDistance, l } < std::tuple{ rDistance, r }; // make sure we have no equal! partial_sort is unstable!
        });
        if (connectedTmp.size() > partialSize)
            connectedTmp.resize(partialSize);

        std::vector<MapTilePtrList> pathAlternatives;
        for (MapTilePtr closest : connectedTmp) {
            auto path = aStarPath(tileZone, cell, closest);
            if (!path.empty())
                pathAlternatives.push_back(std::move(path));
        }
        if (pathAlternatives.empty())
            throw std::runtime_error("Failed to connect node!");

        auto pathIt = std::min_element(pathAlternatives.begin(), pathAlternatives.end(), [](const MapTilePtrList& l, const MapTilePtrList& r) {
            const auto lDistance = l.size();
            const auto rDistance = r.size();
            return std::tuple{ lDistance, l[0] } < std::tuple{ rDistance, r[0] };
        });
        auto path   = *pathIt;

        connected.push_back(cell);
        roadRegion.insert(path);
        if (level == RoadLevel::Towns) {
            tileZone.m_midTownNodes.insert(path[path.size() / 2]);
        }
        if (level == RoadLevel::Exits) {
            tileZone.m_midExitNodes.insert(path[path.size() / 2]);
        }

        auto newLevel = tileZone.m_roadTypes[level];
        for (auto* rcell : path) {
            auto oldLevel = tileZone.m_roads.getLevel(rcell);
            if (oldLevel < newLevel)
                tileZone.m_roads.add(rcell, newLevel);
        }
    };
    for (const auto& [level, unconnectedRoadNodes] : tileZone.m_nodes.m_byLevel) {
        if (unconnectedRoadNodes.empty())
            continue;
        auto unconnected = unconnectedRoadNodes;
        if (connected.empty()) {
            auto cell = unconnected[0];
            connected.push_back(cell);
            roadRegion.insert(cell);
        }
        unconnected.erase(roadRegion);
        while (!unconnected.empty()) {
            auto       pathIt = std::min_element(unconnected.begin(), unconnected.end(), [&connected](const MapTilePtr& l, const MapTilePtr& r) {
                int64_t lDistance = 0;
                int64_t rDistance = 0;

                for (MapTilePtr c : connected) {
                    lDistance += posDistance(c, l, 100);
                    rDistance += posDistance(c, r, 100);
                }
                return std::tuple{ lDistance, l } < std::tuple{ rDistance, r };
            });
            MapTilePtr cell   = *pathIt;
            unconnected.erase(cell);
            connectCell(cell, level);
            unconnected.erase(roadRegion);
        }
    }

    // redundant road cleanup
    while (redundantCleanup(tileZone)) {
    }

    // correct intersections;
    {
        MapTileRegion copy = tileZone.m_roads.getCombinedRegion(FHRoadType::None);
        for (auto* cell : copy) {
            size_t cobbleCount = 0;
            size_t gravelCount = 0;
            for (auto* ncell : cell->m_orthogonalNeighbours) {
                auto nlevel = tileZone.m_roads.getLevel(ncell);
                if (nlevel == FHRoadType::Cobblestone)
                    cobbleCount++;
                if (nlevel == FHRoadType::Gravel)
                    gravelCount++;
            }
            if (cobbleCount >= 2)
                tileZone.m_roads.add(cell, FHRoadType::Cobblestone);
            else if (gravelCount >= 2)
                tileZone.m_roads.add(cell, FHRoadType::Gravel);
        }
    }
}

bool RoadHelper::redundantCleanup(TileZone& tileZone)
{
    bool                  result = false;
    Mernel::ProfilerScope scope("redundantCleanup");

    // check that every tile in tiles argument is
    // mustBeRoad=true  - contains  in pendingRegion
    // mustBeRoad=false - not exist in pendingRegion
    auto checkTileListAllOf = [&tileZone](const MapTilePtrList& tiles, bool mustBeRoad) {
        for (auto* cell : tiles) {
            const bool isRoad = tileZone.m_roads.getLevel(cell) > FHRoadType::None;
            if (mustBeRoad && !isRoad)
                return false;
            if (!mustBeRoad && isRoad)
                return false;
        }
        return true;
    };

    auto applyCorrectionPattern = [&tileZone, &checkTileListAllOf, &result](const std::vector<FHPos>&              offsetsCheckRoad,
                                                                            const std::vector<FHPos>&              offsetsCheckNonRoad,
                                                                            const std::vector<MapTile::Transform>& transforms) {
        MapTileRegion copy = tileZone.m_roads.getCombinedRegion(FHRoadType::None);
        copy.erase(tileZone.m_roadIgnoredNodes);
        for (auto* cell : copy) {
            for (const MapTile::Transform& transform : transforms) {
                const MapTilePtrList tilesCheckRoad    = cell->neighboursByOffsets(offsetsCheckRoad, transform);
                const MapTilePtrList tilesCheckNonRoad = cell->neighboursByOffsets(offsetsCheckNonRoad, transform);

                if (checkTileListAllOf(tilesCheckRoad, true) && checkTileListAllOf(tilesCheckNonRoad, false)) {
                    tileZone.m_roads.erase(cell);
                    result = true;
                }
            }
        }
    };

    {
        // Detect pattern, R - road, . - non-road, remove central road C
        // R R R
        // R C .
        // R . .
        const std::vector<FHPos> offsetsCheckRoad{
            { -1, -1 }, // TL
            { +0, -1 }, // T
            { +1, -1 }, // TR
            { -1, +0 }, // L
            { -1, +1 }, // BL
        };
        const std::vector<FHPos> offsetsCheckNonRoad{
            { +1, +0 }, // R
            { +0, +1 }, // B
            { +1, +1 }, // BR
        };
        const std::vector<MapTile::Transform> transforms{
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = true },
        };
        applyCorrectionPattern(offsetsCheckRoad, offsetsCheckNonRoad, transforms);
    }

    {
        // Detect pattern, R - road, . - non-road, X - anything = remove central road C
        // R R X
        // R C .
        // R R X
        const std::vector<FHPos> offsetsCheckRoad{
            { -1, -1 }, // TL
            { +0, -1 }, // T
            { -1, +0 }, // L
            { -1, +1 }, // BL
            { +0, +1 }, // B
        };
        const std::vector<FHPos> offsetsCheckNonRoad{
            //{ +1, -1 }, // TR
            { +1, +0 }, // R
            //{ +1, +1 }, // BR
        };
        const std::vector<MapTile::Transform> transforms{
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = true, .m_flipVert = true },
        };
        applyCorrectionPattern(offsetsCheckRoad, offsetsCheckNonRoad, transforms);
    }

    {
        // Detect pattern, R - road, . - non-road, remove central road C
        // R R .
        // R C .
        // R . .
        const std::vector<FHPos> offsetsCheckRoad{
            { -1, -1 }, // TL
            { +0, -1 }, // T
            { -1, +0 }, // L
            { -1, +1 }, // BL
        };
        const std::vector<FHPos> offsetsCheckNonRoad{
            { +1, -1 }, // TR
            { +1, +0 }, // R
            { +0, +1 }, // B
            { +1, +1 }, // BR
        };
        const std::vector<MapTile::Transform> transforms{
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = true },

            MapTile::Transform{ .m_transpose = true, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = true, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = false, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = true, .m_flipVert = true },
        };
        applyCorrectionPattern(offsetsCheckRoad, offsetsCheckNonRoad, transforms);
    }

    {
        // Detect pattern, R - road, . - non-road, remove central road C
        // R . .
        // R C .
        // R . .
        const std::vector<FHPos> offsetsCheckRoad{
            { -1, -1 }, // TL
            { -1, +0 }, // L
            { -1, +1 }, // BL
        };
        const std::vector<FHPos> offsetsCheckNonRoad{
            { +0, -1 }, // T
            { +1, -1 }, // TR
            { +1, +0 }, // R
            { +0, +1 }, // B
            { +1, +1 }, // BR
        };
        const std::vector<MapTile::Transform> transforms{
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = true, .m_flipVert = true },
        };
        applyCorrectionPattern(offsetsCheckRoad, offsetsCheckNonRoad, transforms);
    }

    {
        // Detect pattern, R - road, . - non-road, X - anything = remove central road C
        // . . R X
        // . C R R
        // . . . X
        const std::vector<FHPos> offsetsCheckRoad{
            { +1, -1 }, // TR
            { +1, +0 }, // R
            { +2, +0 }, // R2
        };
        const std::vector<FHPos> offsetsCheckNonRoad{
            { -1, -1 }, // TL
            { +0, -1 }, // T
            { -1, +0 }, // L
            { -1, +1 }, // BL
            { +0, +1 }, // B
            { +1, +1 }, // BR
        };
        const std::vector<MapTile::Transform> transforms{
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = false, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = false, .m_flipHor = true, .m_flipVert = true },

            MapTile::Transform{ .m_transpose = true, .m_flipHor = false, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = true, .m_flipVert = false },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = false, .m_flipVert = true },
            MapTile::Transform{ .m_transpose = true, .m_flipHor = true, .m_flipVert = true },
        };
        applyCorrectionPattern(offsetsCheckRoad, offsetsCheckNonRoad, transforms);
    }

    return result;
}

MapTilePtrList RoadHelper::aStarPath(TileZone& zone, MapTilePtr start, MapTilePtr end) const
{
    Mernel::ProfilerScope scope("aStarPath");

    AstarGenerator generator;
    generator.setPoints(start, end);

    generator.setNonCollision(zone.m_roadPotentialArea);

    auto path = generator.findPath();
    if (!generator.isSuccess()) {
        throw std::runtime_error("Failed to find valid path in " + zone.m_id + ", from:" + start->toPrintableString() + " to:" + end->toPrintableString());
    }
    //    if (std::find(path.cbegin(), path.cend(), start) == path.cend())
    //        throw std::runtime_error("no start in path!");
    //    if (std::find(path.cbegin(), path.cend(), end) == path.cend())
    //        throw std::runtime_error("no end in path!");

    const auto pathCopy = path;
    path.clear();
    path.push_back(pathCopy[0]);
    for (size_t i = 1; i < pathCopy.size(); i++) {
        MapTilePtr prev  = pathCopy[i - 1];
        MapTilePtr cur   = pathCopy[i];
        const int  prevX = prev->m_pos.m_x;
        const int  prevY = prev->m_pos.m_y;
        const int  curX  = cur->m_pos.m_x;
        const int  curY  = cur->m_pos.m_y;
        if (prevX != curX && prevY != curY) // diagonal
        {
            MapTilePtr extra1 = prev;
            MapTilePtr extra2 = prev;

            // from TL to BR
            if (prevX < curX && prevY < curY) {
                extra1 = prev->m_neighborB;
                extra2 = prev->m_neighborR;
            }

            // from BL to TR
            if (prevX < curX && prevY > curY) {
                extra1 = prev->m_neighborT;
                extra2 = prev->m_neighborR;
            }

            // from TR to BL
            if (prevX > curX && prevY < curY) {
                extra1 = prev->m_neighborB;
                extra2 = prev->m_neighborL;
            }

            // from BR to TL
            if (prevX > curX && prevY > curY) {
                extra1 = prev->m_neighborT;
                extra2 = prev->m_neighborL;
            }

            if (zone.m_roads.m_all.contains(extra1))
                path.push_back(extra1);
            else if (zone.m_roads.m_all.contains(extra2))
                path.push_back(extra2);
            else if (zone.m_roadPotentialArea.contains(extra1))
                path.push_back(extra1);
            else if (zone.m_roadPotentialArea.contains(extra2))
                path.push_back(extra2);
            else if (!zone.m_needPlaceObstacles.contains(extra1))
                path.push_back(extra1);
            else if (!zone.m_needPlaceObstacles.contains(extra2))
                path.push_back(extra2);
        }
        path.push_back(cur);
    }

    return path;
}

}
