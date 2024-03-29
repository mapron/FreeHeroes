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

        {
            for (auto it = openSet.m_data.begin(); it != openSet.m_data.end(); it++) {
                auto node = *it;
                if (node->getScore() <= current->getScore()) {
                    current    = node;
                    current_it = it;
                }
            }
        }

        if (current->m_pos == m_target) {
            m_success = true;
            break;
        }

        {
            closedSet.add(current);
            openSet.erase(current_it);
        }
        Node* currentRaw = current.get();

        auto applyCandidate = [this, currentRaw, &openSet, &closedSet](uint64_t cost, MapTilePtr newCoordinates) {
            if (!m_nonCollision.contains(newCoordinates) || closedSet.find(newCoordinates)) {
                return;
            }

            uint64_t totalCost = currentRaw->m_G + cost;

            Node* successorRaw = openSet.find(newCoordinates);
            if (successorRaw == nullptr) {
                auto successor = std::make_shared<Node>(newCoordinates, currentRaw);
                successor->m_G = totalCost;
                successor->m_H = posDistance(successor->m_pos->m_pos, m_target->m_pos) * 10;
                openSet.add(successor);
            } else if (totalCost < successorRaw->m_G) {
                successorRaw->m_parent = currentRaw;
                successorRaw->m_G      = totalCost;
            }
        };

        for (MapTilePtr newCoordinates : currentRaw->m_pos->m_orthogonalNeighbours) {
            applyCandidate(10, newCoordinates);
        }
        if (m_useDiag) {
            for (MapTilePtr newCoordinates : currentRaw->m_pos->m_diagNeighbours) {
                applyCandidate(14, newCoordinates);
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

}
