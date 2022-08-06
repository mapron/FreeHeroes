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

// wrapper functions to hide RTTR details.
bool deserialize(IGameDatabase& idResolver, LibraryFaction& faction, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibrarySecondarySkill& skill, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryUnit& unit, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryHeroSpec& spec, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryArtifact& artifact, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryHero& hero, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibrarySpell& spell, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryResource& obj, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryTerrain& obj, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryMapObject& obj, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, SkillHeroItem& obj, const PropertyTree& jsonObj);
bool deserialize(IGameDatabase& idResolver, LibraryGameRules& obj, const PropertyTree& jsonObj);

bool serialize(const SkillHeroItem& obj, PropertyTree& jsonObj);

}
