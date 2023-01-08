/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "FHMap.hpp"

#include "GameDatabasePropertyReader.hpp"
#include "GameDatabasePropertyWriter.hpp"

#include "IGameDatabase.hpp"

#include "AdventureReflection.hpp"

#include "FHMapReflection.hpp"

#include "MernelPlatform/Logger.hpp"

namespace FreeHeroes {
using namespace Mernel;

void FHMap::toJson(PropertyTree& data) const
{
    Core::PropertyTreeWriterDatabase writer;
    writer.valueToJson(*this, data);
}

void FHMap::fromJson(PropertyTree data, const Core::IGameDatabase* database)
{
    Core::PropertyTreeReaderDatabase reader(database);
    *this = {};
    if (data.contains("rngZones")) {
        PropertyTreeMap baseItems;
        for (auto& [key, item] : data["rngZones"].getMap()) {
            if (item.contains("isNormal"))
                continue;
            baseItems[key] = item;
        }
        for (auto& [key, item] : baseItems) {
            data["rngZones"].getMap().erase(key);
        }
        for (auto& [key, item] : data["rngZones"].getMap()) {
            if (!item.contains("base"))
                continue;
            auto baseKey = item["base"].getScalar().toString();
            PropertyTree::mergePatch(item, baseItems.at(baseKey));
        }
    }
    reader.jsonToValue(data, *this);
}

void FHMap::applyRngUserSettings(const Mernel::PropertyTree& data, const Core::IGameDatabase* database)
{
    Core::PropertyTreeReaderDatabase reader(database);
    reader.jsonToValue(data, m_rngUserSettings);
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

    if (m_tileMapUpdateRequired) {
        m_tileMap.correctTerrainTypes(dirtTerrain, sandTerrain, waterTerrain);
        m_tileMap.correctRoads();
        m_tileMap.correctRivers();
    }
}

void FHMap::rescaleToSize(int mapSize)
{
    int wmult = mapSize;
    int wdiv  = m_tileMap.m_width;

    int hmult = mapSize;
    int hdiv  = m_tileMap.m_height;

    m_tileMap.m_width  = mapSize;
    m_tileMap.m_height = mapSize;

    for (auto& [key, rngZone] : m_rngZones) {
        rngZone.m_centerAvg.m_x = rngZone.m_centerAvg.m_x * wmult / wdiv;
        rngZone.m_centerAvg.m_y = rngZone.m_centerAvg.m_y * hmult / hdiv;

        rngZone.m_centerDispersion.m_x = rngZone.m_centerDispersion.m_x * wmult / wdiv;
        rngZone.m_centerDispersion.m_y = rngZone.m_centerDispersion.m_y * hmult / hdiv;
    }
}

}
