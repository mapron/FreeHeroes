/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <json_fwd.hpp>

namespace FreeHeroes::Core { struct SkillHeroItem; }
namespace FreeHeroes::Core::Reflection {

// wrapper functions to hide RTTR details.
class LibraryIdResolver;
bool deserialize(LibraryIdResolver & idResolver, LibraryFaction & faction, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibrarySecondarySkill & skill, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryUnit & unit, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryHeroSpec & spec, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryArtifact & artifact, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryHero & hero, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibrarySpell & spell, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryResource & obj, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryTerrain & obj, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, LibraryMapObject & obj, const nlohmann::json & jsonObj);
bool deserialize(LibraryIdResolver & idResolver, Core::SkillHeroItem & obj, const nlohmann::json& jsonObj);

bool serialize(const Core::SkillHeroItem & obj, nlohmann::json& jsonObj);

}
