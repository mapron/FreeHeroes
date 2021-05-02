/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleControlPlan.hpp"

namespace FreeHeroes::Gui {

BattleControlPlan::BattleControlPlan(QObject* parent)
    : QObject(parent)
{
}

void BattleControlPlan::updateAlt()
{
    auto old = m_alt;
    m_alt    = m_altButtons;
    if (m_currentModifiers.testFlag(Qt::AltModifier) && !m_currentAlternatives.empty()) {
        m_alt = m_currentAlternatives[0];
    }
    if (m_alt != old)
        emit altUpdated();
}

void BattleControlPlan::sendPlanUpdated()
{
    emit planUpdated();
}

void BattleControlPlan::setModifiers(Qt::KeyboardModifiers mods)
{
    if (m_currentModifiers == mods)
        return;
    m_currentModifiers = mods;

    emit modifiersChanged();
    updateAlt();
}

void BattleControlPlan::setButtonsAltIndex(Core::BattlePlanAttackParams::Alteration index)
{
    if (index == m_altButtons)
        return;
    m_altButtons = index;
    updateAlt();
}

void BattleControlPlan::setHeroSpell(Core::LibrarySpellConstPtr spell)
{
    bool isHeroCast = !!spell;
    if (m_planCastParams.m_spell == spell && m_planCastParams.m_isHeroCast == isHeroCast)
        return;

    m_planCastParams.m_spell      = spell;
    m_planCastParams.m_isHeroCast = isHeroCast;

    emit spellChanged();
}

BattleControlPlan::~BattleControlPlan() = default;

}
