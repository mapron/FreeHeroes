/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMap.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

#include "FHTileMapReflection.hpp"

namespace Mernel::Reflection {

ENUM_REFLECTION_STRINGIFY(
    FHPlayerId,
    Invalid,
    Invalid,
    None,
    Red,
    Blue,
    Tan,
    Green,
    Orange,
    Purple,
    Teal,
    Pink)

ENUM_REFLECTION_STRINGIFY(
    Core::GameVersion,
    Invalid,
    SOD,
    HOTA)

ENUM_REFLECTION_STRINGIFY(
    FHResource::Type,
    Resource,
    Resource,
    TreasureChest,
    CampFire)

ENUM_REFLECTION_STRINGIFY(
    FHRandomArtifact::Type,
    Invalid,
    Any,
    Treasure,
    Minor,
    Major,
    Relic)

// clang-format off
STRUCT_REFLECTION_PAIRED(
    FHPlayer,
    "ai",                  m_aiPossible,
    "human",               m_humanPossible,
    "factions",            m_startingFactions,
    "generateHero",        m_generateHeroAtMainTown
)
// clang-format on

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHHeroData,
    m_hasExp,
    m_hasSecSkills,
    m_hasPrimSkills,
    m_hasCustomBio,
    m_hasSpells,
    m_army)

// clang-format off
STRUCT_REFLECTION_PAIRED(
    FHHero,
    "pos",                 m_pos,
    "order",               m_order,
    "player",              m_player,

    "main",                m_isMain,
    "data",                m_data,
    "questId",             m_questIdentifier
)
// clang-format on

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTown,
    m_pos,
    m_order,
    m_player,
    m_defIndex,

    m_isMain,
    m_factionId,
    m_hasFort,
    m_questIdentifier,
    m_spellResearch)

// clang-format off
STRUCT_REFLECTION_PAIRED(
    FHZone::Rect,
    "pos",                 m_pos,
    "w",                   m_width,
    "h",                   m_height
)
// clang-format on

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHResource,
    m_pos,
    m_order,
    m_defIndex,

    m_amount,
    m_id,
    m_type,
    m_visitableId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomResource,
    m_pos,
    m_order,

    m_amount)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHArtifact,
    m_pos,
    m_order,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomArtifact,
    m_pos,
    m_order,

    m_type)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPandora,
    m_pos,
    m_order,

    m_reward)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMonster,
    m_pos,
    m_order,
    m_defIndex,

    m_id,
    m_count,
    m_agressionMin,
    m_agressionMax,
    m_joinOnlyForMoney,
    m_joinPercent,
    m_questIdentifier,
    m_guardValue)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDwelling,
    m_pos,
    m_order,
    m_player,

    m_id,
    m_defIndex)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMine,
    m_pos,
    m_order,
    m_defIndex,

    m_player,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHBank,
    m_pos,
    m_order,
    m_defIndex,

    m_id,
    m_guardsVariants)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHObstacle,
    m_pos,
    m_order,
    m_defIndex,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHVisitable,
    m_pos,
    m_order,
    m_defIndex,

    m_visitableId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHShrine,
    m_pos,
    m_order,
    m_defIndex,

    m_visitableId,

    m_spellId,
    m_randomLevel)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHSkillHut,
    m_pos,
    m_order,
    m_defIndex,

    m_visitableId,

    m_skillIds)

ENUM_REFLECTION_STRINGIFY(
    FHQuest::Type,
    Invalid,
    Invalid,
    GetHeroLevel,
    GetPrimaryStat,
    KillHero,
    KillCreature,
    BringArtifacts,
    BringCreatures,
    BringResource,
    BeHero,
    BePlayer)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHQuest,
    m_type,
    m_artifacts,
    m_units,
    m_resources,
    m_primary,
    m_level,
    m_targetQuestId);

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHQuestHut,
    m_pos,
    m_order,
    m_defIndex,

    m_visitableId,

    m_reward,
    m_quest)

ENUM_REFLECTION_STRINGIFY(
    FHScholar::Type,
    Random,
    Primary,
    Secondary,
    Spell,
    Random)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHScholar,
    m_pos,
    m_order,
    m_defIndex,

    m_visitableId,

    m_type,
    m_primaryType,
    m_skillId,
    m_spellId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZoneTown,
    m_town,
    m_playerControlled,
    m_useZoneFaction)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone,
    m_player,
    m_mainTownFaction,
    m_rewardsFaction,
    m_terrain,
    m_towns,
    m_centerAvg,
    m_centerDispersion,
    m_relativeSizeAvg,
    m_relativeSizeDispersion,
    m_cornerRoads,
    m_isNormal)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngConnection,
    m_from,
    m_to,
    m_mirrorGuard,
    m_guard)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDebugTile,
    m_pos,
    m_valueA,
    m_valueB,
    m_valueC)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngOptions,
    m_roughTilePercentage,
    m_rotationDegreeDispersion,
    m_allowFlip)

ENUM_REFLECTION_STRINGIFY(
    FHRngUserSettings::HeroGeneration,
    RandomStartingFaction,
    None,
    RandomAnyFaction,
    RandomStartingFaction,
    Fixed)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngUserSettings::UserPlayer,
    m_faction,
    m_startingHero,
    m_extraHero,
    m_startingHeroGen,
    m_extraHeroGen)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngUserSettings,
    m_players,
    m_difficultyScale,
    m_defaultRoad,
    m_mapSize)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap::Objects,
    m_resources,
    m_resourcesRandom,
    m_artifacts,
    m_artifactsRandom,
    m_monsters,
    m_dwellings,
    m_banks,
    m_obstacles,
    m_visitables,
    m_mines,
    m_pandoras,
    m_shrines,
    m_skillHuts,
    m_scholars,
    m_questHuts)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap::Config,
    m_allowSpecialWeeks,
    m_hasRoundLimit,
    m_roundLimit)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap,
    m_version,
    m_seed,
    m_tileMap,
    m_tileMapUpdateRequired,
    m_name,
    m_descr,
    m_difficulty,
    m_players,
    m_wanderingHeroes,
    m_towns,
    m_zones,
    m_debugTiles,
    m_rngZones,
    m_rngConnections,
    m_rngOptions,
    m_objects,
    m_config,
    m_rivers,
    m_roads,
    m_defaultTerrain,
    m_disabledHeroes,
    m_disabledArtifacts,
    m_disabledSpells,
    m_disabledSkills,
    m_customHeroes,
    m_initialObjectDefs,
    m_defReplacements)

template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHRngUserSettings::PlayersMap>{ true };
template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::PlayersMap>{ true };
template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::RngZoneMap>{ true };
template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::RngConnectionMap>{ true };
template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::DefMap>{ true };

}
