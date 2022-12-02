/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "PropertyTree.hpp"

#include "CorePlatformExport.hpp"

namespace FreeHeroes::Core {

/// @todo: rewrite noexcept version as wrappers over throwing.

COREPLATFORM_EXPORT bool readJsonFromBuffer(const std::string& buffer, PropertyTree& data) noexcept(true);
COREPLATFORM_EXPORT bool writeJsonToBuffer(std::string& buffer, const PropertyTree& data) noexcept(true);

COREPLATFORM_EXPORT PropertyTree readJsonFromBufferThrow(const std::string& buffer) noexcept(false);
COREPLATFORM_EXPORT std::string writeJsonToBufferThrow(const PropertyTree& data) noexcept(false);

}
