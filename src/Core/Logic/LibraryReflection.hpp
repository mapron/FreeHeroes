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
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryResource.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryUnit.hpp"
#include "TranslationMap.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

#include "MernelPlatform/StringUtils.hpp"

#include "CoreLogicExport.hpp"

namespace Mernel {
class PropertyTree;
}

namespace Mernel::Reflection {
using namespace FreeHeroes::Core;
// clang-format off

STRUCT_REFLECTION_PAIRED(
    PrimaryRngParams,
    "luck",                        luck,
    "morale",                      morale
)


STRUCT_REFLECTION_PAIRED(
    PrimaryAttackParams,
    "attack",                      attack,
    "defense",                     defense
)


STRUCT_REFLECTION_PAIRED(
    PrimaryMagicParams,
    "sp",                          spellPower,
    "int",                         intelligence
)


STRUCT_REFLECTION_PAIRED(
    DamageDesc,
    "min",                         minDamage,
    "max",                         maxDamage
)

STRUCT_REFLECTION_PAIRED(
    UnitPrimaryParams,
    "dmg",                         dmg,
    "ad",                          ad,
    "maxHealth",                   maxHealth,
    "speed",                       battleSpeed,
    "armySpeed",                   armySpeed,
    "shoots",                      shoots
)

STRUCT_REFLECTION_PAIRED(
    HeroPrimaryParams,
    "ad",                          ad,
    "magic",                       magic
)

ENUM_REFLECTION_PAIRED(HeroPrimaryParamType,
    Attack,
    "attack" ,                    Attack,
    "defense",                    Defense,
    "sp"     ,                    SpellPower,
    "int"    ,                    Intelligence,
    "mana"   ,                    Mana,
    "exp"    ,                    Experience
)


STRUCT_REFLECTION_PAIRED(
    MagicReduce,
    
    "all"      ,                  allMagic,
    "air"      ,                  air   ,
    "earth"    ,                  earth ,
    "fire"     ,                  fire  ,
    "water"    ,                  water 
)

STRUCT_REFLECTION_PAIRED(
    MagicIncrease,

    "all",                        allMagic,
    "air"      ,                  air  ,
    "earth"    ,                  earth,
    "fire"     ,                  fire ,
    "water"    ,                  water
)

STRUCT_REFLECTION_PAIRED(
    RngChanceMultiplier,
    
    "luckPositive",                luckPositive,
    "moralePositive",              moralePositive,
    "luckNegative",                luckNegative,
    "moraleNegative",              moraleNegative
)

// ------------------------------------------------------------------------------------------

template<>
inline constexpr const bool s_isStringMap<LibraryTerrain::Presentation::BTMap>{ true };
ENUM_REFLECTION_STRINGIFY(
    LibraryTerrain::TileBase,
    None,
    None,
    Sand,
    Dirt
    )

ENUM_REFLECTION_STRINGIFY(LibraryTerrain::BorderType,
    Invalid,
    TL,
    L,
    T,
    BR,
    TLS,
    BRS,
    Mixed_DNND,
    Mixed_DNNS,
    Mixed_SNNS,
    Mixed_NDSD,
    Mixed_NSDD,
    Mixed_NDNS,
    Mixed_NNDS,
    Mixed_NSDS,
    Mixed_NDSS,
    Center,
    Special_DDDD,
    Special_SSSS,
    Special_DDDS,
    Special_SSSD,
    Special_NSSD,
    Special_NDDS
)


STRUCT_REFLECTION_STRINGIFY(
    LibraryTerrain::Presentation,
    music,
    backgroundsBattle,
    order,
    defFile,
    dirtBorderTilesOffset,
    sandBorderTilesOffset,
    mixedBorderTilesOffset,
    specialBorderTilesOffset,
    centerTilesOffset,
    centerTilesCount,
    centerTilesClearCount,
    borderCounts,
    borderMixedCounts,
    borderSpecialCounts,
    hasRotations,
    minimapBlocked,
    minimapUnblocked,
    name
)

STRUCT_REFLECTION_PAIRED(
    LibraryTerrain::Bonus,
    
    "rngMult",                     rngMult
)

STRUCT_REFLECTION_PAIRED(
    LibraryTerrain,
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "moveCost",                    moveCost,
    "isObstacle",                  isObstacle,
    "extraLayer",                  extraLayer,
    "bonusAll",                    bonusAll,
    "pres",                        presentationParams,
    "tileBase",                    tileBase,
    "nonUnderground",              nonUnderground
)
// ------------------------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    LibraryResource::Presentation,
    
