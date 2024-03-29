/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "LibraryUnit.hpp"
#include "LibrarySpell.hpp"

#include "BattleFieldPosition.hpp"
#include "BattleFwd.hpp"

namespace FreeHeroes::Core {

struct BattlePlanCast {
    BattlePosition       m_castPosition;
    LibrarySpellConstPtr m_spell   = nullptr;
    bool                 m_isValid = false;

    DamageResult::Loss m_lossTotal;

    std::set<BattlePosition> m_affectedArea;
    struct Target {
        BattleStackConstPtr stack = nullptr;
        DamageResult::Loss  loss;
        BonusRatio          magicSuccessChance{ 1, 1 };
        BonusRatio          totalFactor{ 1, 1 };
    };
    std::vector<Target> m_targeted;

    SpellCastParams m_power;

    void clear() noexcept
    {
        m_spell   = nullptr;
        m_isValid = false;
    }
    bool isValid() const noexcept { return m_spell != nullptr && m_isValid; }
};
struct BattlePlanCastParams {
    BattlePosition       m_target;
    LibrarySpellConstPtr m_spell      = nullptr;
    bool                 m_isHeroCast = false;
    bool                 m_isUnitCast = false;

    void clear() noexcept
    {
        m_isHeroCast = false;
        m_isUnitCast = false;
    }
    bool isActive() const noexcept { return m_isHeroCast || m_isUnitCast; }
};

}
