/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMap.hpp"

#include "Reflection/EnumTraits.hpp"
#include "Reflection/MetaInfo.hpp"

#include "FHTileMapReflection.hpp"

namespace FreeHeroes::Core::Reflection {

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHPlayerId> = EnumTraits::make(
    FHPlayerId::Invalid,
    "Invalid"  , FHPlayerId::Invalid,
    "None"     , FHPlayerId::None,
    "Red"      , FHPlayerId::Red,
    "Blue"     , FHPlayerId::Blue,
    "Tan"      , FHPlayerId::Tan,
    "Green"    , FHPlayerId::Green,
    "Orange"   , FHPlayerId::Orange,
    "Purple"   , FHPlayerId::Purple,
    "Teal"     , FHPlayerId::Teal,
    "Pink"     , FHPlayerId::Pink
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<Core::GameVersion> = EnumTraits::make(
    Core::GameVersion::Invalid,
    "SOD"  , Core::GameVersion::SOD,
    "HOTA" , Core::GameVersion::HOTA
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHResource::Type> = EnumTraits::make(
    FHResource::Type::Resource,
    "Resource"      , FHResource::Type::Resource,
    "TreasureChest" , FHResource::Type::TreasureChest
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHRandomArtifact::Type> = EnumTraits::make(
    FHRandomArtifact::Type::Invalid,
    "Any"        , FHRandomArtifact::Type::Any,
    "Treasure"   , FHRandomArtifact::Type::Treasure,
    "Minor"      , FHRandomArtifact::Type::Minor,
    "Major"      , FHRandomArtifact::Type::Major,
    "Relic"      , FHRandomArtifact::Type::Relic
    );

// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHPlayer>{
    Field("ai", &FHPlayer::m_aiPossible),
    Field("human", &FHPlayer::m_humanPossible),
    Field("factions", &FHPlayer::m_startingFactions),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHHeroData>{
    Field("hasExp", &FHHeroData::m_hasExp),
    Field("hasSecSkills", &FHHeroData::m_hasSecSkills),
    Field("hasPrimSkills", &FHHeroData::m_hasPrimSkills),
    Field("hasCustomBio", &FHHeroData::m_hasCustomBio),
    Field("hasSpells", &FHHeroData::m_hasSpells),
    Field("army", &FHHeroData::m_army),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHHero>{
    Field("pos", &FHHero::m_pos),
    Field("order", &FHHero::m_order),
    Field("player", &FHHero::m_player),

    Field("main", &FHHero::m_isMain),
    Field("data", &FHHero::m_data),
    Field("questId", &FHHero::m_questIdentifier),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHTown>{
    Field("pos", &FHTown::m_pos),
    Field("order", &FHTown::m_order),
    Field("player", &FHTown::m_player),

    Field("main", &FHTown::m_isMain),
    Field("factionId", &FHTown::m_factionId),
    Field("hasFort", &FHTown::m_hasFort),
    Field("questId", &FHTown::m_questIdentifier),
    Field("spellResearch", &FHTown::m_spellResearch),
    Field("defFile", &FHTown::m_defFile),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHZone::Rect>{
    Field("pos", &FHZone::Rect::m_pos),
    Field("w", &FHZone::Rect::m_width),
    Field("h", &FHZone::Rect::m_height),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHResource>{
    Field("pos", &FHResource::m_pos),
    Field("order", &FHResource::m_order),

    Field("amount", &FHResource::m_amount),
    Field("id", &FHResource::m_id),
    Field("type", &FHResource::m_type),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHArtifact>{
    Field("pos", &FHArtifact::m_pos),
    Field("order", &FHArtifact::m_order),

    Field("id", &FHArtifact::m_id),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHRandomArtifact>{
    Field("pos", &FHRandomArtifact::m_pos),
    Field("order", &FHRandomArtifact::m_order),

    Field("type", &FHRandomArtifact::m_type),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMonster>{
    Field("pos", &FHMonster::m_pos),
    Field("order", &FHMonster::m_order),

    Field("id", &FHMonster::m_id),
    Field("count", &FHMonster::m_count),
    Field("agressionMin", &FHMonster::m_agressionMin),
    Field("agressionMax", &FHMonster::m_agressionMax),
    Field("questIdentifier", &FHMonster::m_questIdentifier),

};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHDwelling>{
    Field("pos", &FHDwelling::m_pos),
    Field("order", &FHDwelling::m_order),
    Field("player", &FHDwelling::m_player),

    Field("id", &FHDwelling::m_id),
    Field("defVariant", &FHDwelling::m_defVariant),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHBank>{
    Field("pos", &FHBank::m_pos),
    Field("order", &FHBank::m_order),

    Field("id", &FHBank::m_id),
    Field("defVariant", &FHBank::m_defVariant),
    Field("guardsVariants", &FHBank::m_guardsVariants),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHObstacle>{
    Field("pos", &FHObstacle::m_pos),
    Field("order", &FHObstacle::m_order),

    Field("id", &FHObstacle::m_id),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMap::Objects>{
    Field("resources", &FHMap::Objects::m_resources),
    Field("artifacts", &FHMap::Objects::m_artifacts),
    Field("artifactsRandom", &FHMap::Objects::m_artifactsRandom),
    Field("monsters", &FHMap::Objects::m_monsters),
    Field("dwellings", &FHMap::Objects::m_dwellings),
    Field("banks", &FHMap::Objects::m_banks),
    Field("obstacles", &FHMap::Objects::m_obstacles),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMap>{
    Field("version", &FHMap::m_version),
    Field("seed", &FHMap::m_seed),
    Field("tileMap", &FHMap::m_tileMap),
    Field("name", &FHMap::m_name),
    Field("descr", &FHMap::m_descr),
    Field("difficulty", &FHMap::m_difficulty),
    Field("players", &FHMap::m_players),
    Field("wanderingHeroes", &FHMap::m_wanderingHeroes),
    Field("towns", &FHMap::m_towns),
    Field("zones", &FHMap::m_zones),
    Field("objects", &FHMap::m_objects),
    Field("rivers", &FHMap::m_rivers),
    Field("roads", &FHMap::m_roads),
    Field("defaultTerrain", &FHMap::m_defaultTerrain),
    Field("disabledHeroes", &FHMap::m_disabledHeroes),
    Field("disabledArtifacts", &FHMap::m_disabledArtifacts),
    Field("disabledSpells", &FHMap::m_disabledSpells),
    Field("disabledSkills", &FHMap::m_disabledSkills),
    Field("customHeroes", &FHMap::m_customHeroes),
    Field("initialObjectDefs", &FHMap::m_initialObjectDefs),
};

template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::PlayersMap>{ true };

}
