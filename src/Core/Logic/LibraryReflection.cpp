/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibraryReflection.hpp"
#include "LibraryIdResolver.hpp"

#include "IGameDatabase.hpp"
#include "LibraryFaction.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryUnit.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryArtifact.hpp"
#include "LibraryHero.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryResource.hpp"
#include "LibraryMapObject.hpp"

#include "StringUtils.hpp"

#include <rttr/registration>

#include <iostream>

using namespace rttr;

template<typename T>
void registerConverters()
{
    using namespace FreeHeroes::Core;
    type::register_converter_func( [](const T * value, bool& ok) -> std::string
        { ok = true; return value ? value->id : ""; });
    type::register_converter_func( [](T * value, bool& ok) -> std::string
        { ok = true; return value ? value->id : ""; });

    type t = type::get<const T *>();

    using DS = Reflection::LibraryIdResolver;

    DS::registerIdResolver(std::string{t.get_name()}, [](const DS::ResolutionId & id, const DS::ResolutionContext& context) -> DS::ResolutionResult{
        auto ptr = context.database->container<T>()->find(id);
        return std::pair{variant(ptr), !!ptr};
    });

}
#define ref policy::prop::as_reference_wrapper
RTTR_REGISTRATION
{
    using namespace FreeHeroes::Core;
    registration::class_<PrimaryRngParams>("PrimaryRngParams")
        .constructor<>()
        .property("luck"  , &PrimaryRngParams::luck)
        .property("morale", &PrimaryRngParams::morale)
        ;
    registration::class_<PrimaryAttackParams>("PrimaryAttackParams")
        .constructor<>()
        .property("attack" , &PrimaryAttackParams::attack)
        .property("defense", &PrimaryAttackParams::defense)
        ;
    registration::class_<PrimaryMagicParams>("PrimaryMagicParams")
        .constructor<>()
        .property("sp" , &PrimaryMagicParams::spellPower)
        .property("int", &PrimaryMagicParams::intelligence)
        ;
    registration::class_<DamageDesc>("DamageDesc")
        .constructor<>()
        .property("min", &DamageDesc::minDamage)
        .property("max", &DamageDesc::maxDamage)
        ;
    registration::class_<UnitPrimaryParams>("UnitPrimaryParams")
        .constructor<>()
        .property("dmg"      , &UnitPrimaryParams::dmg)(ref)
        .property("ad"       , &UnitPrimaryParams::ad)(ref)
        .property("maxHealth", &UnitPrimaryParams::maxHealth)
        .property("speed"    , &UnitPrimaryParams::battleSpeed)
        .property("armySpeed", &UnitPrimaryParams::armySpeed)
        .property("shoots"   , &UnitPrimaryParams::shoots)
        ;
    registration::class_<HeroPrimaryParams>("HeroPrimaryParams")
        .constructor<>()
        .property("ad", &HeroPrimaryParams::ad)(ref)
        .property("magic", &HeroPrimaryParams::magic)(ref)
        ;
    registration::enumeration<HeroPrimaryParamType>("HeroPrimaryParamType")
        (
        value("attack" ,   HeroPrimaryParamType::Attack),
        value("defense",   HeroPrimaryParamType::Defense),
        value("sp"     ,   HeroPrimaryParamType::SpellPower),
        value("int"    ,   HeroPrimaryParamType::Intelligence),
        value("mana"   ,   HeroPrimaryParamType::Mana),
        value("exp"    ,   HeroPrimaryParamType::Experience)
        );

    registration::class_<Reflection::LibraryIdResolver::DelayedDeserializeParam>("DelayedDeserializeParam")
        .constructor<>()
        ;

    registration::class_<BonusRatio>("BonusRatio")
        .constructor<>()
        ;
    type::register_converter_func( [](std::string value, bool& ok) -> BonusRatio
        { ok = false;
          auto parts = splitLine(value, '/', true);
          if (parts.size() == 2) {
             int n = std::atoi(parts[0].c_str());
             int d = std::atoi(parts[1].c_str());
             if (d != 0) {
                 ok = true;
                 return BonusRatio(n, d);
             }
          }
          return BonusRatio();
        });
    registration::class_<MagicReduce>("MagicReduce")
        .constructor<>()
        .property("all"      , &MagicReduce::allMagic)
        .property("air"      , &MagicReduce::air   )
        .property("earth"    , &MagicReduce::earth )
        .property("fire"     , &MagicReduce::fire  )
        .property("water"    , &MagicReduce::water )
        ;
    registration::class_<MagicIncrease>("MagicIncrease")
        .constructor<>()
        .property("all"      , &MagicIncrease::allMagic)
        .property("air"      , &MagicIncrease::air   )
        .property("earth"    , &MagicIncrease::earth )
        .property("fire"     , &MagicIncrease::fire  )
        .property("water"    , &MagicIncrease::water )
        ;

    // ------------------------------------------------------------------------------------------
    registration::class_<LibraryTerrain::Presentation>("LibraryTerrainPres")
        .constructor<>()
        .property("music"              , &LibraryTerrain::Presentation::music)
        .property("backgroundsBattle"  , &LibraryTerrain::Presentation::backgroundsBattle)(ref)
        .property("order"              , &LibraryTerrain::Presentation::order)
        .property("icon"               , &LibraryTerrain::Presentation::icon)
        ;
    registration::class_<LibraryTerrain>("LibraryTerrain")
        .constructor<>()
        .property("untranslatedName"   , &LibraryTerrain::untranslatedName)
        .property("moveCost"           , &LibraryTerrain::moveCost)
        .property("isObstacle"         , &LibraryTerrain::isObstacle)
        .property("extraLayer"         , &LibraryTerrain::extraLayer)
        .property("pres"               , &LibraryTerrain::presentationParams)(ref)
        ;
    // ------------------------------------------------------------------------------------------
    registration::class_<LibraryResource::Presentation>("LibraryResourcePres")
        .constructor<>()
        .property("orderKingdom"     , &LibraryResource::Presentation::orderKingdom)
        .property("orderCommon"      , &LibraryResource::Presentation::orderCommon)
        .property("icon"             , &LibraryResource::Presentation::icon)
        .property("iconLarge"        , &LibraryResource::Presentation::iconLarge)
        .property("iconTrade"        , &LibraryResource::Presentation::iconTrade)
        ;
    registration::class_<LibraryResource>("LibraryResource")
        .constructor<>()
        .property("untranslatedName" , &LibraryResource::untranslatedName)
        .property("pres"             , &LibraryResource::presentationParams)(ref)
        ;

    registration::class_<ResourceAmount>("ResourceAmount")(metadata("transform", Reflection::getJsonTransform<ResourceAmount>()))
        .constructor<>()
        .property("gold"     , &ResourceAmount::gold)
        .property("wood"     , &ResourceAmount::wood)
        .property("ore"      , &ResourceAmount::ore )
        .property("mercury"  , &ResourceAmount::mercury)
        .property("sulfur"   , &ResourceAmount::sulfur )
        .property("crystal"  , &ResourceAmount::crystal)
        .property("gems"     , &ResourceAmount::gems   )
        ;

    //  -----------------------------------------------------------------------------
    registration::class_<LibraryFactionHeroClass::Presentation>("LibraryFactionHeroClassPres")
        .constructor<>()
        .property("battleSpriteMale"  , &LibraryFactionHeroClass::Presentation::battleSpriteMale)
        .property("battleSpriteFemale", &LibraryFactionHeroClass::Presentation::battleSpriteFemale)
        ;

    registration::class_<LibraryFactionHeroClass>("LibraryFactionHeroClass")
        .constructor<>()
        .property("id", &LibraryFactionHeroClass::id)
        .property("startParams"       , &LibraryFactionHeroClass::startParams)(ref)
        .property("skills"            , &LibraryFactionHeroClass::secondarySkillWeights)(ref, metadata("transform", Reflection::getJsonTransform<LibraryFactionHeroClass::SkillWeights>()))
        .property("untranslatedName"  , &LibraryFactionHeroClass::untranslatedName)
        .property("lowLevelIncrease"  , &LibraryFactionHeroClass::lowLevelIncrease)(ref, metadata("transform", Reflection::getJsonTransform<LibraryFactionHeroClass::SkillWeights>()))
        .property("highLevelIncrease" , &LibraryFactionHeroClass::highLevelIncrease)(ref, metadata("transform", Reflection::getJsonTransform<LibraryFactionHeroClass::SkillWeights>()))
        .property("pres"              , &LibraryFactionHeroClass::presentationParams)(ref)
        ;
    registration::enumeration<LibraryFaction::Alignment>("FactionAlignment")
        (
        value("good"       ,  LibraryFaction::Alignment::Good),
        value("evil"       ,  LibraryFaction::Alignment::Evil),
        value("neutral"    ,  LibraryFaction::Alignment::Neutral),
        value("independent",  LibraryFaction::Alignment::Independent),
        value("special"    ,  LibraryFaction::Alignment::Special)
        );
    registration::class_<LibraryFaction::Presentation>("LibraryFactionPres")
        .constructor<>()
        .property("goesAfterId"       , &LibraryFaction::Presentation::goesAfterId)
        .property("unitBackground" , &LibraryFaction::Presentation::unitBackground)
        ;
    registration::class_<LibraryFaction>("LibraryFaction")
        .constructor<>()
        .property("alignment"         , &LibraryFaction::alignment)
        .property("untranslatedName"  , &LibraryFaction::untranslatedName)
        .property("fighterClass"      , &LibraryFaction::fighterClass)(ref)
        .property("mageClass"         , &LibraryFaction::mageClass)(ref)
        .property("nativeTerrain"     , &LibraryFaction::nativeTerrain)
        .property("pres"              , &LibraryFaction::presentationParams)(ref)
        ;
     //  -----------------------------------------------------------------------------
    registration::class_<LibrarySecondarySkill::Presentation::LevelParams>("LibrarySecondarySkillLevelParams")
        .constructor<>()
        .property("iconSmall" , &LibrarySecondarySkill::Presentation::LevelParams::iconSmall)
        .property("iconMedium", &LibrarySecondarySkill::Presentation::LevelParams::iconMedium)
        .property("iconLarge" , &LibrarySecondarySkill::Presentation::LevelParams::iconLarge)
        ;
    registration::enumeration<LibrarySecondarySkill::HandlerType>("SkillHandlerType")
        (
        value("stat"       ,  LibrarySecondarySkill::HandlerType::Stat),
        value("special"    ,  LibrarySecondarySkill::HandlerType::Special),
        value("wisdom"     ,  LibrarySecondarySkill::HandlerType::Wisdom),
        value("school"     ,  LibrarySecondarySkill::HandlerType::School)
        );
    registration::class_<LibrarySecondarySkill::Presentation>("LibrarySecondarySkillPres")
        .constructor<>()
            .property("levels"            , &LibrarySecondarySkill::Presentation::levels)(ref)
            .property("order"             , &LibrarySecondarySkill::Presentation::order)
            ;
    registration::class_<LibrarySecondarySkill>("LibrarySecondarySkill")
        .constructor<>()
        .property("untranslatedName"  , &LibrarySecondarySkill::untranslatedName)
        .property("frequencyFighter"  , &LibrarySecondarySkill::frequencyFighter)
        .property("frequencyMage"     , &LibrarySecondarySkill::frequencyMage)
        .property("handler"           , &LibrarySecondarySkill::handler)
        .property("calc"              , &LibrarySecondarySkill::calc)(ref)
        .property("pres"              , &LibrarySecondarySkill::presentationParams)(ref)
        ;
     //  -----------------------------------------------------------------------------

    registration::enumeration<UnitType>("UnitType")
        (
        value("living"     ,  UnitType::Living),
        value("nonLiving"  ,  UnitType::NonLiving),
        value("siege"      ,  UnitType::SiegeMachine),
        value("arrowTower" ,  UnitType::ArrowTower),
        value("wall"       ,  UnitType::Wall),
        value(""           ,  UnitType::Unknown)
        );
    registration::enumeration<UnitNonLivingType>("UnitNonLivingType")
        (
        value(""         ,   UnitNonLivingType::None),
        value("undead"   ,   UnitNonLivingType::Undead),
        value("golem"    ,   UnitNonLivingType::Golem),
        value("gargoyle" ,   UnitNonLivingType::Gargoyle),
        value("elemental",   UnitNonLivingType::Elemental),
        value("bm"       ,   UnitNonLivingType::BattleMachine)
        );
    registration::class_<LibraryUnit::Traits>("LibraryUnitTraits")
        .constructor<>()
        .property("large"        , &LibraryUnit::Traits::large)
        .property("ranged"       , &LibraryUnit::Traits::rangeAttack)
        .property("fly"          , &LibraryUnit::Traits::fly)
        .property("teleport"     , &LibraryUnit::Traits::teleport)
        .property("dblAttack"    , &LibraryUnit::Traits::doubleAttack)
        .property("freeAttack"   , &LibraryUnit::Traits::freeAttack)
        .property("catapultShoot", &LibraryUnit::Traits::canBeCatapult)
        .property("autoReturn"   , &LibraryUnit::Traits::returnAfterAttack)
        ;

    registration::enumeration<LibraryUnit::Abilities::SplashAttack>("SplashAttack")
        (
        value(""          ,  LibraryUnit::Abilities::SplashAttack::None),
        value("dragon"    ,  LibraryUnit::Abilities::SplashAttack::Dragon),
        value("neighbours",  LibraryUnit::Abilities::SplashAttack::Neighbours),
        value("sides"     ,  LibraryUnit::Abilities::SplashAttack::Sides),
        value("ranged"    ,  LibraryUnit::Abilities::SplashAttack::Ranged)
        );
    registration::enumeration<LibraryUnit::Abilities::DamagePenalty>("DamagePenalty")
        (
        value("melee"   ,  LibraryUnit::Abilities::DamagePenalty::Melee),
        value("distance",  LibraryUnit::Abilities::DamagePenalty::Distance),
        value("obstacle",  LibraryUnit::Abilities::DamagePenalty::Obstacle)
        );
    registration::enumeration<LibraryUnit::Abilities::AttackWithElement>("AttackWithElement")
        (
        value(""       ,  LibraryUnit::Abilities::AttackWithElement::None),
        value("fire"   ,  LibraryUnit::Abilities::AttackWithElement::Fire),
        value("earth"  ,  LibraryUnit::Abilities::AttackWithElement::Earth),
        value("air"    ,  LibraryUnit::Abilities::AttackWithElement::Air),
        value("ice"    ,  LibraryUnit::Abilities::AttackWithElement::Ice),
        value("mind"   ,  LibraryUnit::Abilities::AttackWithElement::Mind),
        value("magic"  ,  LibraryUnit::Abilities::AttackWithElement::Magic),
        value("undead" ,  LibraryUnit::Abilities::AttackWithElement::Undead)
        );

    registration::class_<LibraryUnit::Abilities>("LibraryUnitAbilities")
        .constructor<>()
        .property("type"               , &LibraryUnit::Abilities::type)
        .property("nonLivingType"      , &LibraryUnit::Abilities::nonLivingType)
        .property("splash"             , &LibraryUnit::Abilities::splashType)
        .property("splashFriendlyFire" , &LibraryUnit::Abilities::splashFriendlyFire)
        .property("splashButtons"      , &LibraryUnit::Abilities::splashButtons)
        .property("splashSpell"        , &LibraryUnit::Abilities::splashSpell)
        .property("splashElement"      , &LibraryUnit::Abilities::splashElement)
        .property("retaliations"       , &LibraryUnit::Abilities::maxRetaliations)
        .property("chargeAttack"       , &LibraryUnit::Abilities::chargeAttack)
        .property("noPenalty"          , &LibraryUnit::Abilities::disabledPenalties)(ref)
        .property("increaseHeroMorale" , &LibraryUnit::Abilities::increaseHeroMorale)
        .property("decreaseEnemyMorale", &LibraryUnit::Abilities::decreaseEnemyMorale)
        .property("increaseHeroLuck"   , &LibraryUnit::Abilities::increaseHeroLuck)
        .property("decreaseEnemyLuck"  , &LibraryUnit::Abilities::decreaseEnemyLuck)

        .property("minimalLuckLevel"    , &LibraryUnit::Abilities::minimalLuckLevel)
        .property("minimalMoraleLevel"  , &LibraryUnit::Abilities::minimalMoraleLevel)
        .property("magicReduce"         , &LibraryUnit::Abilities::magicReduce)(ref)
        .property("magicOppSuccessChance"           , &LibraryUnit::Abilities::magicOppSuccessChance)
        .property("magicOppSuccessChanceNeighbours" , &LibraryUnit::Abilities::magicOppSuccessChanceNeighbours)

        .property("casts"              , &LibraryUnit::Abilities::casts)(ref)
        .property("vulnerable"         , &LibraryUnit::Abilities::vulnerable)(ref)
        .property("vulnerableRatio"    , &LibraryUnit::Abilities::vulnerableRatio)
        .property("immunes"            , &LibraryUnit::Abilities::immunes)(ref)
        .property("immuneBreakable"    , &LibraryUnit::Abilities::immuneBreakable)


        .property("weekIncome"                      , &LibraryUnit::Abilities::weekIncome)(ref)
        ;

    registration::class_<LibraryUnit::HeroStackSize>("HeroStackSize")
        .constructor<>()
        .property("min", &LibraryUnit::HeroStackSize::min)
        .property("max", &LibraryUnit::HeroStackSize::max)
        ;


    registration::class_<LibraryUnit::Presentation>("LibraryUnitPresentation")
        .constructor<>()
        .property("spriteBattle"         , &LibraryUnit::Presentation::spriteBattle)
        .property("spriteAdventure"      , &LibraryUnit::Presentation::spriteAdventure)
        .property("portrait"             , &LibraryUnit::Presentation::portrait)
        .property("portraitSM"           , &LibraryUnit::Presentation::portraitSmall)
        .property("soundId"              , &LibraryUnit::Presentation::soundId)
        .property("spriteProjectile"     , &LibraryUnit::Presentation::spriteProjectile)
        .property("soundHasShoot"        , &LibraryUnit::Presentation::soundHasShoot)
        .property("soundHasMovementStart", &LibraryUnit::Presentation::soundHasMovementStart)
        ;

    registration::class_<LibraryUnit>("LibraryUnit")
        .constructor<>()
        .property("untranslatedName"     , &LibraryUnit::untranslatedName)
        .property("primary"              , &LibraryUnit::primary)  (ref)
        .property("factionId"            , &LibraryUnit::faction)
        .property("level"                , &LibraryUnit::level)
        .property("growth"               , &LibraryUnit::growth)
        .property("countWithHeroBase"    , &LibraryUnit::countWithHeroBase)(ref)
        .property("cost"                 , &LibraryUnit::cost)(ref)
        .property("value"                , &LibraryUnit::value)
        .property("upgrades"             , &LibraryUnit::upgrades)(ref)
        .property("bmArtifact"           , &LibraryUnit::battleMachineArtifact)(metadata("optional", true))
        .property("abilities"            , &LibraryUnit::abilities)(ref)
        .property("pres"                 , &LibraryUnit::presentationParams)(ref)
        .property("traits"               , &LibraryUnit::traits)   (ref, metadata("transform", Reflection::getJsonTransform<LibraryUnit::Traits>()))
        ;

    // ------------------------------------------------------------------------------------------
    registration::enumeration<ArtifactSlotType>("ArtifactSlotType")
        (
        value(""       ,  ArtifactSlotType::Invalid),
        value("sword"  ,  ArtifactSlotType::Sword),
        value("shield" ,  ArtifactSlotType::Shield),
        value("helm"   ,  ArtifactSlotType::Helm),
        value("torso"  ,  ArtifactSlotType::Torso),
        value("ring"   ,  ArtifactSlotType::Ring),
        value("neck"   ,  ArtifactSlotType::Neck),
        value("boots"  ,  ArtifactSlotType::Boots),
        value("cape"   ,  ArtifactSlotType::Cape),
        value("misc"   ,  ArtifactSlotType::Misc),

        value("bmAmmo" ,  ArtifactSlotType::BmAmmo),
        value("bmShoot",  ArtifactSlotType::BmShoot),
        value("bmTent" ,  ArtifactSlotType::BmTent),
        value("bagOnly",  ArtifactSlotType::BagOnly),

        value("ring1"   ,  ArtifactSlotType::Ring1),
        value("misc1"   ,  ArtifactSlotType::Misc1),
        value("misc2"   ,  ArtifactSlotType::Misc2),
        value("misc3"   ,  ArtifactSlotType::Misc3),
        value("misc4"   ,  ArtifactSlotType::Misc4)
        );
    registration::enumeration<LibraryArtifact::TreasureClass>("ArtifactTreasureClass")
        (
        value("treasure" ,   LibraryArtifact::TreasureClass::Treasure),
        value("minor"    ,   LibraryArtifact::TreasureClass::Minor),
        value("major"    ,   LibraryArtifact::TreasureClass::Major),
        value("relic"    ,   LibraryArtifact::TreasureClass::Relic),
        value("unique"   ,   LibraryArtifact::TreasureClass::Unique),
        value("complex"  ,   LibraryArtifact::TreasureClass::Complex),
        value("bm"       ,   LibraryArtifact::TreasureClass::BattleMachine),
        value("scroll"   ,   LibraryArtifact::TreasureClass::Scroll),
        value("special"  ,   LibraryArtifact::TreasureClass::Special)
        );
    registration::class_<LibraryArtifact::Presentation>("LibraryArtifactPres")
        .constructor<>()
        .property("iconStash"       , &LibraryArtifact::Presentation::iconStash)
        .property("iconBonus"       , &LibraryArtifact::Presentation::iconBonus)
        .property("order"           , &LibraryArtifact::Presentation::order)
        .property("orderGroup"      , &LibraryArtifact::Presentation::orderGroup)
        ;
    registration::class_<LibraryArtifact>("LibraryArtifact")
        .constructor<>()
        .property("slot"            , &LibraryArtifact::slot)
        .property("class"           , &LibraryArtifact::treasureClass)
        .property("calc"            , &LibraryArtifact::calc)(ref)
        .property("untranslatedName", &LibraryArtifact::untranslatedName)
        .property("value"           , &LibraryArtifact::value)
        .property("provideSpells"   , &LibraryArtifact::provideSpells)(ref)
        .property("protectSpells"   , &LibraryArtifact::protectSpells)(ref)
        .property("forbidSpells"    , &LibraryArtifact::forbidSpells)(ref)
        .property("spellCasts"      , &LibraryArtifact::spellCasts)(ref)
        .property("parts"           , &LibraryArtifact::parts)(ref)
        .property("bmUnit"          , &LibraryArtifact::battleMachineUnit)(metadata("optional", true))
        .property("pres"            , &LibraryArtifact::presentationParams)(ref)
        ;
    registration::class_<ArtifactRewardAmount>("ArtifactRewardAmount")(metadata("transform", Reflection::getJsonTransform<ArtifactRewardAmount>()))
        .constructor<>()
        .property("artifacts", &ArtifactRewardAmount::artifacts)(ref)
        ;
    registration::class_<ArtifactRewardAmount::SingleReward>("ArtifactRewardAmountSingle")
        .constructor<>()
        .property("class", &ArtifactRewardAmount::SingleReward::treasureClass)
        .property("n"    , &ArtifactRewardAmount::SingleReward::count)
        ;
    // ------------------------------------------------------------------------------------------

    registration::enumeration<LibraryHero::Presentation::Gender>("LibraryHeroGender")
        (
        value(""       ,   LibraryHero::Presentation::Gender::Unspec),
        value("f"      ,   LibraryHero::Presentation::Gender::Female),
        value("m"      ,   LibraryHero::Presentation::Gender::Male)
        );


    registration::enumeration<LibraryHeroSpec::Type>("LibraryHeroSpecType")
        (
        value(""               ,   LibraryHeroSpec::Type::None),
        value("income"         ,   LibraryHeroSpec::Type::Income),
        value("unit"           ,   LibraryHeroSpec::Type::Unit),
        value("unitUpgrade"    ,   LibraryHeroSpec::Type::UnitUpgrade),
        value("unitNonStd"     ,   LibraryHeroSpec::Type::UnitNonStd),
        value("spell"          ,   LibraryHeroSpec::Type::Spell),
        value("skill"          ,   LibraryHeroSpec::Type::Skill),
        value("specialBallista",   LibraryHeroSpec::Type::SpecialBallista),
        value("specialCannon"  ,   LibraryHeroSpec::Type::SpecialCannon),
        value("specialDragons" ,   LibraryHeroSpec::Type::SpecialDragons),
        value("specialSpeed"   ,   LibraryHeroSpec::Type::SpecialSpeed)

        );
    registration::class_<LibraryHeroSpec::Presentation>("LibraryHeroSpecPres")
        .constructor<>()
        .property("icon"             , &LibraryHeroSpec::Presentation::icon)
        ;
    registration::class_<LibraryHeroSpec>("LibraryHeroSpec")
        .constructor<>()
        .property("type"             , &LibraryHeroSpec::type)
        .property("untranslatedName" , &LibraryHeroSpec::untranslatedName)
        .property("dayIncome"        , &LibraryHeroSpec::dayIncome)(ref)
        .property("unitId"           , &LibraryHeroSpec::unit)
        .property("skillId"          , &LibraryHeroSpec::skill)
        .property("spellId"          , &LibraryHeroSpec::spell)
        .property("pres"             , &LibraryHeroSpec::presentationParams)(ref)
        ;
    // ------------------------------------------------------------------------------------------
    registration::class_<SkillHeroItem>("SkillHeroItem")
        .constructor<>()
        .property("skillId"      , &SkillHeroItem::skill)
        .property("level"        , &SkillHeroItem::level)
        ;
    registration::class_<LibraryHero::StartStack>("LibraryHeroStartUnit")(metadata("transform", Reflection::getJsonTransform<LibraryHero::StartStack>()))
        .constructor<>()
        .property("id"       , &LibraryHero::StartStack::unit)
        .property("stackSize", &LibraryHero::StartStack::stackSize)(ref)
        ;
    registration::class_<LibraryHero::Presentation>("LibraryHeroPres")
        .constructor<>()
        .property("portrait"          , &LibraryHero::Presentation::portrait)
        .property("portraitSmall"     , &LibraryHero::Presentation::portraitSmall)
        .property("order"             , &LibraryHero::Presentation::order)
        .property("gender"            , &LibraryHero::Presentation::gender)
        ;
    registration::class_<LibraryHero>("LibraryHero")
        .constructor<>()
        .property("untranslatedName"  , &LibraryHero::untranslatedName)
        .property("factionId"         , &LibraryHero::faction)
        .property("fighter"           , &LibraryHero::isFighter)
        .property("specId"            , &LibraryHero::spec)
        .property("secondarySkills"   , &LibraryHero::secondarySkills) (ref)
        .property("startSpellId"      , &LibraryHero::startSpell)(metadata("optional", true))
        .property("startStacks"       , &LibraryHero::startStacks)(ref)
        .property("pres"              , &LibraryHero::presentationParams)(ref)
        ;
    // ------------------------------------------------------------------------------------------
    registration::enumeration<LibrarySpell::Type>("LibrarySpellType")
        (
        value("temp"       ,   LibrarySpell::Type::Temp),
        value("offensive"  ,   LibrarySpell::Type::Offensive),
        value("special"    ,   LibrarySpell::Type::Special),
        value("summon"     ,   LibrarySpell::Type::Summon),
        value("rising"     ,   LibrarySpell::Type::Rising),
        value("adventure"  ,   LibrarySpell::Type::Adventure)

        );
    registration::enumeration<MagicSchool>("MagicSchool")
        (
        value("any"      ,   MagicSchool::Any),
        value("earth"    ,   MagicSchool::Earth),
        value("air"      ,   MagicSchool::Air),
        value("water"    ,   MagicSchool::Water),
        value("fire"     ,   MagicSchool::Fire)
        );
    registration::enumeration<LibrarySpell::TargetClass>("LibrarySpellTargetClass")
        (
        value(""            ,   LibrarySpell::TargetClass::None),
        value("units"   ,   LibrarySpell::TargetClass::Units),
        value("land"        ,   LibrarySpell::TargetClass::Land),
        value("immediate"   ,   LibrarySpell::TargetClass::Immediate)
        );

    registration::enumeration<LibrarySpell::Range>("LibrarySpellRange")
        (
        value("single"       ,   LibrarySpell::Range::Single),
        value("r1"           ,   LibrarySpell::Range::R1),
        value("r1NoCenter"   ,   LibrarySpell::Range::R1NoCenter),
        value("r2"           ,   LibrarySpell::Range::R2),
        value("r3"           ,   LibrarySpell::Range::R3),
        value("obstacle2"    ,   LibrarySpell::Range::Obstacle2),
        value("obstacle3"    ,   LibrarySpell::Range::Obstacle3),
        value("chain4"       ,   LibrarySpell::Range::Chain4),
        value("chain5"       ,   LibrarySpell::Range::Chain5),
        value("all"          ,   LibrarySpell::Range::All)
        );
    registration::enumeration<LibrarySpell::Qualify>("LibrarySpellQualify")
        (
        value(""            ,   LibrarySpell::Qualify::None),
        value("good"        ,   LibrarySpell::Qualify::Good),
        value("bad"         ,   LibrarySpell::Qualify::Bad)
        );
    registration::enumeration<LibrarySpell::Tag>("LibrarySpellTag")
        (
        value("mind"        ,  LibrarySpell::Tag::Mind),
        value("vision"      ,  LibrarySpell::Tag::Vision),
        value("ice"         ,  LibrarySpell::Tag::Ice),
        value("lightning"   ,  LibrarySpell::Tag::Lightning)
        );
    registration::class_<LibrarySpell::Presentation>("LibrarySpellPres")
        .constructor<>()
        .property("bottomAnimation"     , &LibrarySpell::Presentation::bottomAnimation)
        .property("projectile"          , &LibrarySpell::Presentation::projectile)
        .property("bottomPosition"      , &LibrarySpell::Presentation::bottomPosition)
        .property("animationOnMainPos"  , &LibrarySpell::Presentation::animationOnMainPosition)
        .property("iconBonus"           , &LibrarySpell::Presentation::iconBonus)
        .property("iconInt"             , &LibrarySpell::Presentation::iconInt)
        .property("iconTrans"           , &LibrarySpell::Presentation::iconTrans)
        .property("iconScroll"          , &LibrarySpell::Presentation::iconScroll)
        .property("configOrder"         , &LibrarySpell::Presentation::configOrder)
        .property("animation"           , &LibrarySpell::Presentation::animation)
        .property("sound"               , &LibrarySpell::Presentation::sound)
        .property("soundSpecial"        , &LibrarySpell::Presentation::soundSpecial)
        ;

    registration::class_<LibrarySpell>("LibrarySpell")
        .constructor<>()
        .property("untranslatedName" , &LibrarySpell::untranslatedName)
        .property("isTeachable"      , &LibrarySpell::isTeachable)
        .property("type"             , &LibrarySpell::type)
        .property("qualify"          , &LibrarySpell::qualify)
        .property("school"           , &LibrarySpell::school)
        .property("tags"             , &LibrarySpell::tags)(ref)
        .property("level"            , &LibrarySpell::level)
        .property("manaCost"         , &LibrarySpell::manaCost)
        .property("indistinctive"    , &LibrarySpell::indistinctive)
        .property("targetClass"      , &LibrarySpell::targetClass)
        .property("counterSpells"    , &LibrarySpell::counterSpells)(ref)
        .property("calc"             , &LibrarySpell::calcScript)(ref)
        .property("filter"           , &LibrarySpell::filterScript)(ref)
        .property("rangeByLevel"     , &LibrarySpell::rangeByLevel)(ref)
        .property("pres"             , &LibrarySpell::presentationParams)(ref)
        ;
    registration::class_<SpellCastParams>("SpellCastParams")
        .constructor<>()
        .property("spell"       , &SpellCastParams::spell)
        .property("sp"          , &SpellCastParams::spellPower)
        .property("level"       , &SpellCastParams::skillLevel)
        .property("probability" , &SpellCastParams::probability)
        ;
    registration::class_<SpellFilter>("SpellFilter")
        .constructor<>()
        .property("onlySpells"   , &SpellFilter::onlySpells)(ref)
        .property("notSpells"    , &SpellFilter::notSpells)(ref)
        .property("levels"       , &SpellFilter::levels)(ref)
        .property("schools"      , &SpellFilter::schools)(ref)
        .property("tags"         , &SpellFilter::tags)(ref)
        .property("teachableOnly", &SpellFilter::teachableOnly)
        ;

        // ------------------------------------------------------------------------------------------

    registration::class_<LibraryMapObject::Reward>("LibraryMapObjectReward")
        .constructor<>()
        .property("resources"  , &LibraryMapObject::Reward::resources)(ref)
        .property("unit"   , &LibraryMapObject::Reward::unit)(ref)
        .property("artifacts"  , &LibraryMapObject::Reward::artifacts)(ref)
        ;
    registration::class_<UnitWithCount>("UnitWithCount")(metadata("transform", Reflection::getJsonTransform<UnitWithCount>()))
        .constructor<>()
        .property("id"        , &UnitWithCount::unit)
        .property("n"         , &UnitWithCount::count)
        ;
    registration::class_<LibraryMapObject::Variant>("LibraryMapObjectVariant")
        .constructor<>()
        .property("name"        , &LibraryMapObject::Variant::name)
        .property("rewardIndex" , &LibraryMapObject::Variant::rewardIndex)
        .property("guards"      , &LibraryMapObject::Variant::guards)(ref)
        ;
    registration::class_<LibraryMapObject::Presentation>("LibraryMapObjectPres")
        .constructor<>()
        .property("order"              , &LibraryMapObject::Presentation::order)
        .property("icon"               , &LibraryMapObject::Presentation::icon)
        ;
    registration::class_<LibraryMapObject>("LibraryMapObject")
        .constructor<>()
        .property("untranslatedName"    , &LibraryMapObject::untranslatedName)
        .property("variants"            , &LibraryMapObject::variants)(ref)
        .property("rewards"             , &LibraryMapObject::rewards)(ref)
        .property("pres"                , &LibraryMapObject::presentationParams)(ref)
        .property("fieldLayout"         , &LibraryMapObject::fieldLayout)
        ;
    // ------------------------------------------------------------------------------------------

    registerConverters<LibraryUnit>();
    registerConverters<LibraryHero>();
    registerConverters<LibraryArtifact>();
    registerConverters<LibrarySecondarySkill>();
    registerConverters<LibraryFaction>();
    registerConverters<LibrarySpell>();
    registerConverters<LibraryResource>();
    registerConverters<LibraryTerrain>();
    registerConverters<LibraryMapObject>();
    registerConverters<LibraryHeroSpec>();
}

namespace FreeHeroes::Core::Reflection {

void libraryReflectionStub() {
// just to make linker happy and not throw away registration.
}


}
