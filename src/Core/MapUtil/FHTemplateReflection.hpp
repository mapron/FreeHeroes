/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHTemplate.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

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
    m_guarded,
    m_unguarded,
    m_armyFocusPercent)

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
    m_score,
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
inline constexpr const bool s_isStringMap<FHScoreSettings::AttrMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHRngUserSettings::PlayersMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHTemplate::RngZoneMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHTemplate::RngConnectionMap>{ true };

}
