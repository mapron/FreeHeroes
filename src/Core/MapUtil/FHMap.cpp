/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "FHMap.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

#include "AdventureReflection.hpp"

#include "FHMapReflection.hpp"

#include "Logger.hpp"

namespace FreeHeroes {

void FHMap::toJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void FHMap::fromJson(const PropertyTree& data, const Core::IGameDatabase* database)
{
    Core::Reflection::PropertyTreeReader reader(database);
    *this = {};
    reader.jsonToValue(data, *this);
}

void FHMap::initTiles(const Core::IGameDatabase* database)
{
    m_tileMap.updateSize();
    auto fillZoneTerrain = [this](const FHZone& zone) {
        if (!zone.m_terrainId) {
            assert(0);
            return;
        }

        zone.placeOnMap(m_tileMap);
    };

    if (m_defaultTerrain) {
        for (int z = 0; z < m_tileMap.m_depth; ++z)
            fillZoneTerrain(FHZone{ .m_terrainId = m_defaultTerrain, .m_rect{ FHZone::Rect{ .m_pos{ 0, 0, z }, .m_width = m_tileMap.m_width, .m_height = m_tileMap.m_height } } });
    }
    for (auto& zone : m_zones)
        fillZoneTerrain(zone);
    for (auto& river : m_rivers)
        river.placeOnMap(m_tileMap);
    for (auto& road : m_roads)
        road.placeOnMap(m_tileMap);

    const auto* dirtTerrain  = database->terrains()->find(std::string(Core::LibraryTerrain::s_terrainDirt));
    const auto* sandTerrain  = database->terrains()->find(std::string(Core::LibraryTerrain::s_terrainSand));
    const auto* waterTerrain = database->terrains()->find(std::string(Core::LibraryTerrain::s_terrainWater));

    for (int z = 0; z < m_tileMap.m_depth; ++z) {
        for (int y = 0; y < m_tileMap.m_height; ++y) {
            for (int x = 0; x < m_tileMap.m_width; ++x) {
                auto& tile = m_tileMap.get(x, y, z);
                if (!tile.m_terrain)
                    Logger(Logger::Err) << "No terrain at: (" << x << "," << y << "," << z << ")";
            }
        }
    }

    m_tileMap.correctTerrainTypes(dirtTerrain, sandTerrain, waterTerrain);
    m_tileMap.correctRoads();
    m_tileMap.correctRivers();
}

}
