/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

#include "H3MObjects.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

ENUM_REFLECTION_STRINGIY(
    MapQuest::Mission,
    NONE,
    NONE,
    LEVEL,
    PRIMARY_STAT,
    KILL_HERO,
    KILL_CREATURE,
    ART,
    ARMY,
    RESOURCES,
    HERO,
    PLAYER)

ENUM_REFLECTION_STRINGIY(
    MapQuest::Progress,
    NOT_ACTIVE,
    NOT_ACTIVE,
    IN_PROGRESS,
    COMPLETE)

ENUM_REFLECTION_STRINGIY(
    MapSeerHut::RewardType,
    NOTHING,
    NOTHING,
    EXPERIENCE,
    MANA_POINTS,
    MORALE_BONUS,
    LUCK_BONUS,
    RESOURCES,
    PRIMARY_SKILL,
    SECONDARY_SKILL,
    ARTIFACT,
    SPELL,
    CREATURE)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    StackBasicDescriptor,
    m_id,
    m_count)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    StackSet,
    m_stacks)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    StackSetFixed,
    m_stacks)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapHeroSkill,
    m_id,
    m_level)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    PrimarySkillSet,
    m_prim)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    HeroArtSet,
    m_hasArts,
    m_mainSlots,
    m_cata,
    m_book,
    m_misc5,
    m_bagSlots)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    HeroSpellSet,
    m_hasCustomSpells,
    m_spells)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    HeroPrimSkillSet,
    m_hasCustomPrimSkills,
    m_primSkills)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapHero,
    m_questIdentifier,
    m_playerOwner,
    m_subID,

    m_hasName,
    m_name,
    m_hasExp,
    m_exp,
    m_hasPortrait,
    m_portrait,
    m_hasSecSkills,
    m_secSkills,
    m_hasGarison,
    m_garison,
    m_formation,
    m_artSet,
    m_patrolRadius,
    m_hasCustomBiography,
    m_bio,
    m_sex,
    m_spellSet,
    m_primSkillSet)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapTownEvent,
    m_name,
    m_message,
    m_resourceSet,
    m_players,
    m_humanAffected,
    m_computerAffected,
    m_firstOccurence,
    m_nextOccurence,
    m_buildings,
    m_creaturesAmounts)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapTown,
    m_questIdentifier,
    m_playerOwner,
    m_hasName,
    m_name,
    m_hasGarison,
    m_garison,
    m_formation,
    m_hasCustomBuildings,
    m_builtBuildings,
    m_forbiddenBuildings,
    m_hasFort,
    m_obligatorySpells,
    m_possibleSpells,
    m_spellResearch,
    m_events,
    m_alignment

)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapMonster,
    m_questIdentifier,
    m_count,
    m_joinAppeal,
    m_hasMessage,
    m_message,
    m_resourceSet,
    m_artID,

    m_neverFlees,
    m_notGrowingTeam,
    m_joinPercent,
    m_agressionExact,
    m_joinOnlyForMoney)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapQuest,
    m_missionType,
    m_progress,
    m_lastDay,
    m_134val,
    m_2stats,
    m_5arts,
    m_6creatures,
    m_7resources,
    m_89val,
    m_firstVisitText,
    m_nextVisitText,
    m_completedText

)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapSeerHut,
    m_quest,
    m_reward,
    m_rID,
    m_rVal)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapReward,
    m_gainedExp,
    m_manaDiff,
    m_moraleDiff,
    m_luckDiff,
    m_resourceSet,
    m_primSkillSet,
    m_secSkills,
    m_artifacts,
    m_spells,
    m_creatures)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapPandora,
    m_message,
    m_reward)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapGuards,
    m_hasGuards,
    m_creatures)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapMessage,
    m_hasMessage,
    m_message,
    m_guards)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapArtifact,
    m_message,
    m_spellId,
    m_isSpell)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapResource,
    m_message,
    m_amount)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapGarison,
    m_owner,
    m_garison,
    m_removableUnits)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapEvent,
    m_message,
    m_reward,

    m_availableFor,
    m_computerActivate,
    m_removeAfterVisit,
    m_humanActivate)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapDwelling,
    m_owner,
    m_factionId,
    m_factionMask,
    m_minLevel,
    m_maxLevel)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    MapQuestGuard,
    m_quest)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    ResourceSet,
    m_resourceAmount)

}