    "orderKingdom",                orderKingdom,
    "orderCommon",                 orderCommon,
    "icon",                        icon,
    "iconLarge",                   iconLarge,
    "iconTrade",                   iconTrade
)

STRUCT_REFLECTION_PAIRED(
    LibraryResource,
    
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "objectDefs",                  objectDefs,
    "minesDefs",                   minesDefs,
    "pres",                        presentationParams
)


STRUCT_REFLECTION_PAIRED(
    ResourceAmount,
    
    "gold"     ,                  gold   ,
    "wood"     ,                  wood   ,
    "ore"      ,                  ore    ,
    "mercury"  ,                  mercury,
    "sulfur"   ,                  sulfur ,
    "crystal"  ,                  crystal,
    "gems"     ,                  gems   
)

//  -----------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    LibraryFactionHeroClass::Presentation,
    
    "battleSpriteMale",            battleSpriteMale,
    "battleSpriteFemale",          battleSpriteFemale,
    "adventureSpriteMale",         adventureSpriteMale,
    "adventureSpriteFemale",       adventureSpriteFemale,
    "name",                        name
)


STRUCT_REFLECTION_PAIRED(
    LibraryFactionHeroClass,
    
    "id",                          id,
    "startParams",                 startParams,
    "skills",                      secondarySkillWeights,
    "untranslatedName",            untranslatedName,
    "lowLevelIncrease",            lowLevelIncrease,
    "highLevelIncrease",           highLevelIncrease,
    "pres",                        presentationParams
)
ENUM_REFLECTION_PAIRED(LibraryFaction::Alignment,
    Special,
    "good"       ,                 Good,
    "evil"       ,                 Evil,
    "neutral"    ,                 Neutral,
    "independent",                 Independent,
    "special"    ,                 Special
)


STRUCT_REFLECTION_PAIRED(
    LibraryFaction::Presentation,
    
    "goesAfterId",                 goesAfterId,
    "unitBackground",              unitBackground,
    "name",                        name
)

STRUCT_REFLECTION_PAIRED(
    LibraryFaction,
    
    "alignment",                   alignment,
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "warriorClass",                warriorClass,
    "mageClass",                   mageClass,
    "nativeTerrain",               nativeTerrain,
    "objectDefs",                  objectDefs,
    "pres",                        presentationParams
)
 //  -----------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    LibrarySecondarySkill::Presentation::LevelParams,
    
    "iconSmall" ,                  iconSmall,
    "iconMedium",                  iconMedium,
    "iconLarge" ,                  iconLarge
)

ENUM_REFLECTION_PAIRED(LibrarySecondarySkill::HandlerType,
    Stat,
    "stat"       ,                 Stat,
    "special"    ,                 Special,
    "wisdom"     ,                 Wisdom,
    "school"     ,                 School
)


STRUCT_REFLECTION_PAIRED(
    LibrarySecondarySkill::Presentation,
    "levels",                      levels,
    "order",                       order,
    "name",                        name
)

STRUCT_REFLECTION_PAIRED(
    LibrarySecondarySkill,
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "frequencyWarrior",            frequencyWarrior,
    "frequencyMage",               frequencyMage,
    "handler",                     handler,
    "calc",                        calc,
    "pres",                        presentationParams
)
 //  -----------------------------------------------------------------------------
