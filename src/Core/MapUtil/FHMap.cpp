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
    reader.jsonToValue(data, *this);

    m_tileMap.updateSize();
    m_packedTileMap.unpackToMap(m_tileMap);

    if (data.contains("template")) {
        auto&           zones = data["template"]["zones"];
        PropertyTreeMap baseItems;
        for (auto& [key, item] : zones.getMap()) {
            if (item.contains("isNormal"))
                continue;
            baseItems[key] = item;
        }
        for (auto& [key, item] : baseItems) {
            zones.getMap().erase(key);
        }
        for (auto& [key, item] : baseItems) {
            if (!item.contains("base"))
                continue;

            auto baseKey  = item["base"].getScalar().toString();
            auto baseItem = baseItems.at(baseKey);
            PropertyTree::mergePatch(baseItem, item);

            item = baseItem;
        }

        for (auto& [key, item] : zones.getMap()) {
            if (!item.contains("base"))
                continue;
            auto baseKey  = item["base"].getScalar().toString();
            auto baseItem = baseItems.at(baseKey);
            PropertyTree::mergePatch(baseItem, item);
            item = baseItem;
        }
        reader.jsonToValue(data["template"], m_template);
    }
}

void FHMap::applyRngUserSettings(const Mernel::PropertyTree& data, const Core::IGameDatabase* database)
{
    Core::PropertyTreeReaderDatabase reader(database);
    reader.jsonToValue(data, m_template.m_userSettings);
}

void FHMap::rescaleToUserSize()
{
    const int mapSize = m_template.m_userSettings.m_mapSize;
    m_template.rescaleToSize(mapSize, m_tileMap.m_width, m_tileMap.m_height);

    m_tileMap.m_width  = mapSize;
    m_tileMap.m_height = mapSize;
}

}
