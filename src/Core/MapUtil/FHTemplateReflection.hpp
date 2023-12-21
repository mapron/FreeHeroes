/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTemplate.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

#include "LibraryReflection.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

ENUM_REFLECTION_STRINGIFY(
    RoadLevel,
    NoRoad,
    NoRoad,
    Towns,
    Exits,
    InnerPoints,
    BorderPoints,
    Hidden)

ENUM_REFLECTION_STRINGIFY(
    ZoneObjectType,
    Segment,

    Segment,
    SegmentScatter,
    RoadScatter,

    MainTown,

    TownMid1,
    TownMid2,
    TownMid3,

    ExitMid1)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZoneTown,
    m_town,
    m_playerControlled,
    m_useZoneFaction,
    m_tilesToTarget,
    m_closeToConnection,
    m_excludeFactionZones)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHScoreSettings::ScoreScope,
    m_target,
    m_minSingle,
    m_maxSingle,
    m_maxGroup,
    m_maxRemain,
    m_consumeRemain)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHScoreSettings,
    m_score,
    m_isEnabled,
    m_guardThreshold,
    m_guardMinToGroup,
    m_guardGroupLimit,
    m_preferredHeats,
    m_placementOrder,
    m_objectType,
    m_generatorsInclude,
    m_generatorsExclude,
    m_objectIdInclude,
    m_objectIdExclude,
    m_openPandoras)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorBank::Record,
    m_frequency,
    m_guard,
    m_enabled,
    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorBank,
    m_isEnabled,
    m_records,
    m_maxUniqueFactions)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorArtifact::Record,
    m_frequency,
    m_pool,
    m_filter)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorArtifact,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorResourcePile::Record,
    m_amounts,
    m_frequency,
    m_resource,
    m_guard)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorResourcePile,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorPandora::Record,
    m_reward,
    m_frequency,
    m_guard)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorPandora,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorScroll::Record,
    m_filter,
    m_frequency,
    m_guard)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorScroll,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorDwelling::Record,
    m_level,
    m_value,
    m_frequency,
    m_guard,
    m_factionValues)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorDwelling,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorShrine::Record,
    m_filter,
    m_visualLevel,
    m_frequency,
    m_guard)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorShrine,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorVisitable::Record,
    m_frequency,
    m_guard,
    m_enabled,
    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorVisitable,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorMine::Record,
    m_resourceId,
    m_frequency,
    m_minZone,
    m_maxZone,
    m_guard,
    m_value)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorMine,
    m_isEnabled,
    m_records)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorSkillHut,
    m_isEnabled,
    m_frequency,
    m_guard)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::Generators,
    m_banks,
    m_artifacts,
    m_resources,
    m_pandoras,
    m_shrines,
    m_scrolls,
    m_dwellings,
    m_visitables,
    m_mines,
    m_skillHuts)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone,
    m_player,
    m_mainTownFaction,
    m_rewardsFaction,
    m_dwellingFaction,
    m_terrain,
    m_towns,
    m_centerAvg,
    m_centerDispersion,
    m_relativeSizeAvg,
    m_relativeSizeDispersion,
    m_zoneGuardPercent,
    m_zoneGuardDispersion,
    m_excludeFactionZones,
    m_scoreTargets,
    m_generators,
    m_guardThreshold,
    m_guardMinToGroup,
    m_guardGroupLimit,
    m_segmentAreaSize,
    m_isNormal)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngConnection::Path,
    m_road,
    m_mirrorGuard,
    m_guard,
    m_radius)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngConnection,
    m_from,
    m_to,
    m_paths)

ENUM_REFLECTION_STRINGIFY(
    FHRngUserSettings::HeroGeneration,
    RandomStartingFaction,
    None,
    RandomAnyFaction,
    RandomStartingFaction,
    FixedAny,
    FixedStarting)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngUserSettings::UserPlayer,
    m_faction,
    m_startingHero,
    m_extraHero,
    m_startingHeroGen,
    m_extraHeroGen,
    m_team,
    m_enabled)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngUserSettings::DifficultySettings,
    m_minGuardsPercent,
    m_maxGuardsPercent,
    m_minArmyPercent,
    m_maxArmyPercent,
    m_minGoldPercent,
    m_maxGoldPercent)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngUserSettings,
    m_players,
    m_difficulty,
    m_difficultyScale,
    m_defaultRoad,
    m_innerRoad,
    m_borderRoad,
    m_mapSize,
    m_hasUnderground)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTemplate,
    m_zones,
    m_connections,

    m_roughTilePercentage,
    m_rotationDegreeDispersion,
    m_allowFlip,
    m_stdSkillsWarrior,
    m_stdSkillsMage)

template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorBank::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorArtifact::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorResourcePile::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorPandora::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorScroll::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorShrine::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorDwelling::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorVisitable::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorMine::Map>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::ScoreMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::GeneratorDwelling::Record::FactionMap>{ true };

template<>
inline constexpr const bool s_isStringMap<FHScoreSettings::AttrMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngUserSettings::PlayersMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngZone::TownMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngConnection::PathMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHTemplate::RngZoneMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHTemplate::RngConnectionMap>{ true };
}
