/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHMap;
struct H3Map;
void convertH3M2FH(const H3Map& src, FHMap& dest, const Core::IGameDatabase* database);
void convertFH2H3M(const FHMap& src, H3Map& dest, const Core::IGameDatabase* database);

}
