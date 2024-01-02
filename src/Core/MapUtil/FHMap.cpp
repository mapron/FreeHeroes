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

std::string FHHero::getDefId(bool onWater) const
{
    if (m_isPrison) {
        if (m_isCamp)
            return "avxhcamp";
        return onWater ? "avwtprsn" : "avxprsn0";
    } else if (m_isRandom) {
        return onWater ? "avxboat5" : "ahrandom";
    } else {
        auto* libraryHero = m_data.m_army.hero.library;
        return libraryHero->getAdventureSpriteForMap(onWater);
    }
}

void FHMap::toJson(PropertyTree& data) const
{
    Core::PropertyTreeWriterDatabase writer;
    writer.valueToJson(*this, data);
}

void FHMap::fromJson(PropertyTree data)
{
    Core::PropertyTreeReaderDatabase reader(m_database);

    *this = { .m_database = m_database };
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

void FHMap::applyRngUserSettings(const Mernel::PropertyTree& data)
{
    Core::PropertyTreeReaderDatabase reader(m_database);
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
