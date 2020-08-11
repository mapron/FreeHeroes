/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>
#include <vector>

namespace FreeHeroes::Conversion {

enum Version { VersionSod, VersionHota };
extern const std::vector<std::string> unitIds[2];

extern const std::vector<std::string> artifactIds[2] ;

extern const std::vector<std::string> heroesIds[2] ;

extern const std::vector<std::string> skillsIds[2] ;

extern const std::vector<std::string> spellIds[2] ;

extern const std::vector<std::string> classIds[2] ;

}
