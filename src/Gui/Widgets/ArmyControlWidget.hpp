/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureFwd.hpp"

#include <QFrame>

#include <memory>
#include <functional>

namespace FreeHeroes {
namespace Core {
class IAdventureSquadControl;
}

namespace Gui {
class GuiAdventureSquad;
class GuiAdventureStack;

class GUIWIDGETS_EXPORT ArmyControlWidget : public QFrame {
    Q_OBJECT
public:
    ArmyControlWidget(QWidget* parent = nullptr);
    ~ArmyControlWidget();

signals:
    void showInfo(const GuiAdventureStack* stack, bool modal);
    void hideInfo();

public:
    void refresh();

    void setSource(const GuiAdventureSquad*      squad,
                   Core::IAdventureSquadControl* adventureSquadControl);

    void paintEvent(QPaintEvent*) override;

private:
    void swapItems(Core::AdventureStackConstPtr first, Core::AdventureStackConstPtr second);
    void equalSplit(Core::AdventureStackConstPtr active);
    void splitOneUnit(Core::AdventureStackConstPtr active);
    void splitOnesFill(Core::AdventureStackConstPtr active);
    void groupTogether(Core::AdventureStackConstPtr active);
    void clearState();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    class UnitButton;
};

}
}
