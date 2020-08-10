/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleFieldPathFinder.hpp"

#include "Profiler.hpp"

#include <algorithm>
#include <unordered_set>

#include <cassert>

namespace FreeHeroes::Core {

namespace {

bool pathFindDirectionOrder(const BattleDirection left, const BattleDirection right) {
    static const std::vector<BattleDirection> priorities {
        BattleDirection::BR, BattleDirection::R, BattleDirection::TR,
        BattleDirection::TL, BattleDirection::L, BattleDirection::BL,
    };
    auto leftIt  = std::find(priorities.cbegin(), priorities.cend(), left);
    auto rightIt = std::find(priorities.cbegin(), priorities.cend(), right);

    return leftIt < rightIt;
}

}

void BattleFieldPathFinder::floodFill(const BattlePosition start)
{
    //ProfilerScope scope("BF::floodFill");
    assert(field.isValid(start));

    matrix.fill(valEmpty);

    matrix.set(start, 0);

    Matrix remainPositions(matrix.width, matrix.height);
    remainPositions.fill(1);

    remainPositions.set(start, 0);
    if (!goThroughObstacles) {
        for (auto ob : obstacles)
            remainPositions.setChecked(ob, 0);
    }

    BattlePositionSet edgeTmp = field.getAdjacentSet(start);
    BattlePositionSet edge;
    for (auto pos : edgeTmp) {
        if (1 == remainPositions.get(pos))
            edge.insert(pos);
    }
    BattlePositionSet nextEdge;
    int step = 0;
    BattlePosition pos;
    while (!edge.empty() ) {
        ++step;
        nextEdge.clear();
        //ProfilerScope scope("outer");
        for (const auto edgePos : edge) {
            remainPositions.set(edgePos, 0);
        }
        for (const auto edgePos : edge) {
            //ProfilerScope scope("inner");

            matrix.set(edgePos, step);

            // all this stuff is ugly way to make it as fast as possible in debug mode.
            // sets, unordered sets - all too slow when testing AI (AI calling findPAth a lot).
            // please do not change anything without uncommenting ProfilerScope :)
            if (1 == remainPositions.getChecked(pos = field.neighbour(edgePos, BattleDirection::TR)) ) nextEdge.insert(pos);
            if (1 == remainPositions.getChecked(pos = field.neighbour(edgePos, BattleDirection::R )) ) nextEdge.insert(pos);
            if (1 == remainPositions.getChecked(pos = field.neighbour(edgePos, BattleDirection::BR)) ) nextEdge.insert(pos);
            if (1 == remainPositions.getChecked(pos = field.neighbour(edgePos, BattleDirection::TL)) ) nextEdge.insert(pos);
            if (1 == remainPositions.getChecked(pos = field.neighbour(edgePos, BattleDirection::L )) ) nextEdge.insert(pos);
            if (1 == remainPositions.getChecked(pos = field.neighbour(edgePos, BattleDirection::BL)) ) nextEdge.insert(pos);
        }
        edge = nextEdge;
    }
}

BattlePositionPath BattleFieldPathFinder::fromStartTo(const BattlePosition end, int limit) const
{
    //ProfilerScope scope("BF::fromStartTo");
    BattlePositionPath result;
    assert(field.isValid(end));
    const int endVal = matrix.get(end);
    if (endVal < 0 || obstacles.contains(end) || (limit != -1 && endVal > limit))
        return result;

    BattlePosition current = end;
    result.push_back(current);
    BattlePositionSet visited;
    BattleFieldGeometry::AdjacentMap edge;
    while (true) {
        edge = field.getAdjacent(current);
        for (auto i = edge.begin(), last = edge.end(); i != last; ) {
            if (visited.count(i->second) > 0 || (!goThroughObstacles && obstacles.contains(i->second)))
                i = edge.erase(i);
             else
                ++i;
        }
        if (edge.empty())
            break;

        auto minIt = std::min_element(edge.cbegin(), edge.cend(), [this](const auto left, const auto right) {
             const int valLeft  = matrix.get(left.second);
             const int valRight = matrix.get(right.second);
             if (valLeft < 0 && valRight < 0)
                 return false;
             if (valLeft < 0)
                 return false;
             if (valRight < 0)
                 return true;
             if (valRight != valLeft)
                 return valLeft < valRight;
             // now we have equal gradient values. let's decide specific walk order for that depending on direction.
             return pathFindDirectionOrder(left.first, right.first);
        });
        current = minIt->second;
        result.push_back(current);
        visited.insert(current);
        if (matrix.get(current) == 0)
            break;
    }
    std::reverse(result.begin(), result.end());
    result.erase(result.begin()); // start position is usually excess.

    return result;
}

BattlePositionSet BattleFieldPathFinder::findAvailable(int limit) const
{
    BattlePositionSet result;
    if (limit == 0)
        return result;

    for (int w = 0; w < field.width; ++w) {
        for (int h = 0; h < field.height; ++h) {
            int distance = matrix.get({w, h});
            if (distance <= 0)
                continue;
            if (limit != -1 && distance > limit)
                continue;
            if (obstacles.contains({w,h}))
                continue;

            result.insert({w, h});
        }
    }
    return result;
}

BattlePositionDistanceMap BattleFieldPathFinder::findDistances(int limit) const
{
    BattlePositionDistanceMap result;
    if (limit == 0)
        return result;
    for (int w = 0; w < field.width; ++w) {
        for (int h = 0; h < field.height; ++h) {
            int distance = matrix.get({w, h});
            if (distance <= 0)
                continue;
            if (limit != -1 && distance > limit)
                continue;
            if (obstacles.contains({w,h}))
                continue;

            result[{w, h}] = distance;
        }
    }
    return result;
}

}
