/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

#include "H3MMap.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Pos,
    m_x,
    m_y,
    m_z)

ENUM_REFLECTION_STRINGIFY(
    PlayerInfo::AiTactic,
    NONE,
    NONE,
    RANDOM,
    WARRIOR,
    BUILDER,
    EXPLORER)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    SHeroName,
    m_heroId,
    m_heroName

)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    DisposedHero,
    m_heroId,
    m_portrait,
    m_name,
    m_players)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    PlayerInfo,
    m_canHumanPlay,
    m_canComputerPlay,
    m_aiTactic,
    m_allowedFactionsBitmask,
    m_isFactionRandom,
    m_hasRandomHero,
    m_mainCustomHeroPortrait,
    m_mainCustomHeroName,
    m_mainCustomHeroId,
    m_hasMainTown,
    m_generateHeroAtMainTown,
    m_posOfMainTown,
    m_team,
    m_generatedHeroTownFaction,
    m_unused1,
    m_placeholder,
    m_heroesNames)

ENUM_REFLECTION_STRINGIFY(
    MapFormat,
    Invalid,
    Invalid,
    ROE,
    AB,
    SOD,
    HC,
    HOTA1,
    HOTA2,
    HOTA3,
    WOG,
    VCMI)

ENUM_REFLECTION_STRINGIFY(
    H3Map::VictoryConditionType,
    WINSTANDARD,
    ARTIFACT,
    GATHERTROOP,
    GATHERRESOURCE,
    BUILDCITY,
    BUILDGRAIL,
    BEATHERO,
    CAPTURECITY,
    BEATMONSTER,
    TAKEDWELLINGS,
    TAKEMINES,
    TRANSPORTITEM,
    DEFEATALL,
    SURVIVETIME,
    WINSTANDARD)

ENUM_REFLECTION_STRINGIFY(
    H3Map::LossConditionType,
    LOSSSTANDARD,
    LOSSCASTLE,
    LOSSHERO,
    TIMEEXPIRES,
    LOSSSTANDARD)

ENUM_REFLECTION_STRINGIFY(
    ObjectTemplate::Type,
    INVALID,
    INVALID,
    COMMON,
    CREATURE,
    HERO,
    ARTIFACT,
    RESOURCE)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapTileH3M,
    m_roadDir,
    m_roadType,
    m_riverDir,
    m_riverType,
    m_terView,
    m_terType,
    m_flipHor,
    m_flipVert,
    m_riverFlipHor,
    m_riverFlipVert,
    m_roadFlipHor,
    m_roadFlipVert,
    m_coastal,
    m_unused)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapTileSet,
    m_size,
    m_hasUnderground,
    m_tiles)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    ObjectTemplate,
    m_animationFile,
    m_visitMask,
    m_blockMask,
    m_terrainsHard,
    m_terrainsSoft,
    m_id,
    m_subid,
    m_type,
    m_drawPriority)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    GlobalMapEvent,
    m_name,
    m_message,
    m_resourceSet,
    m_players,
    m_humanAffected,
    m_computerAffected,
    m_firstOccurence,
    m_nextOccurence)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    CustomHeroData::SecSkill,
    m_id,
    m_level)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    CustomHeroData,
    m_enabled,
    m_hasExp,
    m_exp,
    m_hasSkills,
    m_skills,
    m_artSet,
    m_hasCustomBio,
    m_bio,
    m_sex,
    m_spellSet,
    m_primSkillSet)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    CustomHeroDataExt,
    m_unknown1,
    m_unknown2)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Map::HotaVersion,
    m_ver1,
    m_ver2,
    m_ver3,
    m_unknown1,
    m_unknown2,
    m_allowSpecialWeeks,
    m_roundLimit,
    m_unknown3,
    m_unknown4)
STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Map::Rumor,
    m_name,
    m_text)
STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Map::VictoryCondition,
    m_type,
    m_allowNormalVictory,
    m_appliesToAI,
    m_artID,
    m_creatureID,
    m_creatureCount,
    m_resourceID,
    m_resourceAmount,
    m_pos,
    m_hallLevel,
    m_castleLevel,
    m_days)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Map::LossCondition,
    m_type,
    m_pos,
    m_days)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Map,
    m_format,
    m_hotaVer,
    m_anyPlayers,
    m_mapName,
    m_mapDescr,
    m_difficulty,
    m_levelLimit,

    m_players,
    m_victoryCondition,
    m_lossCondition,
    m_teamCount,
    m_allowedHeroes,
    m_placeholderHeroes,
    m_disposedHeroes,
    m_allowedArtifacts,
    m_allowedSpells,
    m_allowedSecSkills,

    m_rumors,
    m_customHeroData,
    m_customHeroDataExt,
    m_tiles,
    m_objectDefs,

    m_globalEvents);
}
