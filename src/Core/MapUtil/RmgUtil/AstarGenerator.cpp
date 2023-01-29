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

void AstarGenerator::setWorldSize(FHPos worldSize_)
{
    m_worldSize = worldSize_;
}

void AstarGenerator::addCollision(FHPos coordinates_)
{
    m_collisions.push_back(coordinates_);
}

void AstarGenerator::removeCollision(FHPos coordinates_)
{
    auto it = std::find(m_collisions.begin(), m_collisions.end(), coordinates_);
    if (it != m_collisions.end()) {
        m_collisions.erase(it);
    }
}

void AstarGenerator::clearCollisions()
{
    m_collisions.clear();
}

AstarGenerator::CoordinateList AstarGenerator::findPath(FHPos source, FHPos target)
{
    std::shared_ptr<Node> current;
    NodeSet               openSet, closedSet;
    openSet.reserve(100);
    closedSet.reserve(100);
    openSet.push_back(std::make_shared<Node>(source));

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

        if (current->m_pos == target) {
            break;
        }

        closedSet.push_back(current);
        openSet.erase(current_it);

        for (uint64_t i = 0; i < m_directions.size(); ++i) {
            FHPos newCoordinates(current->m_pos + m_directions[i]);
            if (detectCollision(newCoordinates) || findNodeOnList(closedSet, newCoordinates)) {
                continue;
            }

            uint64_t totalCost = current->m_G + ((i < 4) ? 10 : 14);

            Node* successorRaw = findNodeOnList(openSet, newCoordinates);
            if (successorRaw == nullptr) {
                auto successor = std::make_shared<Node>(newCoordinates, current.get());
                successor->m_G = totalCost;
                successor->m_H = posDistance(successor->m_pos, target) * 10;
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

bool AstarGenerator::detectCollision(FHPos coordinates_)
{
    if (coordinates_.m_x < 0 || coordinates_.m_x >= m_worldSize.m_x || coordinates_.m_y < 0 || coordinates_.m_y >= m_worldSize.m_y || std::find(m_collisions.begin(), m_collisions.end(), coordinates_) != m_collisions.end()) {
        return true;
    }
    return false;
}

}
