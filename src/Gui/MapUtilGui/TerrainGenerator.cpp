/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "TerrainGenerator.hpp"

#include <set>

namespace FreeHeroes {

void generateTerrainPlane(TerrainPlane& plane)
{
    struct MapRegion {
        std::set<TerrainPlanePoint> points;
    };

    for (int y = 0; y < plane.height(); y++)
        for (int x = 0; x < plane.width(); ++x) {
            plane.set(x, y, -1);
        }

    const int                      area        = plane.width() * plane.height();
    const int                      regionCount = 5;
    const int                      regionArea  = area / regionCount;
    std::vector<TerrainPlanePoint> startPoints{ TerrainPlanePoint{ 0, 0 },
                                                TerrainPlanePoint{ plane.width() / 2, plane.height() / 2 },
                                                TerrainPlanePoint{ plane.width() * 2 / 3, 2 },
                                                TerrainPlanePoint{ 2, plane.height() * 2 / 3 },
                                                TerrainPlanePoint{ plane.width() - 2, plane.height() - 2 } };

    auto growRegion = [&plane](TerrainPlanePoint start, int remainArea, int value) {
        MapRegion edge;
        edge.points.insert(start);
        plane.set(start, value);

        MapRegion nextEdge;

        while (!edge.points.empty()) {
            //ProfilerScope scope("outer");
            for (const auto edgePos : edge.points) {
                //ProfilerScope scope("inner");
                if (remainArea <= 0)
                    break;

                plane.set(edgePos, value);
                remainArea--;
            }
            if (remainArea <= 0)
                break;

            nextEdge.points.clear();
            for (const auto edgePos : edge.points) {
                for (int dx = -1; dx <= +1; ++dx) {
                    for (int dy = -1; dy <= +1; ++dy) {
                        if (dx == 0 && dy == 0)
                            continue;
                        if (dx == -1 && edgePos.x == 0)
                            continue;
                        if (dy == -1 && edgePos.y == 0)
                            continue;
                        if (dx == +1 && edgePos.x == plane.width() - 1)
                            continue;
                        if (dy == +1 && edgePos.y == plane.height() - 1)
                            continue;
                        const TerrainPlanePoint newPos{ edgePos.x + dx, edgePos.y + dy };
                        if (plane.get(newPos) != -1)
                            continue;

                        nextEdge.points.insert(newPos);
                    }
                }
            }
            edge = nextEdge;
        }
    };
    for (int i = 0; i < regionCount; ++i) {
        growRegion(startPoints[i], regionArea + 1, i);
    }
}

}
