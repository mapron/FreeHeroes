/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Reflection/EnumTraits.hpp"
#include "Reflection/MetaInfo.hpp"

#include "H3MMap.hpp"

namespace FreeHeroes::Core::Reflection {

template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Pos>{
    Field("x", &H3Pos::m_x),
    Field("y", &H3Pos::m_y),
    Field("z", &H3Pos::m_z),
};

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<AiTactic> = EnumTraits::make(
    AiTactic::NONE,
    "NONE", AiTactic::NONE,
    "RANDOM", AiTactic::RANDOM,
    "WARRIOR", AiTactic::WARRIOR,
    "BUILDER", AiTactic::BUILDER,
    "EXPLORER", AiTactic::EXPLORER
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<SHeroName>{
    Field("heroId", &SHeroName::m_heroId),
    Field("heroName", &SHeroName::m_heroName),

};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<DisposedHero>{
    Field("heroId", &DisposedHero::m_heroId),
    Field("portrait", &DisposedHero::m_portrait),
    Field("name", &DisposedHero::m_name),
    Field("players", &DisposedHero::m_players),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<PlayerInfo>{
    Field("canHumanPlay", &PlayerInfo::m_canHumanPlay),
    Field("canComputerPlay", &PlayerInfo::m_canComputerPlay),
    Field("aiTactic", &PlayerInfo::m_aiTactic),
    Field("allowedFactionsBitmask", &PlayerInfo::m_allowedFactionsBitmask),
    Field("isFactionRandom", &PlayerInfo::m_isFactionRandom),
    Field("hasRandomHero", &PlayerInfo::m_hasRandomHero),
    Field("mainCustomHeroPortrait", &PlayerInfo::m_mainCustomHeroPortrait),
    Field("mainCustomHeroName", &PlayerInfo::m_mainCustomHeroName),
    Field("mainCustomHeroId", &PlayerInfo::m_mainCustomHeroId),
    Field("hasMainTown", &PlayerInfo::m_hasMainTown),
    Field("generateHeroAtMainTown", &PlayerInfo::m_generateHeroAtMainTown),
    Field("posOfMainTown", &PlayerInfo::m_posOfMainTown),
    Field("team", &PlayerInfo::m_team),
    Field("generateHero", &PlayerInfo::m_generateHero),
    Field("p7", &PlayerInfo::m_p7),
    Field("powerPlaceholders", &PlayerInfo::m_powerPlaceholders),
    Field("heroesNames", &PlayerInfo::m_heroesNames),

};

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<MapFormat> = EnumTraits::make(
    MapFormat::Invalid,
    "INVALID"  , MapFormat::Invalid,
    "ROE"      , MapFormat::ROE,
    "AB"       , MapFormat::AB,
    "SOD"      , MapFormat::SOD,
    "HOTA1"    , MapFormat::HOTA1,
    "HOTA2"    , MapFormat::HOTA2,
    "HOTA3"    , MapFormat::HOTA3,
    "WOG"      , MapFormat::WOG,
    "VCMI"     , MapFormat::VCMI
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<H3Map::VictoryConditionType> = EnumTraits::make(
    H3Map::VictoryConditionType::WINSTANDARD,
    "ARTIFACT"       , H3Map::VictoryConditionType::ARTIFACT,
    "GATHERTROOP"    , H3Map::VictoryConditionType::GATHERTROOP,
    "GATHERRESOURCE" , H3Map::VictoryConditionType::GATHERRESOURCE,
    "BUILDCITY"      , H3Map::VictoryConditionType::BUILDCITY,
    "BUILDGRAIL"     , H3Map::VictoryConditionType::BUILDGRAIL,
    "BEATHERO"       , H3Map::VictoryConditionType::BEATHERO,
    "CAPTURECITY"    , H3Map::VictoryConditionType::CAPTURECITY,
    "BEATMONSTER"    , H3Map::VictoryConditionType::BEATMONSTER,
    "TAKEDWELLINGS"  , H3Map::VictoryConditionType::TAKEDWELLINGS,
    "TAKEMINES"      , H3Map::VictoryConditionType::TAKEMINES,
    "TRANSPORTITEM"  , H3Map::VictoryConditionType::TRANSPORTITEM,
    "WINSTANDARD"    , H3Map::VictoryConditionType::WINSTANDARD
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<H3Map::LossConditionType> = EnumTraits::make(
    H3Map::LossConditionType::LOSSSTANDARD,
    "LOSSCASTLE"   , H3Map::LossConditionType::LOSSCASTLE,
    "LOSSHERO"     , H3Map::LossConditionType::LOSSHERO,
    "TIMEEXPIRES"  , H3Map::LossConditionType::TIMEEXPIRES,
    "LOSSSTANDARD" , H3Map::LossConditionType::LOSSSTANDARD
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<ObjectTemplate::Type> = EnumTraits::make(
    ObjectTemplate::Type::INVALID,
    "INVALID"  , ObjectTemplate::Type::INVALID,
    "COMMON"   , ObjectTemplate::Type::COMMON,
    "CREATURE" , ObjectTemplate::Type::CREATURE,
    "HERO"     , ObjectTemplate::Type::HERO,
    "ARTIFACT" , ObjectTemplate::Type::ARTIFACT,
    "RESOURCE" , ObjectTemplate::Type::RESOURCE
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTile>{
    Field("ex", &MapTile::m_extTileFlags),
    Field("rdd", &MapTile::m_roadDir),
    Field("rdt", &MapTile::m_roadType),
    Field("rid", &MapTile::m_riverDir),
    Field("rit", &MapTile::m_riverType),
    Field("v", &MapTile::m_terView),
    Field("t", &MapTile::m_terType),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTileSet>{
    Field("size", &MapTileSet::m_size),
    Field("hasUnderground", &MapTileSet::m_hasUnderground),
    Field("tiles", &MapTileSet::m_tiles),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<ObjectTemplate>{
    Field("animationFile", &ObjectTemplate::m_animationFile),
    Field("visitMask", &ObjectTemplate::m_visitMask),
    Field("blockMask", &ObjectTemplate::m_blockMask),
    Field("terrainsHard", &ObjectTemplate::m_terrainsHard),
    Field("terrainsSoft", &ObjectTemplate::m_terrainsSoft),
    Field("id", &ObjectTemplate::m_id),
    Field("subid", &ObjectTemplate::m_subid),
    Field("type", &ObjectTemplate::m_type),
    Field("drawPriority", &ObjectTemplate::m_drawPriority),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<GlobalMapEvent>{
    Field("name", &GlobalMapEvent::m_name),
    Field("message", &GlobalMapEvent::m_message),
    Field("resourceSet", &GlobalMapEvent::m_resourceSet),
    Field("players", &GlobalMapEvent::m_players),
    Field("humanAffected", &GlobalMapEvent::m_humanAffected),
    Field("computerAffected", &GlobalMapEvent::m_computerAffected),
    Field("firstOccurence", &GlobalMapEvent::m_firstOccurence),
    Field("nextOccurence", &GlobalMapEvent::m_nextOccurence),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<CustomHeroData::SecSkill>{
    Field("id", &CustomHeroData::SecSkill::m_id),
    Field("l", &CustomHeroData::SecSkill::m_level),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<CustomHeroData>{
    Field("enabled", &CustomHeroData::m_enabled),
    Field("hasExp", &CustomHeroData::m_hasExp),
    Field("exp", &CustomHeroData::m_exp),
    Field("hasSkills", &CustomHeroData::m_hasSkills),
    Field("skills", &CustomHeroData::m_skills),
    Field("arts", &CustomHeroData::m_artSet),
    Field("hasCustomBio", &CustomHeroData::m_hasCustomBio),
    Field("bio", &CustomHeroData::m_bio),
    Field("sex", &CustomHeroData::m_sex),
    Field("spells", &CustomHeroData::m_spellSet),
    Field("prim", &CustomHeroData::m_primSkillSet),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Map::HotaVersion>{
    Field("v1", &H3Map::HotaVersion::m_ver1),
    Field("v2", &H3Map::HotaVersion::m_ver2),
    Field("v3", &H3Map::HotaVersion::m_ver3),
    Field("fixed", &H3Map::HotaVersion::m_fixedMap),
    Field("roundLimit", &H3Map::HotaVersion::m_roundLimit),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Map::Rumor>{
    Field("name", &H3Map::Rumor::m_name),
    Field("text", &H3Map::Rumor::m_text),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Map::VictoryCondition>{
    Field("type", &H3Map::VictoryCondition::m_type),
    Field("allowNormalVictory", &H3Map::VictoryCondition::m_allowNormalVictory),
    Field("appliesToAI", &H3Map::VictoryCondition::m_appliesToAI),
    Field("artID", &H3Map::VictoryCondition::m_artID),
    Field("creatureID", &H3Map::VictoryCondition::m_creatureID),
    Field("creatureCount", &H3Map::VictoryCondition::m_creatureCount),
    Field("resourceID", &H3Map::VictoryCondition::m_resourceID),
    Field("resourceAmount", &H3Map::VictoryCondition::m_resourceAmount),
    Field("pos", &H3Map::VictoryCondition::m_pos),
    Field("hallLevel", &H3Map::VictoryCondition::m_hallLevel),
    Field("castleLevel", &H3Map::VictoryCondition::m_castleLevel),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Map::LossCondition>{
    Field("type", &H3Map::LossCondition::m_type),
    Field("pos", &H3Map::LossCondition::m_pos),
    Field("days", &H3Map::LossCondition::m_daysPassed),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<H3Map>{

    Field("format", &H3Map::m_format),
    Field("hotaVer", &H3Map::m_hotaVer),
    Field("anyPlayers", &H3Map::m_anyPlayers),
    Field("mapName", &H3Map::m_mapName),
    Field("mapDescr", &H3Map::m_mapDescr),
    Field("difficulty", &H3Map::m_difficulty),
    Field("levelLimit", &H3Map::m_levelLimit),

    Field("players", &H3Map::m_players),
    Field("victoryCondition", &H3Map::m_victoryCondition),
    Field("lossCondition", &H3Map::m_lossCondition),
    Field("teamCount", &H3Map::m_teamCount),
    Field("teamSettings", &H3Map::m_teamSettings),
    Field("allowedHeroes", &H3Map::m_allowedHeroes),
    Field("placeholderHeroes", &H3Map::m_placeholderHeroes),
    Field("disposedHeroes", &H3Map::m_disposedHeroes),
    Field("allowedArtifacts", &H3Map::m_allowedArtifacts),
    Field("allowedSpells", &H3Map::m_allowedSpells),
    Field("allowedSecSkills", &H3Map::m_allowedSecSkills),

    Field("rumors", &H3Map::m_rumors),
    Field("customHeroData", &H3Map::m_customHeroData),
    Field("tiles", &H3Map::m_tiles),
    Field("objectDefs", &H3Map::m_objectDefs),

    Field("events", &H3Map::m_events),
};

}
