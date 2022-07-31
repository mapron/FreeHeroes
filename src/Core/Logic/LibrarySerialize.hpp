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
}
namespace FreeHeroes::Core::Reflection {

// wrapper functions to hide RTTR details.
class LibraryIdResolver;
bool deserialize(LibraryIdResolver& idResolver, LibraryFaction& faction, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibrarySecondarySkill& skill, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryUnit& unit, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryHeroSpec& spec, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryArtifact& artifact, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryHero& hero, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibrarySpell& spell, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryResource& obj, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryTerrain& obj, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryMapObject& obj, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, SkillHeroItem& obj, const PropertyTree& jsonObj);
bool deserialize(LibraryIdResolver& idResolver, LibraryGameRules& obj, const PropertyTree& jsonObj);

bool serialize(const SkillHeroItem& obj, PropertyTree& jsonObj);

}
