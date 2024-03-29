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
#include "FHTemplateReflection.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

ENUM_REFLECTION_STRINGIFY(
    Core::GameVersion,
    Invalid,
    SOD,
    HOTA)

ENUM_REFLECTION_STRINGIFY(
    FHRandomArtifact::Type,
    Invalid,
    Any,
    Treasure,
    Minor,
    Major,
    Relic)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPlayer::HeroName,
    m_name,
    m_hero)

ENUM_REFLECTION_STRINGIFY(
    FHPlayer::AiTactic,
    NONE,
    NONE,
    RANDOM,
    WARRIOR,
    BUILDER,
    EXPLORER)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPlayer,
    m_aiPossible,
    m_humanPossible,
    m_generateHeroAtMainTown,
    m_hasRandomHero,
    m_isFactionRandom,
    m_aiTactic,
    m_startingFactions,
    m_team,
    m_unused1,
    m_placeholder,
    m_generatedHeroTownFaction,
    m_mainCustomHeroPortrait,
    m_mainCustomHeroName,
    m_heroesNames)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDisposedHero,
    m_heroId,
    m_portrait,
    m_name,
    m_players)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHGuard,
    m_hasGuards,
    m_creatures)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMessageWithBattle,
    m_hasMessage,
    m_message,
    m_guards)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHHeroData,
    m_hasExp,
    m_hasSecSkills,
    m_hasPrimSkills,
    m_hasCustomBio,
    m_hasSpells,
    m_hasArmy,
    m_hasArts,
    m_hasName,
    m_hasPortrait,
    m_portrait,
    m_sex,
    m_name,
    m_bio,
    m_army)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHCustomHeroDataExt,
    m_unknown1,
    m_unknown2)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHHero,
    m_pos,
    m_order,
    m_defIndex,

    m_player,

    m_isMain,
    m_data,
    m_patrolRadius,
    m_isRandom,
    m_isPrison,
    m_isCamp,
    m_groupedFormation,
    m_questIdentifier,
    m_unknown1,
    m_unknown2)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTownEvent,
    m_name,
    m_message,
    m_resources,
    m_players,
    m_humanAffected,
    m_computerAffected,
    m_firstOccurence,
    m_nextOccurence,
    m_buildings,
    m_creaturesAmounts,
    m_unknown1,
    m_unknown2,
    m_unknown3,
    m_unknown4)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTown,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_player,

    m_isMain,
    m_factionId,
    m_hasFort,
    m_questIdentifier,
    m_spellResearch,
    m_hasCustomBuildings,
    m_buildings,
    m_forbiddenBuildings,
    m_events,
    m_somethingBuildingRelated,
    m_obligatorySpells,
    m_possibleSpells,
    m_hasGarison,
    m_hasName,
    m_randomTown,
    m_randomId,
    m_alignment,
    m_groupedFormation,
    m_name,
    m_garison,
    m_garisonRmg)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHResource,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_amount,
    m_id,
    m_messageWithBattle)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomResource,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_amount,
    m_messageWithBattle)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHArtifact,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_id,
    m_messageWithBattle,
    m_pickupCondition1,
    m_pickupCondition2)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomArtifact,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_type,
    m_messageWithBattle,
    m_pickupCondition1,
    m_pickupCondition2)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPandora,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_reward,
    m_key,
    m_openPandora,
    m_messageWithBattle)

ENUM_REFLECTION_STRINGIFY(
    FHMonster::UpgradedStack,
    Invalid,

    Invalid,
    Random,
    Yes,
    No)
ENUM_REFLECTION_STRINGIFY(
    FHMonster::SplitStack,
    Invalid,

    Invalid,
    Average,
    OneMore,
    OneLess,
    Exact)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMonster,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_id,
    m_count,
    m_randomLevel,
    m_aggressionMin,
    m_aggressionMax,
    m_joinOnlyForMoney,
    m_joinPercent,
    m_neverFlees,
    m_notGrowingTeam,
    m_questIdentifier,
    m_guardValue,
    m_quantityMode,
    m_quantityByAiValue,
    m_hasMessage,
    m_reward,
    m_message,
    m_upgradedStack,
    m_splitStackExact,
    m_splitStackType)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDwelling,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_player,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomDwelling,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_player,

    m_id,
    m_hasFaction,
    m_hasLevel,
    m_factionId,
    m_factionMask,
    m_minLevel,
    m_maxLevel)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMine,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_player,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHAbandonedMine,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_resources,
    m_customGuards,
    m_creatureId,
    m_countMin,
    m_countMax)

