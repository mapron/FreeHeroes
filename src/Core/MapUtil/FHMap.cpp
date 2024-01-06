/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "FHMap.hpp"

#include "GameDatabasePropertyReader.hpp"
#include "GameDatabasePropertyWriter.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

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

void FHMap::derandomize(Core::IRandomGenerator* rng)
{
    std::vector<Core::LibraryFactionConstPtr> factionsTown;
    std::set<Core::LibraryFactionConstPtr>    factionsDwelling;
    {
        auto& factions = m_database->factions()->records();
        for (auto* faction : factions) {
            if (faction->alignment == Core::LibraryFaction::Alignment::Special)
                continue;
            factionsDwelling.insert(faction);
            if (faction->alignment == Core::LibraryFaction::Alignment::Independent)
                continue;
            factionsTown.push_back(faction);
        }
    }
    {
        auto allRes = m_database->resources()->records();

        for (auto& obj : m_objects.m_resourcesRandom) {
            auto*      rndRes = allRes[rng->gen(allRes.size() - 1)];
            FHResource res;
            const bool isRare = rndRes->rarity == Core::LibraryResource::Rarity::Rare;
            res.m_amount      = rng->genDispersed(isRare ? 4 : 6, 2) * rndRes->pileSize;
            res.m_pos         = obj.m_pos;
            res.m_id          = rndRes;

            m_objects.m_resources.push_back(res);
        }
        m_objects.m_resourcesRandom.clear();
    }
    {
        std::map<FHRandomArtifact::Type, std::vector<Core::LibraryArtifactConstPtr>> arts;
        for (auto* art : m_database->artifacts()->records()) {
            if (art->treasureClass == Core::LibraryArtifact::TreasureClass::Treasure) {
                arts[FHRandomArtifact::Type::Treasure].push_back(art);
                arts[FHRandomArtifact::Type::Any].push_back(art);
            }
            if (art->treasureClass == Core::LibraryArtifact::TreasureClass::Minor) {
                arts[FHRandomArtifact::Type::Minor].push_back(art);
                arts[FHRandomArtifact::Type::Any].push_back(art);
            }
            if (art->treasureClass == Core::LibraryArtifact::TreasureClass::Major) {
                arts[FHRandomArtifact::Type::Major].push_back(art);
                arts[FHRandomArtifact::Type::Any].push_back(art);
            }
            if (art->treasureClass == Core::LibraryArtifact::TreasureClass::Relic) {
                arts[FHRandomArtifact::Type::Relic].push_back(art);
                arts[FHRandomArtifact::Type::Any].push_back(art);
            }
        }
        for (auto& obj : m_objects.m_artifactsRandom) {
            auto&      artstc = arts.at(obj.m_type);
            auto*      rndArt = artstc[rng->gen(artstc.size() - 1)];
            FHArtifact art;
            art.m_pos = obj.m_pos;
            art.m_id  = rndArt;
            m_objects.m_artifacts.push_back(art);
        }
        m_objects.m_artifactsRandom.clear();
    }
    {
        std::map<int, std::vector<Core::LibraryUnitConstPtr>> units;
        for (auto* unit : m_database->units()->records()) {
            if (!factionsDwelling.contains(unit->faction))
                continue;
            units[unit->level / 10].push_back(unit);
            units[0].push_back(unit);
        }

        std::vector<FHMonster> keepMonsters;
        std::vector<FHMonster> extraMonsters;
        for (auto& obj : m_objects.m_monsters) {
            if (obj.m_randomLevel < 0) {
                keepMonsters.push_back(obj);
                continue;
            }
            auto&     unitsl  = units.at(obj.m_randomLevel);
            auto*     rndUnit = unitsl[rng->gen(unitsl.size() - 1)];
            FHMonster monster;
            monster.m_pos   = obj.m_pos;
            monster.m_id    = rndUnit;
            monster.m_count = 1;
            extraMonsters.push_back(monster);
        }
        m_objects.m_monsters.clear();
        m_objects.m_monsters.insert(m_objects.m_monsters.end(), keepMonsters.cbegin(), keepMonsters.cend());
        m_objects.m_monsters.insert(m_objects.m_monsters.end(), extraMonsters.cbegin(), extraMonsters.cend());
    }
    {
        std::vector<FHTown> keepTowns;
        std::vector<FHTown> extraTowns;
        for (auto& obj : m_towns) {
            if (!obj.m_randomTown) {
                keepTowns.push_back(obj);
                continue;
            }
            auto*  rndFaction = factionsTown[rng->gen(factionsTown.size() - 1)];
            FHTown town;
            town.m_pos       = obj.m_pos;
            town.m_factionId = rndFaction;
            town.m_hasFort   = obj.m_hasFort;
            town.m_player    = obj.m_player;
            extraTowns.push_back(town);
        }
        m_towns.clear();
        m_towns.insert(m_towns.end(), keepTowns.cbegin(), keepTowns.cend());
        m_towns.insert(m_towns.end(), extraTowns.cbegin(), extraTowns.cend());
    }
    {
        std::map<int, std::vector<Core::LibraryDwellingConstPtr>> dwellings;
        for (auto* dwell : m_database->dwellings()->records()) {
            if (dwell->creatureIds.size() != 1)
                continue;
            if (!factionsDwelling.contains(dwell->creatureIds[0]->faction))
                continue;
            dwellings[dwell->creatureIds[0]->level / 10].push_back(dwell);
            dwellings[0].push_back(dwell);
        }
        for (auto& obj : m_objects.m_randomDwellings) {
            int level = obj.m_hasLevel ? rng->genMinMax(obj.m_minLevel, obj.m_maxLevel) + 1 : 0;
            // @todo: faction
            auto&      dwellingsl = dwellings.at(level);
            auto*      rndDw      = dwellingsl[rng->gen(dwellingsl.size() - 1)];
            FHDwelling dwelling;
            dwelling.m_pos    = obj.m_pos;
            dwelling.m_id     = rndDw;
            dwelling.m_player = obj.m_player;
            m_objects.m_dwellings.push_back(dwelling);
        }
        m_objects.m_randomDwellings.clear();
    }
}

}
