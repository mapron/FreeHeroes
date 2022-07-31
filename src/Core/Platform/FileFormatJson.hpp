/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "PropertyTree.hpp"

#include "CorePlatformExport.hpp"

namespace FreeHeroes::Core {

COREPLATFORM_EXPORT bool readJsonFromBuffer(const std::string& buffer, PropertyTree& data);
COREPLATFORM_EXPORT bool writeJsonToBuffer(std::string& buffer, const PropertyTree& data);

}
