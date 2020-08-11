/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IBattleControl.hpp"

#include <QObject>

#include <set>

namespace FreeHeroes::Gui {


class BattleControlPlan : public QObject
{
    Q_OBJECT
public:
    BattleControlPlan(QObject * parent);
    ~BattleControlPlan();

    void updateAlt();

    void sendPlanUpdated();
    void setModifiers(Qt::KeyboardModifiers mods);
    void setButtonsAltIndex(Core::BattlePlanAttackParams::Alteration index);
    void setSpell(Core::LibrarySpellConstPtr spell);

signals:
    void planUpdated();
    void altUpdated();
    void modifiersChanged();
    void spellChanged();

    // @todo: no public please?
public:
    Core::BattlePlanMove m_planMove;
    Core::BattlePlanMoveParams m_planMoveParams;
    Core::BattlePlanAttackParams m_planAttackParams;
    Core::BattlePlanCast m_planCast;
    Core::BattlePlanCastParams m_planCastParams;


    Core::BattleStackConstPtr m_hoveredStack = nullptr;
    Core::BattleStackConstPtr m_selectedStack = nullptr;

    Qt::KeyboardModifiers currentModifiers() const { return m_currentModifiers;}
    Core::BattlePlanAttackParams::Alteration getAlt() const { return m_alt; }
    Core::BattlePlanAttackParams::Alteration getAltForButtons() const { return m_altButtons;}

    Core::LibrarySpellConstPtr m_spell = nullptr;
    std::vector<Core::BattlePlanAttackParams::Alteration> m_currentAlternatives;

    bool m_humanControlAvailable = false;
private:
    Core::BattlePlanAttackParams::Alteration m_alt = Core::BattlePlanAttackParams::Alteration::None;
    Qt::KeyboardModifiers m_currentModifiers = Qt::NoModifier;
    Core::BattlePlanAttackParams::Alteration m_altButtons = Core::BattlePlanAttackParams::Alteration::None;
};



}
