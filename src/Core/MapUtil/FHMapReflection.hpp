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

// clang-format off
STRUCT_REFLECTION_PAIRED(
    FHPlayer,
    "ai",                  m_aiPossible,
    "human",               m_humanPossible,
    "factions",            m_startingFactions,
    "team",                m_team,
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
    m_guard,
    m_score,

    m_isMain,
    m_factionId,
    m_hasFort,
    m_questIdentifier,
    m_spellResearch,
    m_hasCustomBuildings,
    m_buildings,
    m_hasGarison,
    m_garison,
    m_garisonRmg)

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
    m_guard,
    m_score,

    m_amount,
    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomResource,
    m_pos,
    m_order,
    m_guard,
    m_score,

    m_amount)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHArtifact,
    m_pos,
    m_order,
    m_guard,
    m_score,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomArtifact,
    m_pos,
    m_order,
    m_guard,
    m_score,

    m_type)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPandora,
    m_pos,
    m_order,
    m_guard,
    m_score,

    m_reward,
    m_generationId)

ENUM_REFLECTION_STRINGIFY(
    FHMonster::UpgradedStack,
    Invalid,

    Invalid,
    Random,
    Yes,
    No)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMonster,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,

    m_id,
    m_count,
    m_aggressionMin,
    m_aggressionMax,
    m_joinOnlyForMoney,
    m_joinPercent,
    m_questIdentifier,
    m_guardValue,
    m_upgradedStack)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDwelling,
    m_pos,
    m_order,
    m_player,
    m_defIndex,
    m_guard,
    m_score,

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMine,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,

    m_player,

    m_id)

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

    m_id)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHVisitable,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,

    m_visitableId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHShrine,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,

    m_visitableId,

    m_spellId,
    m_randomLevel)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHSkillHut,
    m_pos,
    m_order,
    m_defIndex,
    m_guard,
    m_score,

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
    m_guard,
    m_score,

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
    m_guard,
    m_score,

    m_visitableId,

    m_type,
    m_primaryType,
    m_skillId,
    m_spellId)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDebugTile,
    m_pos,
    m_valueA,
    m_valueB,
    m_valueC)

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
    m_isWaterMap,
    m_players,
    m_wanderingHeroes,
    m_towns,
    m_zones,
    m_debugTiles,
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
