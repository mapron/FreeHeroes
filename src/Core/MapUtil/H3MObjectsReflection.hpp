/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Reflection/EnumTraitsMacro.hpp"
#include "Reflection/MetaInfo.hpp"

#include "H3MObjects.hpp"

namespace FreeHeroes::Core::Reflection {

ENUM_REFLECTION_STRINGIY(MapQuest::Mission,
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

ENUM_REFLECTION_STRINGIY(MapQuest::Progress,
                         NOT_ACTIVE,
                         NOT_ACTIVE,
                         IN_PROGRESS,
                         COMPLETE)

ENUM_REFLECTION_STRINGIY(MapSeerHut::RewardType,
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

template<>
inline constexpr const std::tuple MetaInfo::s_fields<StackBasicDescriptor>{
    Field("id", &StackBasicDescriptor::m_id),
    Field("count", &StackBasicDescriptor::m_count),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<StackSet>{
    Field("stacks", &StackSet::m_stacks),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<StackSetFixed>{
    Field("stacks", &StackSetFixed::m_stacks),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapHeroSkill>{
    Field("id", &MapHeroSkill::m_id),
    Field("level", &MapHeroSkill::m_level),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<PrimarySkillSet>{
    Field("prim", &PrimarySkillSet::m_prim),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<HeroArtSet>{
    Field("hasArts", &HeroArtSet::m_hasArts),
    Field("main", &HeroArtSet::m_mainSlots),
    Field("cata", &HeroArtSet::m_cata),
    Field("book", &HeroArtSet::m_book),
    Field("misc5", &HeroArtSet::m_misc5),
    Field("bag", &HeroArtSet::m_bagSlots),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<HeroSpellSet>{
    Field("hasCustomSpells", &HeroSpellSet::m_hasCustomSpells),
    Field("spells", &HeroSpellSet::m_spells),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<HeroPrimSkillSet>{
    Field("hasCustomPrimSkills", &HeroPrimSkillSet::m_hasCustomPrimSkills),
    Field("primSkills", &HeroPrimSkillSet::m_primSkills),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapHero>{
    Field("questIdentifier", &MapHero::m_questIdentifier),
    Field("playerOwner", &MapHero::m_playerOwner),
    Field("subID", &MapHero::m_subID),

    Field("hasName", &MapHero::m_hasName),
    Field("name", &MapHero::m_name),
    Field("hasExp", &MapHero::m_hasExp),
    Field("exp", &MapHero::m_exp),
    Field("hasPortrait", &MapHero::m_hasPortrait),
    Field("portrait", &MapHero::m_portrait),
    Field("hasSecSkills", &MapHero::m_hasSecSkills),
    Field("secSkills", &MapHero::m_secSkills),
    Field("hasGarison", &MapHero::m_hasGarison),
    Field("garison", &MapHero::m_garison),
    Field("formation", &MapHero::m_formation),
    Field("arts", &MapHero::m_artSet),
    Field("patrolRadius", &MapHero::m_patrolRadius),
    Field("hasCustomBiography", &MapHero::m_hasCustomBiography),
    Field("bio", &MapHero::m_bio),
    Field("sex", &MapHero::m_sex),
    Field("spells", &MapHero::m_spellSet),
    Field("prim", &MapHero::m_primSkillSet),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTownEvent>{
    Field("name", &MapTownEvent::m_name),
    Field("message", &MapTownEvent::m_message),
    Field("resourceSet", &MapTownEvent::m_resourceSet),
    Field("players", &MapTownEvent::m_players),
    Field("humanAffected", &MapTownEvent::m_humanAffected),
    Field("computerAffected", &MapTownEvent::m_computerAffected),
    Field("firstOccurence", &MapTownEvent::m_firstOccurence),
    Field("nextOccurence", &MapTownEvent::m_nextOccurence),
    Field("buildings", &MapTownEvent::m_buildings),
    Field("creaturesAmounts", &MapTownEvent::m_creaturesAmounts),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapTown>{
    Field("questIdentifier", &MapTown::m_questIdentifier),
    Field("playerOwner", &MapTown::m_playerOwner),
    Field("hasName", &MapTown::m_hasName),
    Field("name", &MapTown::m_name),
    Field("hasGarison", &MapTown::m_hasGarison),
    Field("garison", &MapTown::m_garison),
    Field("formation", &MapTown::m_formation),
    Field("hasCustomBuildings", &MapTown::m_hasCustomBuildings),
    Field("builtBuildings", &MapTown::m_builtBuildings),
    Field("forbiddenBuildings", &MapTown::m_forbiddenBuildings),
    Field("hasFort", &MapTown::m_hasFort),
    Field("obligatorySpells", &MapTown::m_obligatorySpells),
    Field("possibleSpells", &MapTown::m_possibleSpells),
    Field("spellResearch", &MapTown::m_spellResearch),
    Field("events", &MapTown::m_events),
    Field("alignment", &MapTown::m_alignment),

};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapMonster>{
    Field("questIdentifier", &MapMonster::m_questIdentifier),
    Field("count", &MapMonster::m_count),
    Field("joinAppeal", &MapMonster::m_joinAppeal),
    Field("hasMessage", &MapMonster::m_hasMessage),
    Field("message", &MapMonster::m_message),
    Field("resourceSet", &MapMonster::m_resourceSet),
    Field("artID", &MapMonster::m_artID),

    Field("neverFlees", &MapMonster::m_neverFlees),
    Field("notGrowingTeam", &MapMonster::m_notGrowingTeam),
    Field("joinPercent", &MapMonster::m_joinPercent),
    Field("agressionExact", &MapMonster::m_agressionExact),
    Field("joinOnlyForMoney", &MapMonster::m_joinOnlyForMoney),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapQuest>{
    Field("missionType", &MapQuest::m_missionType),
    Field("progress", &MapQuest::m_progress),
    Field("lastDay", &MapQuest::m_lastDay),
    Field("134val", &MapQuest::m_134val),
    Field("2stats", &MapQuest::m_2stats),
    Field("5arts", &MapQuest::m_5arts),
    Field("6creatures", &MapQuest::m_6creatures),
    Field("7resources", &MapQuest::m_7resources),
    Field("89val", &MapQuest::m_89val),
    Field("firstVisitText", &MapQuest::m_firstVisitText),
    Field("nextVisitText", &MapQuest::m_nextVisitText),
    Field("completedText", &MapQuest::m_completedText),

};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapSeerHut>{
    Field("quest", &MapSeerHut::m_quest),
    Field("reward", &MapSeerHut::m_reward),
    Field("rID", &MapSeerHut::m_rID),
    Field("rVal", &MapSeerHut::m_rVal),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapReward>{
    Field("gainedExp", &MapReward::m_gainedExp),
    Field("manaDiff", &MapReward::m_manaDiff),
    Field("moraleDiff", &MapReward::m_moraleDiff),
    Field("luckDiff", &MapReward::m_luckDiff),
    Field("resourceSet", &MapReward::m_resourceSet),
    Field("primSkillSet", &MapReward::m_primSkillSet),
    Field("secSkills", &MapReward::m_secSkills),
    Field("artifacts", &MapReward::m_artifacts),
    Field("spells", &MapReward::m_spells),
    Field("creatures", &MapReward::m_creatures),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapPandora>{
    Field("message", &MapPandora::m_message),
    Field("reward", &MapPandora::m_reward),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapGuards>{
    Field("hasGuards", &MapGuards::m_hasGuards),
    Field("creatures", &MapGuards::m_creatures),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapMessage>{
    Field("hasMessage", &MapMessage::m_hasMessage),
    Field("message", &MapMessage::m_message),
    Field("guards", &MapMessage::m_guards),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapArtifact>{
    Field("message", &MapArtifact::m_message),
    Field("spellId", &MapArtifact::m_spellId),
    Field("isSpell", &MapArtifact::m_isSpell),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapResource>{
    Field("message", &MapResource::m_message),
    Field("amount", &MapResource::m_amount),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapGarison>{
    Field("owner", &MapGarison::m_owner),
    Field("garison", &MapGarison::m_garison),
    Field("removableUnits", &MapGarison::m_removableUnits),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapEvent>{
    Field("message", &MapEvent::m_message),
    Field("reward", &MapEvent::m_reward),

    Field("availableFor", &MapEvent::m_availableFor),
    Field("computerActivate", &MapEvent::m_computerActivate),
    Field("removeAfterVisit", &MapEvent::m_removeAfterVisit),
    Field("humanActivate", &MapEvent::m_humanActivate),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapDwelling>{
    Field("owner", &MapDwelling::m_owner),
    Field("factionId", &MapDwelling::m_factionId),
    Field("factionMask", &MapDwelling::m_factionMask),
    Field("minLevel", &MapDwelling::m_minLevel),
    Field("maxLevel", &MapDwelling::m_maxLevel),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MapQuestGuard>{
    Field("quest", &MapQuestGuard::m_quest),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<ResourceSet>{
    Field("resourceAmount", &ResourceSet::m_resourceAmount),
};

}
