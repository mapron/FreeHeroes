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
#include <unordered_set>

#include "MapTileArea.hpp"

namespace FreeHeroes {

class AstarGenerator {
public:
    struct Node {
        uint64_t   m_G = 0, m_H = 0;
        MapTilePtr m_pos    = nullptr;
        Node*      m_parent = nullptr;

        Node(MapTilePtr pos, Node* parent = nullptr);
        uint64_t getScore();
    };

    struct NodeSet {
        using Vec = std::vector<std::shared_ptr<Node>>;
        Vec                            m_data;
        std::unordered_set<MapTilePtr> m_used;

        void add(std::shared_ptr<Node> node)
        {
            m_data.push_back(node);
            m_used.insert(node->m_pos);
        }
        void erase(const Vec::iterator& it)
        {
            m_used.erase(it->get()->m_pos);
            m_data.erase(it);
        }
    };

    Node* findNodeOnList(NodeSet& nodes, MapTilePtr coordinates);

public:
    AstarGenerator();
    void setPoints(MapTilePtr source, MapTilePtr target)
    {
        m_source = source;
        m_target = target;
    }
    MapTilePtrList findPath();

    bool isSuccess() const { return m_success; }

    void setNonCollision(MapTileRegion nonCollision) { m_nonCollision = std::move(nonCollision); }

private:
    std::vector<FHPos> m_directions;
    MapTileRegion      m_nonCollision;

    MapTilePtr m_source = nullptr;
    MapTilePtr m_target = nullptr;
    bool       m_success{ false };
};

}
