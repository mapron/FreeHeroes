/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <json_fwd.hpp>

#include <rttr/type>

namespace FreeHeroes::Core::Reflection {

nlohmann::json serializeToJson(rttr::instance obj);

}
