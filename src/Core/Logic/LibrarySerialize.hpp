/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

namespace Mernel {
class PropertyTree;
}
namespace FreeHeroes::Core {
struct SkillHeroItem;
class IGameDatabase;

void deserialize(const IGameDatabase* gameDatabase, LibraryArtifact& artifact, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryBuilding& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryDwelling& dwelling, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryFaction& faction, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryHero& hero, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryHeroSpec& spec, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryMapBank& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryMapObstacle& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryMapVisitable& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryObjectDef& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryPlayer& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryResource& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibrarySecondarySkill& skill, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibrarySpell& spell, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryTerrain& obj, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, LibraryUnit& unit, const Mernel::PropertyTree& jsonObj);
void deserialize(const IGameDatabase* gameDatabase, SkillHeroItem& obj, const Mernel::PropertyTree& jsonObj);

void deserialize(const IGameDatabase* gameDatabase, LibraryGameRules& obj, const Mernel::PropertyTree& jsonObj);

void serialize(const SkillHeroItem& obj, Mernel::PropertyTree& jsonObj);

}
