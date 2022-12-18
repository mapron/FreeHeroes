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

#include "Reflection/EnumTraitsMacro.hpp"
#include "Reflection/MetaInfoMacro.hpp"

#include "StringUtils.hpp"

#include "CoreLogicExport.hpp"

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

ENUM_REFLECTION_PAIRED(HeroPrimaryParamType,
    Attack,
    "attack" ,  Attack,
    "defense",  Defense,
    "sp"     ,  SpellPower,
    "int"    ,  Intelligence,
    "mana"   ,  Mana,
    "exp"    ,  Experience
)

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

ENUM_REFLECTION_PAIRED(LibraryTerrain::BorderType,
    Invalid,
    "TL"               ,   TL,
    "L"                ,   L,
    "T"                ,   T,
    "BR"               ,   BR,
    "TLS"              ,   TLS,
    "BRS"              ,   BRS,
    "ThreeWay_DD"      ,   ThreeWay_DD,
    "ThreeWay_DS"      ,   ThreeWay_DS,
    "ThreeWay_SS"      ,   ThreeWay_SS,
    "ThreeWay_RD_BLS"  ,   ThreeWay_RD_BLS,
    "ThreeWay_BD_TRS"  ,   ThreeWay_BD_TRS,
    "ThreeWay_TRD_BRS" ,   ThreeWay_TRD_BRS,
    "ThreeWay_BRS_BLD" ,   ThreeWay_BRS_BLD,
    "ThreeWay_RS_BD"   ,   ThreeWay_RS_BD,
    "ThreeWay_BS_RD"   ,   ThreeWay_BS_RD,
    "Center"           ,   Center
)

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
    Field("minesDefs"        , &LibraryResource::minesDefs),
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
ENUM_REFLECTION_PAIRED(LibraryFaction::Alignment,
    Special,
    "good"       ,  Good,
    "evil"       ,  Evil,
    "neutral"    ,  Neutral,
    "independent",  Independent,
    "special"    ,  Special
)

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

ENUM_REFLECTION_PAIRED(LibrarySecondarySkill::HandlerType,
    Stat,
    "stat"       ,  Stat,
    "special"    ,  Special,
    "wisdom"     ,  Wisdom,
    "school"     ,  School
)

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
ENUM_REFLECTION_PAIRED(UnitType,
    Unknown,
    "living"     , Living,
    "nonLiving"  , NonLiving,
    "siege"      , SiegeMachine,
    "arrowTower" , ArrowTower,
    "wall"       , Wall
)
ENUM_REFLECTION_PAIRED(UnitNonLivingType,
    None,
    "undead"    ,  Undead,
    "golem"     ,  Golem,
    "gargoyle"  ,  Gargoyle,
    "elemental" ,  Elemental,
    "bm"        ,  BattleMachine
)

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

ENUM_REFLECTION_PAIRED(LibraryUnit::Abilities::SplashAttack,
    None,
    "dragon"     ,  Dragon,
    "neighbours" ,  Neighbours,
    "sides"      ,  Sides,
    "ranged"     ,  Ranged
)
ENUM_REFLECTION_PAIRED(RangeAttackPenalty,
    Melee,
    "melee"    , Melee,
    "distance" , Distance,
    "obstacle" , Obstacle,
    "blocked"  , Blocked
)
ENUM_REFLECTION_PAIRED(LibraryUnit::Abilities::AttackWithElement,
    None,
    "fire"   ,  Fire,
    "earth"  ,  Earth,
    "air"    ,  Air,
    "ice"    ,  Ice,
    "mind"   ,  Mind,
    "magic"  ,  Magic,
    "undead" ,  Undead
)

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
ENUM_REFLECTION_PAIRED(ArtifactSlotType,
    Invalid,
    "sword"   ,  Sword,
    "shield"  ,  Shield,
    "helm"    ,  Helm,
    "torso"   ,  Torso,
    "ring"    ,  Ring,
    "neck"    ,  Neck,
    "boots"   ,  Boots,
    "cape"    ,  Cape,
    "misc"    ,  Misc,
    
    "bmAmmo"  ,  BmAmmo,
    "bmShoot" ,  BmShoot,
    "bmTent"  ,  BmTent,
    "bagOnly" ,  BagOnly,
    
    "ring1"   ,  Ring1,
    "misc1"   ,  Misc1,
    "misc2"   ,  Misc2,
    "misc3"   ,  Misc3,
    "misc4"   ,  Misc4
)
ENUM_REFLECTION_PAIRED(LibraryArtifact::TreasureClass,
    Special,
    "treasure" ,  Treasure,
    "minor"    ,  Minor,
    "major"    ,  Major,
    "relic"    ,  Relic,
    "unique"   ,  Unique,
    "complex"  ,  Complex,
    "bm"       ,  BattleMachine,
    "scroll"   ,  Scroll,
    "special"  ,  Special
)
ENUM_REFLECTION_PAIRED(LibraryArtifact::Tag,
    Invalid,
    "stats"      ,  Stats,
    "control"    ,  Control
)
ENUM_REFLECTION_PAIRED(LibraryArtifact::OrderCategory,
    Special,
    "special"  ,  Special,
    "stats"    ,  Stats  ,
    "skills"   ,  Skills ,
    "magic"    ,  Magic  ,
    "income"   ,  Income ,
    "misc"     ,  Misc   ,
    "complex"  ,  Complex,
    "scrolls"  ,  Scrolls
)