ENUM_REFLECTION_PAIRED(UnitType,
    Unknown,
    "living"     ,                 Living,
    "nonLiving"  ,                 NonLiving,
    "siege"      ,                 SiegeMachine,
    "arrowTower" ,                 ArrowTower,
    "wall"       ,                 Wall
)
ENUM_REFLECTION_PAIRED(UnitNonLivingType,
    None,
    "undead"    ,                  Undead,
    "golem"     ,                  Golem,
    "gargoyle"  ,                  Gargoyle,
    "elemental" ,                  Elemental,
    "bm"        ,                  BattleMachine
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Traits,
    
    "large",                       large,
    "ranged",                      rangeAttack,
    "fly",                         fly,
    "teleport",                    teleport,
    "dblAttack",                   doubleAttack,
    "freeAttack",                  freeAttack,
    "catapultShoot",               canBeCatapult,
    "autoReturn",                  returnAfterAttack
)

ENUM_REFLECTION_PAIRED(LibraryUnit::Abilities::SplashAttack,
    None,
    "dragon"     ,                 Dragon,
    "neighbours" ,                 Neighbours,
    "sides"      ,                 Sides,
    "ranged"     ,                 Ranged
)
ENUM_REFLECTION_PAIRED(RangeAttackPenalty,
    Melee,
    "melee"    ,                   Melee,
    "distance" ,                   Distance,
    "obstacle" ,                   Obstacle,
    "blocked"  ,                   Blocked
)
ENUM_REFLECTION_PAIRED(LibraryUnit::Abilities::AttackWithElement,
    None,
    "fire"   ,                     Fire,
    "earth"  ,                     Earth,
    "air"    ,                     Air,
    "ice"    ,                     Ice,
    "mind"   ,                     Mind,
    "magic"  ,                     Magic,
    "undead" ,                     Undead
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Abilities::StatBonus,
   "morale"           ,            morale,
   "luck"             ,            luck,
   "manaCost"         ,            manaCost,
   "rngMult"          ,            rngMult
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Abilities::ExtraDamage,
    "enemies"           ,          enemies,
    "bonus"             ,          damageBonus
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Abilities::CastOnHit,
    "params"           ,          params,
    "melee"            ,          melee,
    "ranged"           ,          ranged,
    "chance"           ,          chance
)

STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Abilities::FixedCast,
    "params"           ,          params,
    "count"            ,          count
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Abilities,
    "type",                        type,
    "nonLivingType",               nonLivingType,
    "splash",                      splashType,
    "splashFriendlyFire",          splashFriendlyFire,
    "splashButtons",               splashButtons,
    "splashSpell",                 splashSpell,
    "splashElement",               splashElement,
    "retaliations",                maxRetaliations,
    "chargeAttack",                chargeAttack,
    "noPenalty",                   disabledPenalties,
    "squadBonus",                  squadBonus,
    "opponentBonus",               opponentBonus,

    "extraDamage",                 extraDamage,
    "attackWithElement",           attackWithElement,
    "vulnerableAgainstElement",    vulnerableAgainstElement,

    "reduceTargetDefense",         reduceTargetDefense,
    "reduceAttackerAttack",        reduceAttackerAttack,
    "minimalLuckLevel",            minimalLuckLevel,
    "minimalMoraleLevel",          minimalMoraleLevel,
    "magicReduce",                 magicReduce,
    "magicOppSuccessChance",       magicOppSuccessChance,
    "magicOppSuccessChanceNeighbours",  magicOppSuccessChanceNeighbours,

    "vulnerable",                  vulnerable,
    "vulnerableBonus",             vulnerableBonus,
    "immunes",                     immunes,
    "immuneBreakable",             immuneBreakable,

    "regenerate",                  regenerate,

    "castsOnHit",                  castsOnHit,
    "fixedCast",                   fixedCast,

    "weekIncome",                  weekIncome
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit::HeroStackSize,
    "min",                         min,
    "max",                         max
)



STRUCT_REFLECTION_PAIRED(
    LibraryUnit::Presentation,
    "spriteBattle",                spriteBattle,
    "portrait",                    portrait,
    "portraitSM",                  portraitSmall,
    "soundId",                     soundId,
    "spriteProjectile",            spriteProjectile,
    "soundHasShoot",               soundHasShoot,
    "soundHasMovementStart",       soundHasMovementStart,
    "name",                        name
)


