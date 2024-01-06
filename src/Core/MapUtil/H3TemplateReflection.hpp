/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

#include "H3Template.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3Template,
    m_packName,
    m_data,
    m_endsWithNL)

}
