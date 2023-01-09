/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <map>
#include <string>

namespace FreeHeroes::Core {

struct TranslationMap {
    using Data = std::map<std::string, std::string>;
    Data ts;
};

}