STRUCT_REFLECTION_PAIRED(
    LibraryUnit,
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "primary",                     primary,
    "factionId",                   faction,
    "level",                       level,
    "growth",                      growth,
    "countWithHeroBase",           countWithHeroBase,
    "cost",                        cost,
    "value",                       value,
    "guardMult1",                  guardMult1,
    "guardMult100",                guardMult100,
    "upgrades",                    upgrades,
    "bmArtifact",                  battleMachineArtifact,
    "abilities",                   abilities,
    "pres",                        presentationParams,
    "objectDefs",                  objectDefs,
    "traits",                      traits
)

// ------------------------------------------------------------------------------------------
ENUM_REFLECTION_PAIRED(ArtifactSlotType,
    Invalid,
    "sword"   ,                    Sword,
    "shield"  ,                    Shield,
    "helm"    ,                    Helm,
    "torso"   ,                    Torso,
    "ring"    ,                    Ring,
    "neck"    ,                    Neck,
    "boots"   ,                    Boots,
    "cape"    ,                    Cape,
    "misc"    ,                    Misc,
    
    "bmAmmo"  ,                    BmAmmo,
    "bmShoot" ,                    BmShoot,
    "bmTent"  ,                    BmTent,
    "bagOnly" ,                    BagOnly,
    
    "ring1"   ,                    Ring1,
    "misc1"   ,                    Misc1,
    "misc2"   ,                    Misc2,
    "misc3"   ,                    Misc3,
    "misc4"   ,                    Misc4
)
ENUM_REFLECTION_PAIRED(LibraryArtifact::TreasureClass,
    Special,
    "treasure" ,                   Treasure,
    "minor"    ,                   Minor,
    "major"    ,                   Major,
    "relic"    ,                   Relic,
    "unique"   ,                   Unique,
    "complex"  ,                   Complex,
    "bm"       ,                   BattleMachine,
    "scroll"   ,                   Scroll,
    "special"  ,                   Special
)
ENUM_REFLECTION_PAIRED(LibraryArtifact::Tag,
    Invalid,
    "stats"      ,                 Stats,
    "control"    ,                 Control
)
ENUM_REFLECTION_PAIRED(LibraryArtifact::OrderCategory,
    Special,
    "special"  ,                  Special,
    "stats"    ,                  Stats  ,
    "skills"   ,                  Skills ,
    "magic"    ,                  Magic  ,
    "income"   ,                  Income ,
    "misc"     ,                  Misc   ,
    "complex"  ,                  Complex,
    "scrolls"  ,                  Scrolls
)

ENUM_REFLECTION_PAIRED(LibraryArtifact::SpecialEffect,
    None,
    "none"               ,        None               ,
    "neutralDiplomacy"   ,        NeutralDiplomacy   ,
    "factionsAlliance"   ,        FactionsAlliance   ,
    "alwaysFly"          ,        AlwaysFly          ,
    "alwaysWaterWalk"    ,        AlwaysWaterWalk    ,
    "resurrectFangarms"  ,        ResurrectFangarms  ,
    "extendedNecromancy" ,        ExtendedNecromancy ,
    "dragonsBuffs"       ,        DragonsBuffs       ,
    "disableSurrender"   ,        DisableSurrender   ,
    "noDamageWhirl"      ,        NoDamageWhirl      ,
    "noTerrainPenalty"   ,        NoTerrainPenalty   ,
    "breakImmunities"    ,        BreakImmunities    ,
    "permanentDeath"     ,        PermanentDeath     
)


STRUCT_REFLECTION_PAIRED(
    LibraryArtifact::Presentation,
    "iconStash",                  iconStash,
    "iconBonus",                  iconBonus,
    "order",                      order,
    "orderGroup",                 orderGroup,
    "orderCategory",              orderCategory,
    "name",                       name
)

STRUCT_REFLECTION_PAIRED(
    LibraryArtifact,
    "slot",                       slot,
    "class",                      treasureClass,
    "special",                    special,
    "calc",                       calc,
    "untranslatedName",           untranslatedName,
    "legacyId",                   legacyId,
    "tags",                       tags,
    "value",                      value,
    "provideSpells",              provideSpells,
    "protectSpells",              protectSpells,
    "forbidSpells",               forbidSpells,
    "spellCasts",                 spellCasts,
    "parts",                      parts,
    "noPenalty",                  disabledPenalties,
    "bmUnit",                     battleMachineUnit,
    "pres",                       presentationParams,
    "objectDefs",                 objectDefs
)


