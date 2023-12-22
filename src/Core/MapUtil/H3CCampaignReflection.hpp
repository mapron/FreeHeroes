/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "H3CCampaign.hpp"

#include "MernelReflection/EnumTraits.hpp"
#include "MernelReflection/MetaInfoMacro.hpp"

namespace Mernel::Reflection {
using namespace FreeHeroes;

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3CCampaign,
    m_version,
    m_campId,
    m_name,
    m_description,
    m_musicId,
    m_difficultyChoosenByPlayer)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3CCampaign::PrologEpilog,
    m_enabled,
    m_video,
    m_music,
    m_text)

STRUCT_REFLECTION_STRINGIFY_OFFSET_2(
    H3CCampaign::Scenario,
    m_filename,
    m_packedMapSize,
    m_preconditions,
    m_regionColor,
    m_difficulty,
    m_regionText,
    m_prolog,
    m_epilog)

}