ENUM_REFLECTION_PAIRED(LibraryArtifact::SpecialEffect,
    None,
    "none"               ,  None               ,
    "neutralDiplomacy"   ,  NeutralDiplomacy   ,
    "factionsAlliance"   ,  FactionsAlliance   ,
    "alwaysFly"          ,  AlwaysFly          ,
    "alwaysWaterWalk"    ,  AlwaysWaterWalk    ,
    "resurrectFangarms"  ,  ResurrectFangarms  ,
    "extendedNecromancy" ,  ExtendedNecromancy ,
    "dragonsBuffs"       ,  DragonsBuffs       ,
    "disableSurrender"   ,  DisableSurrender   ,
    "noDamageWhirl"      ,  NoDamageWhirl      ,
    "noTerrainPenalty"   ,  NoTerrainPenalty   ,
    "breakImmunities"    ,  BreakImmunities    ,
    "permanentDeath"     ,  PermanentDeath     
)

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
    Field("tags"            , &LibraryArtifact::tags),
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
inline constexpr const std::tuple MetaInfo::s_fields<ArtifactFilter>{
    Field("only"     , &ArtifactFilter::onlyArtifacts),
    Field("not"      , &ArtifactFilter::notArtifacts),
    Field("classes"  , &ArtifactFilter::classes),
    Field("tags"     , &ArtifactFilter::tags),
    Field("all"      , &ArtifactFilter::all),
};

