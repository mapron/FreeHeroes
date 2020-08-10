/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
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
    auto old  = m_alt;
    m_alt = m_altButtons;
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


void BattleControlPlan::setSpell(Core::LibrarySpellConstPtr spell)
{
    if (m_spell == spell)
        return;
    m_spell = spell;
    emit spellChanged();
}

BattleControlPlan::~BattleControlPlan() = default;

}
