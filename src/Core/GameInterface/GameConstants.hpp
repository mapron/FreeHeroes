/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes::Core {

[[maybe_unused]] constexpr const char* g_database_HOTA = "hota_base";
[[maybe_unused]] constexpr const char* g_database_SOD  = "sod_base";

enum class GameVersion
{
    Invalid,
    SOD,
    HOTA,
};

}
