/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHMap;
struct H3Map;
void convertFH2H3M(const FHMap& map, H3Map& dest, const Core::IGameDatabase* database, Core::IRandomGenerator* rng);

}
