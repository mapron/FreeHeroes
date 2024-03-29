/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "BattleReplay.hpp"
#include "MernelPlatform/FsUtils.hpp"

#include "CoreLogicExport.hpp"

namespace FreeHeroes::Core {

class IGameDatabase;
struct CORELOGIC_EXPORT AdventureReplayData {
    BattleReplayData m_bat;
    AdventureState   m_adv;

    bool load(const Mernel::std_path& filename, const Core::IGameDatabase* gameDatabase);
    bool save(const Mernel::std_path& filename) const;
};

}
