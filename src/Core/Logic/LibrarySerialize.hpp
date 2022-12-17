/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

namespace FreeHeroes {
class PropertyTree;
}
namespace FreeHeroes::Core {
struct SkillHeroItem;
class IGameDatabase;
}
namespace FreeHeroes::Core::Reflection {

bool deserialize(const IGameDatabase* gameDatabase, LibraryArtifact& artifact, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryDwelling& dwelling, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryFaction& faction, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryHero& hero, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryHeroSpec& spec, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryMapBank& obj, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryMapObstacle& obj, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryObjectDef& obj, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryResource& obj, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibrarySecondarySkill& skill, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibrarySpell& spell, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryTerrain& obj, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, LibraryUnit& unit, const PropertyTree& jsonObj);
bool deserialize(const IGameDatabase* gameDatabase, SkillHeroItem& obj, const PropertyTree& jsonObj);

bool deserialize(const IGameDatabase* gameDatabase, LibraryGameRules& obj, const PropertyTree& jsonObj);

bool serialize(const SkillHeroItem& obj, PropertyTree& jsonObj);

}
