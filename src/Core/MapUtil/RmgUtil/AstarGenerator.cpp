/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AstarGenerator.hpp"

#include "TemplateUtils.hpp"

namespace FreeHeroes {

AstarGenerator::Node::Node(FHPos pos, AstarGenerator::Node* parent)
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

AstarGenerator::CoordinateList AstarGenerator::findPath()
{
    m_success = false;

    std::shared_ptr<Node> current;
    NodeSet               openSet, closedSet;
    openSet.reserve(100);
    closedSet.reserve(100);
    openSet.push_back(std::make_shared<Node>(m_source));

    while (!openSet.empty()) {
        auto current_it = openSet.begin();
        current         = *current_it;

        for (auto it = openSet.begin(); it != openSet.end(); it++) {
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

        closedSet.push_back(current);
        openSet.erase(current_it);

        for (uint64_t i = 0; i < m_directions.size(); ++i) {
            FHPos newCoordinates(current->m_pos + m_directions[i]);
            if (!m_nonCollision.contains(newCoordinates) || findNodeOnList(closedSet, newCoordinates)) {
                continue;
            }

            uint64_t totalCost = current->m_G + ((i < 4) ? 10 : 14);

            Node* successorRaw = findNodeOnList(openSet, newCoordinates);
            if (successorRaw == nullptr) {
                auto successor = std::make_shared<Node>(newCoordinates, current.get());
                successor->m_G = totalCost;
                successor->m_H = posDistance(successor->m_pos, m_target) * 10;
                openSet.push_back(successor);
            } else if (totalCost < successorRaw->m_G) {
                successorRaw->m_parent = current.get();
                successorRaw->m_G      = totalCost;
            }
        }
    }

    CoordinateList path;
    Node*          currentRaw = current.get();
    while (currentRaw != nullptr) {
        path.push_back(currentRaw->m_pos);
        currentRaw = currentRaw->m_parent;
    }

    return path;
}

AstarGenerator::Node* AstarGenerator::findNodeOnList(NodeSet& nodes_, FHPos coordinates_)
{
    for (auto&& node : nodes_) {
        if (node->m_pos == coordinates_) {
            return node.get();
        }
    }
    return nullptr;
}

}