STRUCT_REFLECTION_PAIRED(
    ArtifactFilter,
    "only",                       onlyArtifacts,
    "not",                        notArtifacts,
    "classes",                    classes,
    "tags",                       tags,
    "all",                        all
)

// ------------------------------------------------------------------------------------------


STRUCT_REFLECTION_PAIRED(
    LibraryDwelling,
    "creatureIds",                creatureIds,
    "objectDefs",                 objectDefs,
    "hasPlayer",                  hasPlayer
)
// ------------------------------------------------------------------------------------------
ENUM_REFLECTION_PAIRED(LibraryHero::Presentation::Gender,
    Unspec,
    "f" ,                         Female,
    "m" ,                         Male
)
ENUM_REFLECTION_PAIRED(LibraryHeroSpec::Type,
    None,
    "income"         ,            Income,
    "unit"           ,            Unit,
    "unitUpgrade"    ,            UnitUpgrade,
    "unitNonStd"     ,            UnitNonStd,
    "spell"          ,            Spell,
    "skill"          ,            Skill,
    "specialBallista",            SpecialBallista,
    "specialCannon"  ,            SpecialCannon,
    "specialDragons" ,            SpecialDragons,
    "specialSpeed"   ,            SpecialSpeed
)


STRUCT_REFLECTION_PAIRED(
    LibraryHeroSpec::Presentation,
    "icon",                       icon
)

STRUCT_REFLECTION_PAIRED(
    LibraryHeroSpec,
    "type",                       type,
    "untranslatedName",           untranslatedName,
    "dayIncome",                  dayIncome,
    "unitId",                     unit,
    "skillId",                    skill,
    "spellId",                    spell,
    "pres",                       presentationParams
)
// ------------------------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    SkillHeroItem,
    "skillId",                    skill,
    "level",                      level
)


STRUCT_REFLECTION_PAIRED(
    LibraryHero::StartStack,
    "id",                          unit,
    "stackSize",                   stackSize
)

STRUCT_REFLECTION_PAIRED(
    LibraryHero::Presentation,
    "portrait",                    portrait,
    "portraitSmall",               portraitSmall,
    "order",                       order,
    "gender",                      gender,
    "name",                        name,
    "bio",                         bio
)

STRUCT_REFLECTION_PAIRED(
    LibraryHero,
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "factionId",                   faction,
    "isWarrior",                   isWarrior,
    "specId",                      spec,
    "secondarySkills",             secondarySkills, 
    "startSpellId",                startSpell,
    "startStacks",                 startStacks,
    "pres",                        presentationParams
)
// ------------------------------------------------------------------------------------------
ENUM_REFLECTION_PAIRED(LibrarySpell::Type,
    Temp,
    "temp"       ,                 Temp,
    "offensive"  ,                 Offensive,
    "special"    ,                 Special,
    "summon"     ,                 Summon,
    "rising"     ,                 Rising,
    "adventure"  ,                 Adventure
)
ENUM_REFLECTION_PAIRED(MagicSchool,
    Any,
    "any"      ,                   Any,
    "earth"    ,                   Earth,
    "air"      ,                   Air,
    "water"    ,                   Water,
    "fire"     ,                   Fire
)
ENUM_REFLECTION_PAIRED(LibrarySpell::TargetClass,
    None,
    "units"       ,               Units,
    "land"        ,               Land,
    "immediate"   ,               Immediate
)
ENUM_REFLECTION_PAIRED(LibrarySpell::Range,
    Single,
    "single"       ,              Single,
    "r1"           ,              R1,
    "r1NoCenter"   ,              R1NoCenter,
    "r2"           ,              R2,
    "r3"           ,              R3,
    "obstacle2"    ,              Obstacle2,
    "obstacle3"    ,              Obstacle3,
    "chain4"       ,              Chain4,
    "chain5"       ,              Chain5,
    "all"          ,              All
)
ENUM_REFLECTION_PAIRED(LibrarySpell::Qualify,
    None,
    "good"        ,               Good,
    "bad"         ,               Bad
)
ENUM_REFLECTION_PAIRED(LibrarySpell::Tag,
    Mind,
    "mind"        ,               Mind,
    "vision"      ,               Vision,
    "ice"         ,               Ice,
    "lightning"   ,               Lightning,
    "airElem"     ,               AirElem,
    "fireElem"    ,               FireElem
)
ENUM_REFLECTION_PAIRED(LibrarySpell::EndCondition,
    Time,
    "time"          ,             Time,
    "getHit"        ,             GetHit,
    "makingAttack"  ,             MakingAttack
)


