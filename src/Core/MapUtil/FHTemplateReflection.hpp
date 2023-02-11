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
    FHScoreAttr,
    Invalid,

    Invalid,
    Army,
    ArtStat,
    ArtSupport,
    Gold,
    Resource,
    ResourceGen,
    Experience,
    Control,
    Upgrade,
    SpellOffensive,
    SpellCommon,
    SpellAll,
    Misc)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZoneTown,
    m_town,
    m_playerControlled,
    m_useZoneFaction)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHScoreSettings::ScoreScope,
    m_target,
    m_minSingle,
    m_maxSingle)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHScoreSettings,
    m_score,
    m_isEnabled,
    m_guardPercent)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorBank,
    m_isEnabled)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngZone::GeneratorArtifact::Record,
    m_frequency,
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
    m_weeks,
    m_castles,
    m_frequency,
    m_guard)

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
    FHRngZone::Generators,
    m_banks,
    m_artifacts,
    m_resources,
    m_pandoras,
    m_shrines,
    m_scrolls,
    m_dwellings)

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
    m_scoreTargets,
    m_generators,
    m_guardMin,
    m_guardMax,
    m_cornerRoads,
    m_isNormal)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRngConnection,
    m_from,
    m_to,
    m_mirrorGuard,
    m_guard)

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
    m_stdStats)

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
inline constexpr const bool s_isStringMap<FHRngZone::ScoreMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHScoreSettings::AttrMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngUserSettings::PlayersMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHTemplate::RngZoneMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHTemplate::RngConnectionMap>{ true };

}
