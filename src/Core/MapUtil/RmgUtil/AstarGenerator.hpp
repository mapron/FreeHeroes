/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <set>

#include "../FHPos.hpp"

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

    Node* findNodeOnList(NodeSet& nodes_, FHPos coordinates_);

public:
    AstarGenerator();
    void setPoints(FHPos source, FHPos target)
    {
        m_source = source;
        m_target = target;
    }
    CoordinateList findPath();

    bool isSuccess() const { return m_success; }

    void setNonCollision(std::set<FHPos> nonCollision) { m_nonCollision = std::move(nonCollision); }

private:
    CoordinateList  m_directions;
    std::set<FHPos> m_nonCollision;

    FHPos m_source;
    FHPos m_target;
    bool  m_success{ false };
};

}
