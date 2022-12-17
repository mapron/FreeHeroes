/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryArtifact.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryGameRules.hpp"
#include "LibraryHero.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryResource.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryUnit.hpp"

#include "Reflection/EnumTraits.hpp"
#include "Reflection/MetaInfo.hpp"

#include "StringUtils.hpp"

namespace FreeHeroes {
class PropertyTree;
}

namespace FreeHeroes::Core::Reflection {

// clang-format off
template<>
inline constexpr const std::tuple MetaInfo::s_fields<PrimaryRngParams>{
    Field("luck"  , &PrimaryRngParams::luck),
    Field("morale", &PrimaryRngParams::morale),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<PrimaryAttackParams>{
    Field("attack" , &PrimaryAttackParams::attack),
    Field("defense", &PrimaryAttackParams::defense),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<PrimaryMagicParams>{
    Field("sp"  , &PrimaryMagicParams::spellPower),
    Field("int" , &PrimaryMagicParams::intelligence),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<DamageDesc>{
    Field("min", &DamageDesc::minDamage),
    Field("max", &DamageDesc::maxDamage),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<UnitPrimaryParams>{
    Field("dmg"       , &UnitPrimaryParams::dmg),
    Field("ad"        , &UnitPrimaryParams::ad),
    Field("maxHealth" , &UnitPrimaryParams::maxHealth),
    Field("speed"     , &UnitPrimaryParams::battleSpeed),
    Field("armySpeed" , &UnitPrimaryParams::armySpeed),
    Field("shoots"    , &UnitPrimaryParams::shoots),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<HeroPrimaryParams>{
    Field("ad"   , &HeroPrimaryParams::ad),
    Field("magic", &HeroPrimaryParams::magic),
};
template<>
inline constexpr const auto EnumTraits::s_valueMapping<HeroPrimaryParamType> = EnumTraits::make(
    HeroPrimaryParamType::Attack,
    "attack" ,   HeroPrimaryParamType::Attack,
    "defense",   HeroPrimaryParamType::Defense,
    "sp"     ,   HeroPrimaryParamType::SpellPower,
    "int"    ,   HeroPrimaryParamType::Intelligence,
    "mana"   ,   HeroPrimaryParamType::Mana,
    "exp"    ,   HeroPrimaryParamType::Experience
    );

//inline constexpr const std::tuple MetaInfo::s_fields<Reflection::LibraryIdResolver::DelayedDeserializeParam>{

template<>
inline constexpr const std::tuple MetaInfo::s_fields<MagicReduce>{
    
    Field("all"      , &MagicReduce::allMagic),
    Field("air"      , &MagicReduce::air   ),
    Field("earth"    , &MagicReduce::earth ),
    Field("fire"     , &MagicReduce::fire  ),
    Field("water"    , &MagicReduce::water ),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<MagicIncrease>{
    
    Field("all"      , &MagicIncrease::allMagic),
    Field("air"      , &MagicIncrease::air   ),
    Field("earth"    , &MagicIncrease::earth ),
    Field("fire"     , &MagicIncrease::fire  ),
    Field("water"    , &MagicIncrease::water ),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<RngChanceMultiplier>{
    
    Field("luckPositive"       , &RngChanceMultiplier::luckPositive),
    Field("moralePositive"     , &RngChanceMultiplier::moralePositive),
    Field("luckNegative"       , &RngChanceMultiplier::luckNegative),
    Field("moraleNegative"     , &RngChanceMultiplier::moraleNegative),
};

// ------------------------------------------------------------------------------------------

template<>
inline constexpr const bool MetaInfo::s_isStringMap<LibraryTerrain::Presentation::BTMap>{ true };

template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryTerrain::BorderType> = EnumTraits::make(
    LibraryTerrain::BorderType::Invalid,
    "TL"  ,   LibraryTerrain::BorderType::TL,
    "L"   ,   LibraryTerrain::BorderType::L,
    "T"   ,   LibraryTerrain::BorderType::T,
    "BR"  ,   LibraryTerrain::BorderType::BR,
    "TLS" ,   LibraryTerrain::BorderType::TLS,
    "BRS" ,   LibraryTerrain::BorderType::BRS,
    "ThreeWay_DD"      ,   LibraryTerrain::BorderType::ThreeWay_DD,
    "ThreeWay_DS"      ,   LibraryTerrain::BorderType::ThreeWay_DS,
    "ThreeWay_SS"      ,   LibraryTerrain::BorderType::ThreeWay_SS,
    "ThreeWay_RD_BLS"  ,   LibraryTerrain::BorderType::ThreeWay_RD_BLS,
    "ThreeWay_BD_TRS"  ,   LibraryTerrain::BorderType::ThreeWay_BD_TRS,
    "ThreeWay_TRD_BRS" ,   LibraryTerrain::BorderType::ThreeWay_TRD_BRS,
    "ThreeWay_BRS_BLD" ,   LibraryTerrain::BorderType::ThreeWay_BRS_BLD,
    "ThreeWay_RS_BD"   ,   LibraryTerrain::BorderType::ThreeWay_RS_BD,
    "ThreeWay_BS_RD"   ,   LibraryTerrain::BorderType::ThreeWay_BS_RD,
    "Center" ,   LibraryTerrain::BorderType::Center
    );

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryTerrain::Presentation>{
    Field("music"                     , &LibraryTerrain::Presentation::music),
    Field("backgroundsBattle"         , &LibraryTerrain::Presentation::backgroundsBattle),
    Field("order"                     , &LibraryTerrain::Presentation::order),
    Field("defFile"                   , &LibraryTerrain::Presentation::defFile),
    Field("defFileSplit"              , &LibraryTerrain::Presentation::defFileSplit),
    Field("isAnimated"                , &LibraryTerrain::Presentation::isAnimated),
    Field("dirtBorderTilesOffset"     , &LibraryTerrain::Presentation::dirtBorderTilesOffset),
    Field("sandBorderTilesOffset"     , &LibraryTerrain::Presentation::sandBorderTilesOffset),
    Field("sandDirtBorderTilesOffset" , &LibraryTerrain::Presentation::sandDirtBorderTilesOffset),
    Field("centerTilesOffset"         , &LibraryTerrain::Presentation::centerTilesOffset),
    Field("centerTilesCount"          , &LibraryTerrain::Presentation::centerTilesCount),
    Field("borderCounts"              , &LibraryTerrain::Presentation::borderCounts),
    Field("borderThreeWayCounts"      , &LibraryTerrain::Presentation::borderThreeWayCounts),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryTerrain::Bonus>{
    
    Field("rngMult"            , &LibraryTerrain::Bonus::rngMult),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryTerrain>{
    Field("untranslatedName"   , &LibraryTerrain::untranslatedName),
    Field("legacyId"           , &LibraryTerrain::legacyId),
    Field("moveCost"           , &LibraryTerrain::moveCost),
    Field("isObstacle"         , &LibraryTerrain::isObstacle),
    Field("extraLayer"         , &LibraryTerrain::extraLayer),
    Field("bonusAll"           , &LibraryTerrain::bonusAll),
    Field("pres"               , &LibraryTerrain::presentationParams),
};
// ------------------------------------------------------------------------------------------
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryResource::Presentation>{
    
    Field("orderKingdom"     , &LibraryResource::Presentation::orderKingdom),
    Field("orderCommon"      , &LibraryResource::Presentation::orderCommon),
    Field("icon"             , &LibraryResource::Presentation::icon),
    Field("iconLarge"        , &LibraryResource::Presentation::iconLarge),
    Field("iconTrade"        , &LibraryResource::Presentation::iconTrade),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryResource>{
    
    Field("untranslatedName" , &LibraryResource::untranslatedName),
    Field("legacyId"         , &LibraryResource::legacyId),
    Field("mapObjectDef"     , &LibraryResource::mapObjectDef),
    Field("pres"             , &LibraryResource::presentationParams),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<ResourceAmount>{
    
    Field("gold"     , &ResourceAmount::gold),
    Field("wood"     , &ResourceAmount::wood),
    Field("ore"      , &ResourceAmount::ore ),
    Field("mercury"  , &ResourceAmount::mercury),
    Field("sulfur"   , &ResourceAmount::sulfur ),
    Field("crystal"  , &ResourceAmount::crystal),
    Field("gems"     , &ResourceAmount::gems   ),
};

//  -----------------------------------------------------------------------------
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryFactionHeroClass::Presentation>{
    
    Field("battleSpriteMale"  , &LibraryFactionHeroClass::Presentation::battleSpriteMale),
    Field("battleSpriteFemale", &LibraryFactionHeroClass::Presentation::battleSpriteFemale),
    Field("adventureSpriteMale"  , &LibraryFactionHeroClass::Presentation::adventureSpriteMale),
    Field("adventureSpriteFemale", &LibraryFactionHeroClass::Presentation::adventureSpriteFemale),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryFactionHeroClass>{
    
    Field("id", &LibraryFactionHeroClass::id),
    Field("startParams"       , &LibraryFactionHeroClass::startParams),
    Field("skills"            , &LibraryFactionHeroClass::secondarySkillWeights),
    Field("untranslatedName"  , &LibraryFactionHeroClass::untranslatedName),
    Field("lowLevelIncrease"  , &LibraryFactionHeroClass::lowLevelIncrease),
    Field("highLevelIncrease" , &LibraryFactionHeroClass::highLevelIncrease),
    Field("pres"              , &LibraryFactionHeroClass::presentationParams),
};
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryFaction::Alignment> = EnumTraits::make(
    LibraryFaction::Alignment::Special,
    "good"       ,  LibraryFaction::Alignment::Good,
    "evil"       ,  LibraryFaction::Alignment::Evil,
    "neutral"    ,  LibraryFaction::Alignment::Neutral,
    "independent",  LibraryFaction::Alignment::Independent,
    "special"    ,  LibraryFaction::Alignment::Special
    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryFaction::Presentation>{
    
    Field("goesAfterId"       , &LibraryFaction::Presentation::goesAfterId),
    Field("unitBackground" , &LibraryFaction::Presentation::unitBackground),
    Field("townAnimations" , &LibraryFaction::Presentation::townAnimations),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryFaction>{
    
    Field("alignment"         , &LibraryFaction::alignment),
    Field("untranslatedName"  , &LibraryFaction::untranslatedName),
    Field("legacyId"          , &LibraryFaction::legacyId),
    Field("warriorClass"      , &LibraryFaction::warriorClass),
    Field("mageClass"         , &LibraryFaction::mageClass),
    Field("nativeTerrain"     , &LibraryFaction::nativeTerrain),
    Field("mapObjectDef"      , &LibraryFaction::mapObjectDef),
    Field("pres"              , &LibraryFaction::presentationParams),
};
 //  -----------------------------------------------------------------------------
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibrarySecondarySkill::Presentation::LevelParams>{
    
    Field("iconSmall" , &LibrarySecondarySkill::Presentation::LevelParams::iconSmall),
    Field("iconMedium", &LibrarySecondarySkill::Presentation::LevelParams::iconMedium),
    Field("iconLarge" , &LibrarySecondarySkill::Presentation::LevelParams::iconLarge),
};
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySecondarySkill::HandlerType> = EnumTraits::make(
    LibrarySecondarySkill::HandlerType::Stat,
    "stat"       ,  LibrarySecondarySkill::HandlerType::Stat,
    "special"    ,  LibrarySecondarySkill::HandlerType::Special,
    "wisdom"     ,  LibrarySecondarySkill::HandlerType::Wisdom,
    "school"     ,  LibrarySecondarySkill::HandlerType::School
    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibrarySecondarySkill::Presentation>{
    Field("levels"            , &LibrarySecondarySkill::Presentation::levels),
    Field("order"             , &LibrarySecondarySkill::Presentation::order),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibrarySecondarySkill>{
    Field("untranslatedName"  , &LibrarySecondarySkill::untranslatedName),
    Field("legacyId"          , &LibrarySecondarySkill::legacyId),
    Field("frequencyWarrior"  , &LibrarySecondarySkill::frequencyWarrior),
    Field("frequencyMage"     , &LibrarySecondarySkill::frequencyMage),
    Field("handler"           , &LibrarySecondarySkill::handler),
    Field("calc"              , &LibrarySecondarySkill::calc),
    Field("pres"              , &LibrarySecondarySkill::presentationParams),
};
 //  -----------------------------------------------------------------------------

template<>
inline constexpr const auto EnumTraits::s_valueMapping<UnitType> = EnumTraits::make(
    UnitType::Unknown,
    "living"     ,  UnitType::Living,
    "nonLiving"  ,  UnitType::NonLiving,
    "siege"      ,  UnitType::SiegeMachine,
    "arrowTower" ,  UnitType::ArrowTower,
    "wall"       ,  UnitType::Wall,
    ""           ,  UnitType::Unknown
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<UnitNonLivingType> = EnumTraits::make(
    UnitNonLivingType::None,
    ""          ,   UnitNonLivingType::None,
    "undead"    ,   UnitNonLivingType::Undead,
    "golem"     ,   UnitNonLivingType::Golem,
    "gargoyle"  ,   UnitNonLivingType::Gargoyle,
    "elemental" ,   UnitNonLivingType::Elemental,
    "bm"        ,   UnitNonLivingType::BattleMachine
    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Traits>{
    
    Field("large"        , &LibraryUnit::Traits::large),
    Field("ranged"       , &LibraryUnit::Traits::rangeAttack),
    Field("fly"          , &LibraryUnit::Traits::fly),
    Field("teleport"     , &LibraryUnit::Traits::teleport),
    Field("dblAttack"    , &LibraryUnit::Traits::doubleAttack),
    Field("freeAttack"   , &LibraryUnit::Traits::freeAttack),
    Field("catapultShoot", &LibraryUnit::Traits::canBeCatapult),
    Field("autoReturn"   , &LibraryUnit::Traits::returnAfterAttack),
};

template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryUnit::Abilities::SplashAttack> = EnumTraits::make(
    LibraryUnit::Abilities::SplashAttack::None,
    ""           ,  LibraryUnit::Abilities::SplashAttack::None,
    "dragon"     ,  LibraryUnit::Abilities::SplashAttack::Dragon,
    "neighbours" ,  LibraryUnit::Abilities::SplashAttack::Neighbours,
    "sides"      ,  LibraryUnit::Abilities::SplashAttack::Sides,
    "ranged"     ,  LibraryUnit::Abilities::SplashAttack::Ranged
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<RangeAttackPenalty> = EnumTraits::make(
    RangeAttackPenalty::Melee,
    "melee"    , RangeAttackPenalty::Melee,
    "distance" , RangeAttackPenalty::Distance,
    "obstacle" , RangeAttackPenalty::Obstacle,
    "blocked"  , RangeAttackPenalty::Blocked
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryUnit::Abilities::AttackWithElement> = EnumTraits::make(
    LibraryUnit::Abilities::AttackWithElement::None,
    ""       ,  LibraryUnit::Abilities::AttackWithElement::None,
    "fire"   ,  LibraryUnit::Abilities::AttackWithElement::Fire,
    "earth"  ,  LibraryUnit::Abilities::AttackWithElement::Earth,
    "air"    ,  LibraryUnit::Abilities::AttackWithElement::Air,
    "ice"    ,  LibraryUnit::Abilities::AttackWithElement::Ice,
    "mind"   ,  LibraryUnit::Abilities::AttackWithElement::Mind,
    "magic"  ,  LibraryUnit::Abilities::AttackWithElement::Magic,
    "undead" ,  LibraryUnit::Abilities::AttackWithElement::Undead
    );

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Abilities::StatBonus>{
    Field("morale"           , &LibraryUnit::Abilities::StatBonus::morale),
    Field("luck"             , &LibraryUnit::Abilities::StatBonus::luck),
    Field("manaCost"         , &LibraryUnit::Abilities::StatBonus::manaCost),
    Field("rngMult"          , &LibraryUnit::Abilities::StatBonus::rngMult),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Abilities::ExtraDamage>{
    Field("enemies"           , &LibraryUnit::Abilities::ExtraDamage::enemies),
    Field("bonus"             , &LibraryUnit::Abilities::ExtraDamage::damageBonus),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Abilities::CastOnHit>{
    Field("params"           , &LibraryUnit::Abilities::CastOnHit::params),
    Field("melee"            , &LibraryUnit::Abilities::CastOnHit::melee),
    Field("ranged"           , &LibraryUnit::Abilities::CastOnHit::ranged),
    Field("chance"           , &LibraryUnit::Abilities::CastOnHit::chance),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Abilities::FixedCast>{
    Field("params"           , &LibraryUnit::Abilities::FixedCast::params),
    Field("count"            , &LibraryUnit::Abilities::FixedCast::count),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Abilities>{
    Field("type"               , &LibraryUnit::Abilities::type),
    Field("nonLivingType"      , &LibraryUnit::Abilities::nonLivingType),
    Field("splash"             , &LibraryUnit::Abilities::splashType),
    Field("splashFriendlyFire" , &LibraryUnit::Abilities::splashFriendlyFire),
    Field("splashButtons"      , &LibraryUnit::Abilities::splashButtons),
    Field("splashSpell"        , &LibraryUnit::Abilities::splashSpell),
    Field("splashElement"      , &LibraryUnit::Abilities::splashElement),
    Field("retaliations"       , &LibraryUnit::Abilities::maxRetaliations),
    Field("chargeAttack"       , &LibraryUnit::Abilities::chargeAttack),
    Field("noPenalty"          , &LibraryUnit::Abilities::disabledPenalties),
    Field("squadBonus"         , &LibraryUnit::Abilities::squadBonus),
    Field("opponentBonus"      , &LibraryUnit::Abilities::opponentBonus),

    Field("extraDamage"                     , &LibraryUnit::Abilities::extraDamage),
    Field("attackWithElement"               , &LibraryUnit::Abilities::attackWithElement),
    Field("vulnerableAgainstElement"        , &LibraryUnit::Abilities::vulnerableAgainstElement),

    Field("reduceTargetDefense"             , &LibraryUnit::Abilities::reduceTargetDefense),
    Field("reduceAttackerAttack"            , &LibraryUnit::Abilities::reduceAttackerAttack),
    Field("minimalLuckLevel"                , &LibraryUnit::Abilities::minimalLuckLevel),
    Field("minimalMoraleLevel"              , &LibraryUnit::Abilities::minimalMoraleLevel),
    Field("magicReduce"                     , &LibraryUnit::Abilities::magicReduce),
    Field("magicOppSuccessChance"           , &LibraryUnit::Abilities::magicOppSuccessChance),
    Field("magicOppSuccessChanceNeighbours" , &LibraryUnit::Abilities::magicOppSuccessChanceNeighbours),

    Field("vulnerable"         , &LibraryUnit::Abilities::vulnerable),
    Field("vulnerableBonus"    , &LibraryUnit::Abilities::vulnerableBonus),
    Field("immunes"            , &LibraryUnit::Abilities::immunes),
    Field("immuneBreakable"    , &LibraryUnit::Abilities::immuneBreakable),

    Field("regenerate"                      , &LibraryUnit::Abilities::regenerate),

    Field("castsOnHit"                      , &LibraryUnit::Abilities::castsOnHit),
    Field("fixedCast"                       , &LibraryUnit::Abilities::fixedCast),

    Field("weekIncome"                      , &LibraryUnit::Abilities::weekIncome),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::HeroStackSize>{
    Field("min", &LibraryUnit::HeroStackSize::min),
    Field("max", &LibraryUnit::HeroStackSize::max),
};


template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit::Presentation>{
    Field("spriteBattle"         , &LibraryUnit::Presentation::spriteBattle),
    Field("portrait"             , &LibraryUnit::Presentation::portrait),
    Field("portraitSM"           , &LibraryUnit::Presentation::portraitSmall),
    Field("soundId"              , &LibraryUnit::Presentation::soundId),
    Field("spriteProjectile"     , &LibraryUnit::Presentation::spriteProjectile),
    Field("soundHasShoot"        , &LibraryUnit::Presentation::soundHasShoot),
    Field("soundHasMovementStart", &LibraryUnit::Presentation::soundHasMovementStart),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryUnit>{
    Field("untranslatedName"     , &LibraryUnit::untranslatedName),
    Field("legacyId"             , &LibraryUnit::legacyId),
    Field("primary"              , &LibraryUnit::primary),
    Field("factionId"            , &LibraryUnit::faction),
    Field("level"                , &LibraryUnit::level),
    Field("growth"               , &LibraryUnit::growth),
    Field("countWithHeroBase"    , &LibraryUnit::countWithHeroBase),
    Field("cost"                 , &LibraryUnit::cost),
    Field("value"                , &LibraryUnit::value),
    Field("upgrades"             , &LibraryUnit::upgrades),
    Field("bmArtifact"           , &LibraryUnit::battleMachineArtifact), //(metadata("optional", true))
    Field("abilities"            , &LibraryUnit::abilities),
    Field("pres"                 , &LibraryUnit::presentationParams),
    Field("mapObjectDef"         , &LibraryUnit::mapObjectDef),
    Field("traits"               , &LibraryUnit::traits),
};

// ------------------------------------------------------------------------------------------
template<>
inline constexpr const auto EnumTraits::s_valueMapping<ArtifactSlotType> = EnumTraits::make(
    ArtifactSlotType::Invalid,
    ""       ,  ArtifactSlotType::Invalid,
    "sword"  ,  ArtifactSlotType::Sword,
    "shield" ,  ArtifactSlotType::Shield,
    "helm"   ,  ArtifactSlotType::Helm,
    "torso"  ,  ArtifactSlotType::Torso,
    "ring"   ,  ArtifactSlotType::Ring,
    "neck"   ,  ArtifactSlotType::Neck,
    "boots"  ,  ArtifactSlotType::Boots,
    "cape"   ,  ArtifactSlotType::Cape,
    "misc"   ,  ArtifactSlotType::Misc,

    "bmAmmo" ,  ArtifactSlotType::BmAmmo,
    "bmShoot",  ArtifactSlotType::BmShoot,
    "bmTent" ,  ArtifactSlotType::BmTent,
    "bagOnly",  ArtifactSlotType::BagOnly,

    "ring1"   ,  ArtifactSlotType::Ring1,
    "misc1"   ,  ArtifactSlotType::Misc1,
    "misc2"   ,  ArtifactSlotType::Misc2,
    "misc3"   ,  ArtifactSlotType::Misc3,
    "misc4"   ,  ArtifactSlotType::Misc4
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryArtifact::TreasureClass> = EnumTraits::make(
    LibraryArtifact::TreasureClass::Special,
    "treasure" ,   LibraryArtifact::TreasureClass::Treasure,
    "minor"    ,   LibraryArtifact::TreasureClass::Minor,
    "major"    ,   LibraryArtifact::TreasureClass::Major,
    "relic"    ,   LibraryArtifact::TreasureClass::Relic,
    "unique"   ,   LibraryArtifact::TreasureClass::Unique,
    "complex"  ,   LibraryArtifact::TreasureClass::Complex,
    "bm"       ,   LibraryArtifact::TreasureClass::BattleMachine,
    "scroll"   ,   LibraryArtifact::TreasureClass::Scroll,
    "special"  ,   LibraryArtifact::TreasureClass::Special
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryArtifact::OrderCategory> = EnumTraits::make(
    LibraryArtifact::OrderCategory::Special,
    "special"  ,  LibraryArtifact::OrderCategory::Special,
    "stats"    ,  LibraryArtifact::OrderCategory::Stats  ,
    "skills"   ,  LibraryArtifact::OrderCategory::Skills ,
    "magic"    ,  LibraryArtifact::OrderCategory::Magic  ,
    "income"   ,  LibraryArtifact::OrderCategory::Income ,
    "misc"     ,  LibraryArtifact::OrderCategory::Misc   ,
    "complex"  ,  LibraryArtifact::OrderCategory::Complex,
    "scrolls"  ,  LibraryArtifact::OrderCategory::Scrolls
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryArtifact::SpecialEffect> = EnumTraits::make(
    LibraryArtifact::SpecialEffect::None,
    "none"               ,  LibraryArtifact::SpecialEffect::None               ,
    "neutralDiplomacy"   ,  LibraryArtifact::SpecialEffect::NeutralDiplomacy   ,
    "factionsAlliance"   ,  LibraryArtifact::SpecialEffect::FactionsAlliance   ,
    "alwaysFly"          ,  LibraryArtifact::SpecialEffect::AlwaysFly          ,
    "alwaysWaterWalk"    ,  LibraryArtifact::SpecialEffect::AlwaysWaterWalk    ,
    "resurrectFangarms"  ,  LibraryArtifact::SpecialEffect::ResurrectFangarms  ,
    "extendedNecromancy" ,  LibraryArtifact::SpecialEffect::ExtendedNecromancy ,
    "dragonsBuffs"       ,  LibraryArtifact::SpecialEffect::DragonsBuffs       ,
    "disableSurrender"   ,  LibraryArtifact::SpecialEffect::DisableSurrender   ,
    "noDamageWhirl"      ,  LibraryArtifact::SpecialEffect::NoDamageWhirl      ,
    "noTerrainPenalty"   ,  LibraryArtifact::SpecialEffect::NoTerrainPenalty   ,
    "breakImmunities"    ,  LibraryArtifact::SpecialEffect::BreakImmunities    ,
    "permanentDeath"     ,  LibraryArtifact::SpecialEffect::PermanentDeath     
    );

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryArtifact::Presentation>{
    Field("iconStash"       , &LibraryArtifact::Presentation::iconStash),
    Field("iconBonus"       , &LibraryArtifact::Presentation::iconBonus),
    Field("order"           , &LibraryArtifact::Presentation::order),
    Field("orderGroup"      , &LibraryArtifact::Presentation::orderGroup),
    Field("orderCategory"   , &LibraryArtifact::Presentation::orderCategory),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryArtifact>{
    Field("slot"            , &LibraryArtifact::slot),
    Field("class"           , &LibraryArtifact::treasureClass),
    Field("special"         , &LibraryArtifact::special),
    Field("calc"            , &LibraryArtifact::calc),
    Field("untranslatedName", &LibraryArtifact::untranslatedName),
    Field("legacyId"        , &LibraryArtifact::legacyId),
    Field("value"           , &LibraryArtifact::value),
    Field("provideSpells"   , &LibraryArtifact::provideSpells),
    Field("protectSpells"   , &LibraryArtifact::protectSpells),
    Field("forbidSpells"    , &LibraryArtifact::forbidSpells),
    Field("spellCasts"      , &LibraryArtifact::spellCasts),
    Field("parts"           , &LibraryArtifact::parts),
    Field("noPenalty"       , &LibraryArtifact::disabledPenalties),
    Field("bmUnit"          , &LibraryArtifact::battleMachineUnit), //(metadata("optional", true))
    Field("pres"            , &LibraryArtifact::presentationParams),
    Field("mapObjectDef"    , &LibraryArtifact::mapObjectDef),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<ArtifactRewardAmount>{
    Field("artifacts", &ArtifactRewardAmount::artifacts),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<ArtifactRewardAmount::SingleReward>{
    Field("class", &ArtifactRewardAmount::SingleReward::treasureClass),
    Field("n"    , &ArtifactRewardAmount::SingleReward::count),
};
// ------------------------------------------------------------------------------------------

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryDwelling>{
    Field("creatureIds", &LibraryDwelling::creatureIds),
    Field("mapObjectDefs", &LibraryDwelling::mapObjectDefs),
};
// ------------------------------------------------------------------------------------------

template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryHero::Presentation::Gender> = EnumTraits::make(
    LibraryHero::Presentation::Gender::Unspec,
    ""       ,   LibraryHero::Presentation::Gender::Unspec,
    "f"      ,   LibraryHero::Presentation::Gender::Female,
    "m"      ,   LibraryHero::Presentation::Gender::Male
    );


template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibraryHeroSpec::Type> = EnumTraits::make(
    LibraryHeroSpec::Type::None,
    ""               ,   LibraryHeroSpec::Type::None,
    "income"         ,   LibraryHeroSpec::Type::Income,
    "unit"           ,   LibraryHeroSpec::Type::Unit,
    "unitUpgrade"    ,   LibraryHeroSpec::Type::UnitUpgrade,
    "unitNonStd"     ,   LibraryHeroSpec::Type::UnitNonStd,
    "spell"          ,   LibraryHeroSpec::Type::Spell,
    "skill"          ,   LibraryHeroSpec::Type::Skill,
    "specialBallista",   LibraryHeroSpec::Type::SpecialBallista,
    "specialCannon"  ,   LibraryHeroSpec::Type::SpecialCannon,
    "specialDragons" ,   LibraryHeroSpec::Type::SpecialDragons,
    "specialSpeed"   ,   LibraryHeroSpec::Type::SpecialSpeed

    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryHeroSpec::Presentation>{
    Field("icon"             , &LibraryHeroSpec::Presentation::icon),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryHeroSpec>{
    Field("type"             , &LibraryHeroSpec::type),
    Field("untranslatedName" , &LibraryHeroSpec::untranslatedName),
    Field("dayIncome"        , &LibraryHeroSpec::dayIncome),
    Field("unitId"           , &LibraryHeroSpec::unit),
    Field("skillId"          , &LibraryHeroSpec::skill),
    Field("spellId"          , &LibraryHeroSpec::spell),
    Field("pres"             , &LibraryHeroSpec::presentationParams),
};
// ------------------------------------------------------------------------------------------
template<>
inline constexpr const std::tuple MetaInfo::s_fields<SkillHeroItem>{
    Field("skillId"      , &SkillHeroItem::skill),
    Field("level"        , &SkillHeroItem::level),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryHero::StartStack>{
    Field("id"       , &LibraryHero::StartStack::unit),
    Field("stackSize", &LibraryHero::StartStack::stackSize),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryHero::Presentation>{
    Field("portrait"          , &LibraryHero::Presentation::portrait),
    Field("portraitSmall"     , &LibraryHero::Presentation::portraitSmall),
    Field("order"             , &LibraryHero::Presentation::order),
    Field("gender"            , &LibraryHero::Presentation::gender),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryHero>{
    Field("untranslatedName"  , &LibraryHero::untranslatedName),
    Field("legacyId"          , &LibraryHero::legacyId),
    Field("factionId"         , &LibraryHero::faction),
    Field("isWarrior"         , &LibraryHero::isWarrior),
    Field("specId"            , &LibraryHero::spec),
    Field("secondarySkills"   , &LibraryHero::secondarySkills), 
    Field("startSpellId"      , &LibraryHero::startSpell), //(metadata("optional", true))
    Field("startStacks"       , &LibraryHero::startStacks),
    Field("pres"              , &LibraryHero::presentationParams),
};
// ------------------------------------------------------------------------------------------
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySpell::Type> = EnumTraits::make(
    LibrarySpell::Type::Temp,
    "temp"       ,   LibrarySpell::Type::Temp,
    "offensive"  ,   LibrarySpell::Type::Offensive,
    "special"    ,   LibrarySpell::Type::Special,
    "summon"     ,   LibrarySpell::Type::Summon,
    "rising"     ,   LibrarySpell::Type::Rising,
    "adventure"  ,   LibrarySpell::Type::Adventure
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<MagicSchool> = EnumTraits::make(
     MagicSchool::Any,
    "any"      ,   MagicSchool::Any,
    "earth"    ,   MagicSchool::Earth,
    "air"      ,   MagicSchool::Air,
    "water"    ,   MagicSchool::Water,
    "fire"     ,   MagicSchool::Fire
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySpell::TargetClass> = EnumTraits::make(
    LibrarySpell::TargetClass::None,
    ""            ,   LibrarySpell::TargetClass::None,
    "units"       ,   LibrarySpell::TargetClass::Units,
    "land"        ,   LibrarySpell::TargetClass::Land,
    "immediate"   ,   LibrarySpell::TargetClass::Immediate
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySpell::Range> = EnumTraits::make(
    LibrarySpell::Range::Single,
    "single"       ,   LibrarySpell::Range::Single,
    "r1"           ,   LibrarySpell::Range::R1,
    "r1NoCenter"   ,   LibrarySpell::Range::R1NoCenter,
    "r2"           ,   LibrarySpell::Range::R2,
    "r3"           ,   LibrarySpell::Range::R3,
    "obstacle2"    ,   LibrarySpell::Range::Obstacle2,
    "obstacle3"    ,   LibrarySpell::Range::Obstacle3,
    "chain4"       ,   LibrarySpell::Range::Chain4,
    "chain5"       ,   LibrarySpell::Range::Chain5,
    "all"          ,   LibrarySpell::Range::All
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySpell::Qualify> = EnumTraits::make(
    LibrarySpell::Qualify::None,
    ""            ,   LibrarySpell::Qualify::None,
    "good"        ,   LibrarySpell::Qualify::Good,
    "bad"         ,   LibrarySpell::Qualify::Bad
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySpell::Tag> = EnumTraits::make(
    LibrarySpell::Tag::Mind,
    "mind"        ,  LibrarySpell::Tag::Mind,
    "vision"      ,  LibrarySpell::Tag::Vision,
    "ice"         ,  LibrarySpell::Tag::Ice,
    "lightning"   ,  LibrarySpell::Tag::Lightning,
    "airElem"     ,  LibrarySpell::Tag::AirElem,
    "fireElem"    ,  LibrarySpell::Tag::FireElem
    );
template<>
inline constexpr const auto EnumTraits::s_valueMapping<LibrarySpell::EndCondition> = EnumTraits::make(
    LibrarySpell::EndCondition::Time,
    "time"          ,  LibrarySpell::EndCondition::Time,
    "getHit"        ,  LibrarySpell::EndCondition::GetHit,
    "makingAttack"  ,  LibrarySpell::EndCondition::MakingAttack
    );
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibrarySpell::Presentation>{
    Field("bottomAnimation"     , &LibrarySpell::Presentation::bottomAnimation),
    Field("projectile"          , &LibrarySpell::Presentation::projectile),
    Field("bottomPosition"      , &LibrarySpell::Presentation::bottomPosition),
    Field("animationOnMainPos"  , &LibrarySpell::Presentation::animationOnMainPosition),
    Field("iconBonus"           , &LibrarySpell::Presentation::iconBonus),
    Field("iconInt"             , &LibrarySpell::Presentation::iconInt),
    Field("iconTrans"           , &LibrarySpell::Presentation::iconTrans),
    Field("iconScroll"          , &LibrarySpell::Presentation::iconScroll),
    Field("configOrder"         , &LibrarySpell::Presentation::configOrder),
    Field("animation"           , &LibrarySpell::Presentation::animation),
    Field("sound"               , &LibrarySpell::Presentation::sound),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibrarySpell>{
    Field("untranslatedName"      , &LibrarySpell::untranslatedName),
    Field("legacyId"              , &LibrarySpell::legacyId),
    Field("isTeachable"           , &LibrarySpell::isTeachable),
    Field("type"                  , &LibrarySpell::type),
    Field("qualify"               , &LibrarySpell::qualify),
    Field("school"                , &LibrarySpell::school),
    Field("tags"                  , &LibrarySpell::tags),
    Field("level"                 , &LibrarySpell::level),
    Field("manaCost"              , &LibrarySpell::manaCost),
    Field("indistinctive"         , &LibrarySpell::indistinctive),
    Field("targetClass"           , &LibrarySpell::targetClass),
    Field("counterSpells"         , &LibrarySpell::counterSpells),
    Field("calc"                  , &LibrarySpell::calcScript),
    Field("filter"                , &LibrarySpell::filterScript),
    Field("rangeByLevel"          , &LibrarySpell::rangeByLevel),
    Field("endConditions"         , &LibrarySpell::endConditions),
    Field("retaliationWhenCancel" , &LibrarySpell::retaliationWhenCancel),
    Field("summonUnit"            , &LibrarySpell::summonUnit), // (metadata("optional", true))
    Field("pres"                  , &LibrarySpell::presentationParams),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<SpellCastParams>{
    Field("spell"       , &SpellCastParams::spell),
    Field("sp"          , &SpellCastParams::spellPower),
    Field("level"       , &SpellCastParams::skillLevel),
    Field("spPerUnit"   , &SpellCastParams::spPerUnit),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<SpellFilter>{
    Field("onlySpells"   , &SpellFilter::onlySpells),
    Field("notSpells"    , &SpellFilter::notSpells),
    Field("levels"       , &SpellFilter::levels),
    Field("schools"      , &SpellFilter::schools),
    Field("tags"         , &SpellFilter::tags),
    Field("teachableOnly", &SpellFilter::teachableOnly),
    Field("all"          , &SpellFilter::all),
};

// ------------------------------------------------------------------------------------------

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryMapBank::Reward>{
    Field("resources"  , &LibraryMapBank::Reward::resources),
    Field("unit"       , &LibraryMapBank::Reward::unit),
    Field("artifacts"  , &LibraryMapBank::Reward::artifacts),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<UnitWithCount>{
    Field("id"        , &UnitWithCount::unit),
    Field("n"         , &UnitWithCount::count),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryMapBank::Variant>{
    Field("name"        , &LibraryMapBank::Variant::name),
    Field("rewardIndex" , &LibraryMapBank::Variant::rewardIndex),
    Field("guards"      , &LibraryMapBank::Variant::guards),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryMapBank::Presentation>{
    Field("order"              , &LibraryMapBank::Presentation::order),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryMapBank>{
    Field("mapObjectDef"        , &LibraryMapBank::mapObjectDef),
    Field("untranslatedName"    , &LibraryMapBank::untranslatedName),
    Field("legacyId"            , &LibraryMapBank::legacyId),
    Field("variants"            , &LibraryMapBank::variants),
    Field("rewards"             , &LibraryMapBank::rewards),
    Field("pres"                , &LibraryMapBank::presentationParams),
    Field("fieldLayout"         , &LibraryMapBank::fieldLayout),
};
// ------------------------------------------------------------------------------------------
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryMapObstacle>{
    Field("legacyId"            , &LibraryMapObstacle::legacyId),
    Field("mapObjectDef"        , &LibraryMapObstacle::mapObjectDef),
};
// ------------------------------------------------------------------------------------------
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryGameRules::RngRules>{
    Field("positiveChances"      , &LibraryGameRules::RngRules::positiveChances),
    Field("negativeChances"      , &LibraryGameRules::RngRules::negativeChances),
    Field("maxEffectiveValue"    , &LibraryGameRules::RngRules::maxEffectiveValue),
    Field("minEffectiveValue"    , &LibraryGameRules::RngRules::minEffectiveValue),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryGameRules::PhysicalConst>{
    Field("maxEffectiveAttack"      , &LibraryGameRules::PhysicalConst::maxEffectiveAttack),
    Field("maxEffectiveDefense"     , &LibraryGameRules::PhysicalConst::maxEffectiveDefense),
    Field("attackValue"             , &LibraryGameRules::PhysicalConst::attackValue),
    Field("defenseValue"            , &LibraryGameRules::PhysicalConst::defenseValue),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryGameRules::Limits>{
    Field("stacks"           , &LibraryGameRules::Limits::stacks),
    Field("maxHeroLevel"     , &LibraryGameRules::Limits::maxHeroLevel),
    Field("maxHeroAd"        , &LibraryGameRules::Limits::maxHeroAd),
    Field("maxHeroMagic"     , &LibraryGameRules::Limits::maxHeroMagic),
    Field("maxUnitAd"        , &LibraryGameRules::Limits::maxUnitAd),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryGameRules>{
    Field("luck"            , &LibraryGameRules::luck),
    Field("morale"          , &LibraryGameRules::morale),
    Field("physicalConst"   , &LibraryGameRules::physicalConst),
    Field("limits"          , &LibraryGameRules::limits),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryObjectDef>{
    Field("legacyId"       , &LibraryObjectDef::legacyId),
    Field("defFile"        , &LibraryObjectDef::defFile),
    Field("blockMap"       , &LibraryObjectDef::blockMap),
    Field("visitMap"       , &LibraryObjectDef::visitMap),
    Field("terrainsHard"   , &LibraryObjectDef::terrainsHard),
    Field("terrainsSoft"   , &LibraryObjectDef::terrainsSoft),
    Field("objId"          , &LibraryObjectDef::objId),
    Field("subId"          , &LibraryObjectDef::subId),
    Field("type"           , &LibraryObjectDef::type),
    Field("priority"       , &LibraryObjectDef::priority),
};

template<>
inline constexpr const auto EnumTraits::s_valueMapping<FieldLayout>  = EnumTraits::make(
    FieldLayout::Standard,
    "std"          ,   FieldLayout::Standard,
    "obj"          ,   FieldLayout::Object,
    "churchyard1"  ,   FieldLayout::Churchyard1,
    "churchyard2"  ,   FieldLayout::Churchyard2,
    "ruins"        ,   FieldLayout::Ruins,
    "spit"         ,   FieldLayout::Spit
    );
// clang-format on

template<>
inline constexpr const bool MetaInfo::s_useFromString<BonusRatio>{ true };
template<>
BonusRatio MetaInfo::fromString(const std::string& value);

template<>
inline constexpr const std::tuple MetaInfo::s_fields<BonusRatio>{
    Field("num", &BonusRatio::m_num),
    Field("denom", &BonusRatio::m_denom),
};

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<LibraryUnit::Traits>{ true };
template<>
bool MetaInfo::transformTree<LibraryUnit::Traits>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<ResourceAmount>{ true };
template<>
bool MetaInfo::transformTree<ResourceAmount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<ArtifactRewardAmount>{ true };
template<>
bool MetaInfo::transformTree<ArtifactRewardAmount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<UnitWithCount>{ true };
template<>
bool MetaInfo::transformTree<UnitWithCount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<LibraryHero::StartStack>{ true };
template<>
bool MetaInfo::transformTree<LibraryHero::StartStack>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_isStringMap<LibraryFactionHeroClass::SkillWeights>{ true };
template<>
inline constexpr const bool MetaInfo::s_isStringMap<LibraryFactionHeroClass::PrimaryWeights>{ true };

}
