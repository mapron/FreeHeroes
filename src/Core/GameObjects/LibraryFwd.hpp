/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes::Core {

struct LibraryArtifact;
using LibraryArtifactConstPtr = const LibraryArtifact*;

struct LibraryUnit;
using LibraryUnitConstPtr = const LibraryUnit*;

struct LibraryFaction;
using LibraryFactionConstPtr = const LibraryFaction*;
struct LibraryFactionHeroClass;
using LibraryFactionHeroClassConstPtr = const LibraryFactionHeroClass*;

struct LibraryHeroSpec;
using LibraryHeroSpecConstPtr = const LibraryHeroSpec*;

struct LibraryHero;
using LibraryHeroConstPtr = const LibraryHero*;

struct LibrarySecondarySkill;
using LibrarySecondarySkillConstPtr = const LibrarySecondarySkill*;

struct LibrarySpell;
using LibrarySpellConstPtr = const LibrarySpell*;

struct LibraryTerrain;
using LibraryTerrainConstPtr = const LibraryTerrain*;

struct LibraryResource;
using LibraryResourceConstPtr = const LibraryResource*;

struct LibraryMapObject;
using LibraryMapObjectConstPtr = const LibraryMapObject*;


}
