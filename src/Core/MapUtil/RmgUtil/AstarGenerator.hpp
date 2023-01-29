/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "../FHTileMap.hpp"

namespace FreeHeroes {

class AstarGenerator {
public:
    using CoordinateList = std::vector<FHPos>;
    struct Node {
        uint64_t m_G = 0, m_H = 0;
        FHPos    m_pos;
        Node*    m_parent = nullptr;

        Node(FHPos pos, Node* parent = nullptr);
        uint64_t getScore();
    };

    using NodeSet = std::vector<std::shared_ptr<Node>>;

    bool  detectCollision(FHPos coordinates_);
    Node* findNodeOnList(NodeSet& nodes_, FHPos coordinates_);

public:
    AstarGenerator();
    void           setWorldSize(FHPos worldSize_);
    CoordinateList findPath(FHPos source_, FHPos target_);
    void           addCollision(FHPos coordinates_);
    void           removeCollision(FHPos coordinates_);
    void           clearCollisions();

private:
    CoordinateList m_directions, m_collisions;
    FHPos          m_worldSize;
};

}