STRUCT_REFLECTION_PAIRED(
    LibrarySpell::Presentation,
    "bottomAnimation",             bottomAnimation,
    "projectile",                  projectile,
    "bottomPosition",              bottomPosition,
    "animationOnMainPos",          animationOnMainPosition,
    "iconBonus",                   iconBonus,
    "iconInt",                     iconInt,
    "iconTrans",                   iconTrans,
    "iconScroll",                  iconScroll,
    "configOrder",                 configOrder,
    "animation",                   animation,
    "sound",                       sound,
    "name",                        name
)


STRUCT_REFLECTION_PAIRED(
    LibrarySpell,
    "untranslatedName",            untranslatedName,
    "legacyId",                    legacyId,
    "isTeachable",                 isTeachable,
    "type",                        type,
    "qualify",                     qualify,
    "school",                      school,
    "tags",                        tags,
    "level",                       level,
    "manaCost",                    manaCost,
    "indistinctive",               indistinctive,
    "targetClass",                 targetClass,
    "counterSpells",               counterSpells,
    "calc",                        calcScript,
    "filter",                      filterScript,
    "rangeByLevel",                rangeByLevel,
    "endConditions",               endConditions,
    "retaliationWhenCancel",       retaliationWhenCancel,
    "summonUnit",                  summonUnit,
    "pres",                        presentationParams
)

STRUCT_REFLECTION_PAIRED(
    SpellCastParams,
    "spell",                       spell,
    "sp",                          spellPower,
    "level",                       skillLevel,
    "spPerUnit",                   spPerUnit
)

STRUCT_REFLECTION_PAIRED(
    SpellFilter,
    "onlySpells",                  onlySpells,
    "notSpells",                   notSpells,
    "levels",                      levels,
    "schools",                     schools,
    "tags",                        tags,
    "teachableOnly",               teachableOnly,
    "all",                         all
)

STRUCT_REFLECTION_PAIRED(
    Reward,
    "resources"   ,                resources,
    "units"       ,                units,
    "artifacts"   ,                artifacts,
    "exp"         ,                gainedExp,
    "mana"        ,                manaDiff,
    "rng"         ,                rngBonus,
    "stat"        ,                statBonus,
    "secSkills"   ,                secSkills,
    "spells"      ,                spells
)

// ------------------------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    UnitWithCount,
    "id"        ,                  unit,
    "n"         ,                  count
)

STRUCT_REFLECTION_STRINGIFY(
    LibraryMapBank::Variant,
    name,
    rewardIndex,
    guards)

STRUCT_REFLECTION_STRINGIFY(
    LibraryMapBank::Presentation,
    order,
    name)

STRUCT_REFLECTION_PAIRED(LibraryMapBank,
    "objectDefs"          ,        objectDefs,
    "untranslatedName"    ,        untranslatedName,
    "legacyId"            ,        legacyId,
    "variants"            ,        variants,
    "rewards"             ,        rewards,
    "pres"                ,        presentationParams,
    "fieldLayout"         ,        fieldLayout
    )

ENUM_REFLECTION_STRINGIFY(LibraryMapObstacle::Type, Invalid,
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

    DECORATION,

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


STRUCT_REFLECTION_PAIRED(
    LibraryMapObstacle,
    "legacyId",                    legacyId,
    "objectDefs",                  objectDefs,
    "type",                        type
)
// ------------------------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    LibraryMapVisitable,
    "legacyId",                    legacyId,
    "objectDefs",                  objectDefs
)

