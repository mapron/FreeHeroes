/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes::Core {

struct LibraryArtifact;
struct LibraryBuilding;
struct LibraryDwelling;
struct LibraryFaction;
struct LibraryFactionHeroClass;
struct LibraryGameRules;
struct LibraryHero;
struct LibraryHeroSpec;
struct LibraryMapBank;
struct LibraryMapObstacle;
struct LibraryMapVisitable;
struct LibraryObjectDef;
struct LibraryPlayer;
struct LibraryResource;
struct LibrarySecondarySkill;
struct LibrarySpell;
struct LibraryTerrain;
struct LibraryUnit;

using LibraryArtifactConstPtr         = const LibraryArtifact*;
using LibraryBuildingConstPtr         = const LibraryBuilding*;
using LibraryDwellingConstPtr         = const LibraryDwelling*;
using LibraryFactionConstPtr          = const LibraryFaction*;
using LibraryFactionHeroClassConstPtr = const LibraryFactionHeroClass*;
using LibraryGameRulesConstPtr        = const LibraryGameRules*;
using LibraryHeroConstPtr             = const LibraryHero*;
using LibraryHeroSpecConstPtr         = const LibraryHeroSpec*;
using LibraryMapBankConstPtr          = const LibraryMapBank*;
using LibraryMapObstacleConstPtr      = const LibraryMapObstacle*;
using LibraryMapVisitableConstPtr     = const LibraryMapVisitable*;
using LibraryObjectDefConstPtr        = const LibraryObjectDef*;
using LibraryPlayerConstPtr           = const LibraryPlayer*;
using LibraryResourceConstPtr         = const LibraryResource*;
using LibrarySecondarySkillConstPtr   = const LibrarySecondarySkill*;
using LibrarySpellConstPtr            = const LibrarySpell*;
using LibraryTerrainConstPtr          = const LibraryTerrain*;
using LibraryUnitConstPtr             = const LibraryUnit*;

struct ObjectDefIndex;
struct ObjectDefMappings;

}
