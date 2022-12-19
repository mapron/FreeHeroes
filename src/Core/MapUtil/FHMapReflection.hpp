/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMap.hpp"

#include "Reflection/EnumTraitsMacro.hpp"
#include "Reflection/MetaInfoMacro.hpp"

#include "FHTileMapReflection.hpp"

// clang-format off
namespace FreeHeroes::Core::Reflection {

ENUM_REFLECTION_STRINGIY(FHPlayerId,
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

ENUM_REFLECTION_STRINGIY(Core::GameVersion,
                         Invalid,
                         SOD,
                         HOTA)

ENUM_REFLECTION_STRINGIY(FHResource::Type,
                         Resource,
                         Resource,
                         TreasureChest,
                         CampFire)

ENUM_REFLECTION_STRINGIY(FHRandomArtifact::Type,
                         Invalid,
                         Any,
                         Treasure,
                         Minor,
                         Major,
                         Relic)

STRUCT_REFLECTION_PAIRED(
    FHPlayer,
    "ai",                  m_aiPossible,
    "human",               m_humanPossible,
    "factions",            m_startingFactions
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHHeroData,
    m_hasExp,
    m_hasSecSkills,
    m_hasPrimSkills,
    m_hasCustomBio,
    m_hasSpells,
    m_army
)

STRUCT_REFLECTION_PAIRED(
    FHHero,
    "pos",                 m_pos,
    "order",               m_order,
    "player",              m_player,

    "main",                m_isMain,
    "data",                m_data,
    "questId",             m_questIdentifier
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHTown,
    m_pos,
    m_order,
    m_player,

    m_isMain,
    m_factionId,
    m_hasFort,
    m_questIdentifier,
    m_spellResearch,
    m_defFile
)

STRUCT_REFLECTION_PAIRED(
    FHZone::Rect,
    "pos",                 m_pos,
    "w",                   m_width,
    "h",                   m_height
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHResource,
    m_pos,
    m_order,

    m_amount,
    m_id,
    m_type
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomResource,
    m_pos,
    m_order,

    m_amount
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHArtifact,
    m_pos,
    m_order,

    m_id
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHRandomArtifact,
    m_pos,
    m_order,

    m_type
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHPandora,
    m_pos,
    m_order,

    m_reward
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMonster,
    m_pos,
    m_order,

    m_id,
    m_count,
    m_agressionMin,
    m_agressionMax,
    m_questIdentifier
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHDwelling,
    m_pos,
    m_order,
    m_player,

    m_id,
    m_defVariant
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMine,
    m_pos,
    m_order,
    m_player,

    m_id,
    m_defVariant
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHBank,
    m_pos,
    m_order,

    m_id,
    m_defVariant,
    m_guardsVariants
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHObstacle,
    m_pos,
    m_order,

    m_id
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHVisitable,
    m_pos,
    m_order,

    m_id,
    m_defVariant
)

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
    m_pandoras
)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    FHMap,
    m_version,
    m_seed,
    m_tileMap,
    m_name,
    m_descr,
    m_difficulty,
    m_players,
    m_wanderingHeroes,
    m_towns,
    m_zones,
    m_objects,
    m_rivers,
    m_roads,
    m_defaultTerrain,
    m_disabledHeroes,
    m_disabledArtifacts,
    m_disabledSpells,
    m_disabledSkills,
    m_customHeroes,
    m_initialObjectDefs
)

template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::PlayersMap>{ true };

// clang-format on

}