// ------------------------------------------------------------------------------------------

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryDwelling>{
    Field("creatureIds", &LibraryDwelling::creatureIds),
    Field("mapObjectDefs", &LibraryDwelling::mapObjectDefs),
};
// ------------------------------------------------------------------------------------------
ENUM_REFLECTION_PAIRED(LibraryHero::Presentation::Gender,
    Unspec,
    "f" ,   Female,
    "m" ,   Male
)
ENUM_REFLECTION_PAIRED(LibraryHeroSpec::Type,
    None,
    "income"         ,   Income,
    "unit"           ,   Unit,
    "unitUpgrade"    ,   UnitUpgrade,
    "unitNonStd"     ,   UnitNonStd,
    "spell"          ,   Spell,
    "skill"          ,   Skill,
    "specialBallista",   SpecialBallista,
    "specialCannon"  ,   SpecialCannon,
    "specialDragons" ,   SpecialDragons,
    "specialSpeed"   ,   SpecialSpeed
)

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
ENUM_REFLECTION_PAIRED(LibrarySpell::Type,
    Temp,
    "temp"       ,  Temp,
    "offensive"  ,  Offensive,
    "special"    ,  Special,
    "summon"     ,  Summon,
    "rising"     ,  Rising,
    "adventure"  ,  Adventure
)
ENUM_REFLECTION_PAIRED(MagicSchool,
    Any,
    "any"      ,  Any,
    "earth"    ,  Earth,
    "air"      ,  Air,
    "water"    ,  Water,
    "fire"     ,  Fire
)
ENUM_REFLECTION_PAIRED(LibrarySpell::TargetClass,
    None,
    "units"       ,   Units,
    "land"        ,   Land,
    "immediate"   ,   Immediate
)
ENUM_REFLECTION_PAIRED(LibrarySpell::Range,
    Single,
    "single"       ,   Single,
    "r1"           ,   R1,
    "r1NoCenter"   ,   R1NoCenter,
    "r2"           ,   R2,
    "r3"           ,   R3,
    "obstacle2"    ,   Obstacle2,
    "obstacle3"    ,   Obstacle3,
    "chain4"       ,   Chain4,
    "chain5"       ,   Chain5,
    "all"          ,   All
)
ENUM_REFLECTION_PAIRED(LibrarySpell::Qualify,
    None,
    "good"        ,   Good,
    "bad"         ,   Bad
)
ENUM_REFLECTION_PAIRED(LibrarySpell::Tag,
    Mind,
    "mind"        ,  Mind,
    "vision"      ,  Vision,
    "ice"         ,  Ice,
    "lightning"   ,  Lightning,
    "airElem"     ,  AirElem,
    "fireElem"    ,  FireElem
)
ENUM_REFLECTION_PAIRED(LibrarySpell::EndCondition,
    Time,
    "time"          ,  Time,
    "getHit"        ,  GetHit,
    "makingAttack"  ,  MakingAttack
)

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
inline constexpr const std::tuple MetaInfo::s_fields<Reward>{
    Field("resources"   , &Reward::resources),
    Field("units"       , &Reward::units),
    Field("artifacts"   , &Reward::artifacts),
    Field("exp"         , &Reward::gainedExp),
    Field("mana"        , &Reward::manaDiff),
    Field("rng"         , &Reward::rngBonus),
    Field("stat"        , &Reward::statBonus),
    Field("secSkills"   , &Reward::secSkills),
    Field("spells"      , &Reward::spells),
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

STRUCT_REFLECTION_PAIRED(LibraryMapBank,
    "mapObjectDefs"       , mapObjectDefs,
    "untranslatedName"    , untranslatedName,
    "legacyId"            , legacyId,
    "variants"            , variants,
    "rewards"             , rewards,
    "pres"                , presentationParams,
    "fieldLayout"         , fieldLayout
    )

ENUM_REFLECTION_STRINGIY(LibraryMapObstacle::Type, Invalid,
    BRUSH,
    BUSH,
    CACTUS,
    CANYON,
    CRATER,
    DEAD_VEGETATION,
    FLOWERS,
    FROZEN_LAKE,
    HEDGE,
    HILL,

    HOLE,
    KELP,
    LAKE,
    LAVA_FLOW,
    LAVA_LAKE,
    MUSHROOMS,
    LOG,
    MANDRAKE,
    MOSS,
    MOUND,
    MOUNTAIN,
    OAK_TREES,
    OUTCROPPING,
    PINE_TREES,
    PLANT,

    NON_BLOCKING_DECORATION,

    RIVER_DELTA,

    ROCK,
    SAND_DUNE,
    SAND_PIT,
    SHRUB,
    SKULL,
    STALAGMITE,
    STUMP,
    TAR_PIT,
    TREES,
    VINE,
    VOLCANIC_VENT,
    VOLCANO,
    WILLOW_TREES,
    YUCCA_TREES,
    REEF,

    DESERT_HILLS,
    DIRT_HILLS,
    GRASS_HILLS,
    ROUGH_HILLS,
    SUBTERRANEAN_ROCKS,
    SWAMP_FOLIAGE,

    CLOVER_FIELD,
    CURSED_GROUND,
    EVIL_FOG,
    FAVORABLE_WINDS,
    FIERY_FIELDS,
    HOLY_GROUNDS,
    LUCID_POOLS,
    MAGIC_CLOUDS,
    MAGIC_PLAINS,
    ROCKLANDS
)

template<>
inline constexpr const std::tuple MetaInfo::s_fields<LibraryMapObstacle>{
    Field("legacyId"            , &LibraryMapObstacle::legacyId),
    Field("mapObjectDef"        , &LibraryMapObstacle::mapObjectDef),
    Field("type"                , &LibraryMapObstacle::type),
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

ENUM_REFLECTION_PAIRED(FieldLayout,
    Standard,
    "std"          , Standard,
    "obj"          , Object,
    "churchyard1"  , Churchyard1,
    "churchyard2"  , Churchyard2,
    "ruins"        , Ruins,
    "spit"         , Spit
)

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
CORELOGIC_EXPORT bool MetaInfo::transformTree<ResourceAmount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<UnitWithCount>{ true };
template<>
CORELOGIC_EXPORT bool MetaInfo::transformTree<UnitWithCount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_useCustomTransform<LibraryHero::StartStack>{ true };
template<>
bool MetaInfo::transformTree<LibraryHero::StartStack>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool MetaInfo::s_isStringMap<LibraryFactionHeroClass::SkillWeights>{ true };
template<>
inline constexpr const bool MetaInfo::s_isStringMap<LibraryFactionHeroClass::PrimaryWeights>{ true };

}
