/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AstarGenerator.hpp"

#include "TemplateUtils.hpp"

namespace FreeHeroes {

AstarGenerator::Node::Node(MapTilePtr pos, AstarGenerator::Node* parent)
{
    m_parent = parent;
    m_pos    = pos;
}

uint64_t AstarGenerator::Node::getScore()
{
    return m_G + m_H;
}

AstarGenerator::AstarGenerator()
{
    m_directions = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 }, { -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }
    };
}

MapTilePtrList AstarGenerator::findPath()
{
    m_success = false;

    std::shared_ptr<Node> current;
    NodeSet               openSet, closedSet;
    openSet.m_data.reserve(100);
    closedSet.m_data.reserve(100);
    openSet.add(std::make_shared<Node>(m_source));

    while (!openSet.m_data.empty()) {
        auto current_it = openSet.m_data.begin();
        current         = *current_it;

        for (auto it = openSet.m_data.begin(); it != openSet.m_data.end(); it++) {
            auto node = *it;
            if (node->getScore() <= current->getScore()) {
                current    = node;
                current_it = it;
            }
        }

        if (current->m_pos == m_target) {
            m_success = true;
            break;
        }

        closedSet.add(current);
        openSet.erase(current_it);

        for (uint64_t i = 0; i < m_directions.size(); ++i) {
            MapTilePtr newCoordinates(current->m_pos->neighbourByOffset(m_directions[i]));
            if (!newCoordinates || !m_nonCollision.contains(newCoordinates) || findNodeOnList(closedSet, newCoordinates)) {
                continue;
            }

            uint64_t totalCost = current->m_G + ((i < 4) ? 10 : 14);

            Node* successorRaw = findNodeOnList(openSet, newCoordinates);
            if (successorRaw == nullptr) {
                auto successor = std::make_shared<Node>(newCoordinates, current.get());
                successor->m_G = totalCost;
                successor->m_H = posDistance(successor->m_pos->m_pos, m_target->m_pos) * 10;
                openSet.add(successor);
            } else if (totalCost < successorRaw->m_G) {
                successorRaw->m_parent = current.get();
                successorRaw->m_G      = totalCost;
            }
        }
    }

    MapTilePtrList path;
    Node*          currentRaw = current.get();
    while (currentRaw != nullptr) {
        path.push_back(currentRaw->m_pos);
        currentRaw = currentRaw->m_parent;
    }

    return path;
}

AstarGenerator::Node* AstarGenerator::findNodeOnList(NodeSet& nodes, MapTilePtr coordinates)
{
    if (!nodes.m_used.contains(coordinates))
        return nullptr;

    for (auto&& node : nodes.m_data) {
        if (node->m_pos == coordinates)
            return node.get();
    }
    return nullptr;
}

}
