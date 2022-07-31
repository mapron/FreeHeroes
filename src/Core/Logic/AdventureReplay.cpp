/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureReplay.hpp"

#include "JsonRTTRDeserialize.hpp"
#include "JsonRTTRSerialize.hpp"

#include "IGameDatabase.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryIdResolver.hpp"
#include "PropertyTree.hpp"
#include "FileFormatJson.hpp"
#include "FileIOUtils.hpp"

namespace FreeHeroes::Core {

bool AdventureReplayData::load(const std_path& filename, IGameDatabase& gameDatabase)
{
    std::string buffer;
    if (!readFileIntoBuffer(filename, buffer))
        return false;
    PropertyTree main;
    if (!readJsonFromBuffer(buffer, main))
        return false;

    const PropertyTree&           jsonBattle  = main["bat"];
    const PropertyTree&           jsonRecords = jsonBattle["records"];
    Reflection::LibraryIdResolver idResolver(gameDatabase);
    if (jsonRecords.isList()) {
        for (const PropertyTree& jsonRecord : jsonRecords.getList()) {
            m_bat.m_records.push_back({});
            BattleReplayData::EventRecord& event = m_bat.m_records.back();
            Reflection::deserializeFromJson(idResolver, event, jsonRecord);
            if (event.type == BattleReplayData::EventRecord::Type::MoveAttack)
                assert(!event.moveParams.m_movePos.mainPos().isEmpty());
        }
    }
    const PropertyTree& jsonAdventure = main["adv"];
    m_adv.m_seed                      = jsonAdventure["seed"].getScalar().toInt();
    auto terrainId                    = jsonAdventure["terrain"].getScalar().toString();
    m_adv.m_terrain                   = gameDatabase.terrains()->find(terrainId);
    assert(m_adv.m_terrain);
    Reflection::deserializeFromJson(idResolver, m_adv.m_field, jsonAdventure["field"]);
    Reflection::deserializeFromJson(idResolver, m_adv.m_att, jsonAdventure["att"]);
    Reflection::deserializeFromJson(idResolver, m_adv.m_def, jsonAdventure["def"]);

    return true;
}

bool AdventureReplayData::save(const std_path& filename) const
{
    PropertyTree  main;
    PropertyTree& jsonBattle  = main["bat"];
    PropertyTree& jsonRecords = jsonBattle["records"];
    for (const auto& record : m_bat.m_records) {
        PropertyTree row = Reflection::serializeToJson(record);
        jsonRecords.append(std::move(row));
    }
    PropertyTree& jsonAdventure = main["adv"];
    jsonAdventure["seed"]       = PropertyTreeScalar(m_adv.m_seed);
    jsonAdventure["terrain"]    = PropertyTreeScalar(m_adv.m_terrain->id);
    jsonAdventure["field"]      = Reflection::serializeToJson(m_adv.m_field);
    jsonAdventure["att"]        = Reflection::serializeToJson(m_adv.m_att);
    jsonAdventure["def"]        = Reflection::serializeToJson(m_adv.m_def);

    std::string buffer;
    return writeJsonToBuffer(buffer, main) && writeFileFromBuffer(filename, buffer);
}

}