ENUM_REFLECTION_STRINGIFY(
    FHBank::UpgradedStack,
    Invalid,

    Invalid,
    Random,
    Yes,
    No)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHBank,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_id,
    m_upgradedStack,
    m_guardsVariant,
    m_artifacts)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHObstacle,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHVisitable,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_customChoice,
    m_creatureCount,
    m_reward,
    m_artifactsForSale,
    m_skillsToLearn,
    m_unknown0,
    m_unknown1,
    m_unknown2,
    m_unknown3,
    m_unknown4)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHVisitableControlled,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_player)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHSign,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_text)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHShrine,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_spellId,
    m_randomLevel)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHSkillHut,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

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
    BePlayer,
    BeHeroClass,
    WaitUntilDay)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHQuest,
    m_type,
    m_artifacts,
    m_units,
    m_resources,
    m_primary,
    m_level,
    m_lastDay,
    m_minimumDay,
    m_expectClasses,
    m_targetQuestId,
    m_firstVisitText,
    m_nextVisitText,
    m_completedText);

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHQuestHut::FHQuestWithReward,

    m_reward,
    m_quest)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHQuestHut,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_questsOneTime,
    m_questsRecurring)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHQuestGuard,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

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
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_type,
    m_primaryType,
    m_skillId,
    m_spellId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHLocalEvent,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_message,
    m_reward,
    m_players,
    m_computerActivate,
    m_removeAfterVisit,
    m_humanActivate)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHGarison,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_visitableId,
    m_fixedDef,

    m_player,

    m_garison,
    m_removableUnits)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHHeroPlaceholder,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_player,

    m_hero,
    m_powerRank,
    m_hasArmy,
    m_garison,
    m_bagSlots)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHGrail,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_radius)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHUnknownObject,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,
    m_generationId,

    m_defId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDebugTile,
    m_pos,
    m_brushColor,
    m_brushAlpha,
    m_brushPalette,
    m_penColor,
    m_penAlpha,
    m_penPalette,
    m_textColor,
    m_textAlpha,
    m_textPalette,
    m_shape,
    m_shapeRadius,
    m_text)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap::Objects,
    m_resources,
    m_resourcesRandom,
    m_artifacts,
    m_artifactsRandom,
    m_monsters,
    m_dwellings,
    m_randomDwellings,
    m_banks,
    m_obstacles,
    m_visitables,
    m_controlledVisitables,
    m_mines,
    m_abandonedMines,
    m_pandoras,
    m_shrines,
    m_skillHuts,
    m_scholars,
    m_questHuts,
    m_questGuards,
    m_localEvents,
    m_signs,
    m_garisons,
    m_heroPlaceholders,
    m_grails,
    m_unknownObjects)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHGlobalMapEvent,
    m_name,
    m_message,
    m_resources,
    m_players,
    m_humanAffected,
    m_computerAffected,
    m_firstOccurence,
    m_nextOccurence,
    m_unknown1,
    m_unknown2,
    m_unknown3,
    m_unknown4)

ENUM_REFLECTION_STRINGIFY(
    FHVictoryCondition::Type,
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
    FHLossCondition::Type,
    LOSSSTANDARD,
    LOSSCASTLE,
    LOSSHERO,
    TIMEEXPIRES,
    LOSSSTANDARD)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHVictoryCondition,
    m_type,
    m_allowNormalVictory,
    m_appliesToAI,
    m_artID,
    m_creature,
    m_resourceID,
    m_resourceAmount,
    m_pos,
    m_hallLevel,
    m_castleLevel,
    m_days)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHLossCondition,
    m_type,
    m_pos,
    m_days)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap::Config::HotaVersion,
    m_ver1,
    m_ver2,
    m_ver3)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap::Config,
    m_allowSpecialWeeks,
    m_hasRoundLimit,
    m_roundLimit,
    m_levelLimit,
    m_hotaVersion,
    m_unknown1,
    m_allowedDifficulties,
    m_unknown3,
    m_unknown4)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap::Rumor,
    m_name,
    m_text)

ENUM_REFLECTION_STRINGIFY(
    FHMap::MapFormat,
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

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap,
    m_format,
    m_seed,
    m_tileMap,
    m_packedTileMap,
    m_name,
    m_descr,
    m_difficulty,
    m_isWaterMap,
    m_anyPlayers,
    m_players,
    m_wanderingHeroes,
    m_towns,
    m_debugTiles,
    m_objects,
    m_config,
    m_disabledHeroes,
    m_disabledArtifacts,
    m_disabledSpells,
    m_disabledSkills,
    m_disabledBanks,
    m_placeholderHeroes,
    m_disposedHeroes,
    m_customHeroes,
    m_customHeroDataExt,
    m_globalEvents,
    m_objectDefs,
    m_victoryCondition,
    m_lossCondition,
    m_rumors)

template<>
inline constexpr const bool s_isStringMap<FHMap::PlayersMap>{ true };
template<>
inline constexpr const bool s_isStringMap<FHMap::DefMap>{ true };

#define FHMAP_DISABLE_CONFIG_SETUP(name) \
    template<> \
    inline constexpr const bool s_isStringMap<name::Map>{ true }; \
    template<> \
    inline constexpr const bool s_useCustomTransformRead<name>{ true }; \
    template<> \
    inline bool MetaInfo::transformTreeRead<name>(const PropertyTree& treeIn, PropertyTree& treeOut) \
    { \
        treeOut.convertToMap(); \
        treeOut["data"] = treeIn; \
        return true; \
    } \
    template<> \
    inline constexpr const bool s_useCustomTransformWrite<name>{ true }; \
    template<> \
    inline bool MetaInfo::transformTreeWrite<name>(const PropertyTree& treeIn, PropertyTree& treeOut) \
    { \
        PropertyTree m; \
        m.convertToMap(); \
        treeOut = treeIn.contains("data") ? treeIn["data"] : m; \
        return true; \
    } \
    template<> \
    struct MetaInfo::MetaFields<name> { \
        static inline constexpr const std::tuple s_fields{ \
            Field("data", &name::m_data), \
        }; \
    };

FHMAP_DISABLE_CONFIG_SETUP(FHMap::DisableConfigHeroes)
FHMAP_DISABLE_CONFIG_SETUP(FHMap::DisableConfigArtifacts)
FHMAP_DISABLE_CONFIG_SETUP(FHMap::DisableConfigSpells)
FHMAP_DISABLE_CONFIG_SETUP(FHMap::DisableConfigSecondarySkills)
FHMAP_DISABLE_CONFIG_SETUP(FHMap::DisableConfigBanks)

}