// ------------------------------------------------------------------------------------------

STRUCT_REFLECTION_PAIRED(
    LibraryGameRules::RngRules,
    "positiveChances",             positiveChances,
    "negativeChances",             negativeChances,
    "maxEffectiveValue",           maxEffectiveValue,
    "minEffectiveValue",           minEffectiveValue
)

STRUCT_REFLECTION_PAIRED(
    LibraryGameRules::PhysicalConst,
    "maxEffectiveAttack",          maxEffectiveAttack,
    "maxEffectiveDefense",         maxEffectiveDefense,
    "attackValue",                 attackValue,
    "defenseValue",                defenseValue
)

STRUCT_REFLECTION_PAIRED(
    LibraryGameRules::Limits,
    "stacks",                      stacks,
    "maxHeroLevel",                maxHeroLevel,
    "maxHeroAd",                   maxHeroAd,
    "maxHeroMagic",                maxHeroMagic,
    "maxUnitAd",                   maxUnitAd
)

STRUCT_REFLECTION_PAIRED(
    LibraryGameRules,
    "luck",                        luck,
    "morale",                      morale,
    "physicalConst",               physicalConst,
    "limits",                      limits
)


STRUCT_REFLECTION_PAIRED(
    LibraryObjectDef,
    "legacyId",                    legacyId,
    "defFile",                     defFile,
    "blockMap",                    blockMap,
    "visitMap",                    visitMap,
    "terrainsHard",                terrainsHard,
    "terrainsSoft",                terrainsSoft,
    "objId",                       objId,
    "subId",                       subId,
    "type",                        type,
    "priority",                    priority,
    "substituteFor",               substituteFor,
    "substituteKey",               substituteKey
)

STRUCT_REFLECTION_PAIRED(
    ObjectDefMappings,
    "m",                           variants
)

STRUCT_REFLECTION_PAIRED(
    ObjectDefIndex,
    "v",                           variant,
    "s",                           substitution
)

ENUM_REFLECTION_PAIRED(FieldLayout,
    Standard,
    "std"          ,               Standard,
    "obj"          ,               Object,
    "churchyard1"  ,               Churchyard1,
    "churchyard2"  ,               Churchyard2,
    "ruins"        ,               Ruins,
    "spit"         ,               Spit
)
STRUCT_REFLECTION_PAIRED(
    BonusRatio,
    "num",                         m_num,
    "denom",                       m_denom
)
STRUCT_REFLECTION_PAIRED(
    TranslationMap,
    "ts",                         ts
)
// clang-format on

template<>
inline constexpr const bool s_useCustomTransformRead<LibraryUnit::Traits>{ true };
template<>
CORELOGIC_EXPORT bool MetaInfo::transformTreeRead<LibraryUnit::Traits>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool s_useCustomTransformRead<ResourceAmount>{ true };
template<>
CORELOGIC_EXPORT bool MetaInfo::transformTreeRead<ResourceAmount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool s_useCustomTransformRead<UnitWithCount>{ true };
template<>
CORELOGIC_EXPORT bool MetaInfo::transformTreeRead<UnitWithCount>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool s_useCustomTransformRead<LibraryHero::StartStack>{ true };
template<>
CORELOGIC_EXPORT bool MetaInfo::transformTreeRead<LibraryHero::StartStack>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool s_useCustomTransformRead<ObjectDefMappings>{ true };
template<>
CORELOGIC_EXPORT bool MetaInfo::transformTreeRead<ObjectDefMappings>(const PropertyTree& treeIn, PropertyTree& treeOut);

template<>
inline constexpr const bool s_isStringMap<LibraryFactionHeroClass::SkillWeights>{ true };

template<>
inline constexpr const bool s_isStringMap<LibraryFactionHeroClass::PrimaryWeights>{ true };

template<>
inline constexpr const bool s_isStringMap<ObjectDefMappings::Map>{ true };

template<>
inline constexpr const bool s_isStringMap<TranslationMap::Data>{ true };

}
