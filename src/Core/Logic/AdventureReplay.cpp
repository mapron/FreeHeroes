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

#include <json.hpp>

#include <fstream>

namespace FreeHeroes::Core {
using namespace nlohmann;

bool AdventureReplayData::load(const std_path& filename, IGameDatabase& gameDatabase)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return false;
    json main;
    ifs >> main;

    const json&                   jsonBattle  = main["bat"];
    const json&                   jsonRecords = jsonBattle["records"];
    Reflection::LibraryIdResolver idResolver(gameDatabase);
    for (const json& jsonRecord : jsonRecords) {
        m_bat.m_records.push_back({});
        BattleReplayData::EventRecord& event = m_bat.m_records.back();
        Reflection::deserializeFromJson(idResolver, event, jsonRecord);
        if (event.type == BattleReplayData::EventRecord::Type::MoveAttack)
            assert(!event.moveParams.m_movePos.mainPos().isEmpty());
    }
    const json& jsonAdventure = main["adv"];
    m_adv.m_seed              = jsonAdventure["seed"];
    auto terrainId            = static_cast<std::string>(jsonAdventure["terrain"]);
    m_adv.m_terrain           = gameDatabase.terrains()->find(terrainId);
    assert(m_adv.m_terrain);
    Reflection::deserializeFromJson(idResolver, m_adv.m_field, jsonAdventure["field"]);
    Reflection::deserializeFromJson(idResolver, m_adv.m_att, jsonAdventure["att"]);
    Reflection::deserializeFromJson(idResolver, m_adv.m_def, jsonAdventure["def"]);

    return true;
}

bool AdventureReplayData::save(const std_path& filename) const
{
    std::ofstream ofs(filename);
    if (!ofs)
        return false;

    json  main;
    json& jsonBattle  = main["bat"];
    json& jsonRecords = jsonBattle["records"];
    for (const auto& record : m_bat.m_records) {
        json row = Reflection::serializeToJson(record);
        jsonRecords.push_back(std::move(row));
    }
    json& jsonAdventure      = main["adv"];
    jsonAdventure["seed"]    = m_adv.m_seed;
    jsonAdventure["terrain"] = m_adv.m_terrain->id;
    jsonAdventure["field"]   = Reflection::serializeToJson(m_adv.m_field);
    jsonAdventure["att"]     = Reflection::serializeToJson(m_adv.m_att);
    jsonAdventure["def"]     = Reflection::serializeToJson(m_adv.m_def);
    ofs << main;
    return true;
}

}
