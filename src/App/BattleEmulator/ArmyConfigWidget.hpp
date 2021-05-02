/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <QWidget>

#include <memory>
#include <functional>

namespace Ui {
class ArmyConfigWidget;
}

namespace FreeHeroes {
namespace Core {
struct AdventureArmy;
struct AdventureHero;
class IRandomGenerator;
}
namespace Gui {
class LibraryModelsProvider;
class GuiAdventureArmy;
class UnitsFilterModel;
}

class AdventureControl;
namespace BattleEmulator {

class ArmyConfigWidget : public QWidget {
    Q_OBJECT
public:
    ArmyConfigWidget(QWidget* parent);
    ~ArmyConfigWidget();

    void setAIControl(bool isAI);
    bool isAIControl() const;
    void initFromMapObject(Core::LibraryMapObjectConstPtr mapObject, int variant);

signals:
    void makeLevelup(int newLevel);

public:
    void refresh();

    void setModels(Gui::LibraryModelsProvider& modelProvider, Core::IRandomGenerator* randomGenerator);
    void setSource(Gui::GuiAdventureArmy* army);
    void initHero();

private:
    void generate();
    void showHeroDialog();
    void makeLevelupInternal();

private:
    std::unique_ptr<Ui::ArmyConfigWidget> m_ui;
    Core::IRandomGenerator*               m_randomGenerator = nullptr;
    Gui::GuiAdventureArmy*                m_army            = nullptr;
    Gui::LibraryModelsProvider*           m_modelProvider   = nullptr;
    Gui::UnitsFilterModel*                m_unitsFilter     = nullptr;

    std::unique_ptr<AdventureControl> m_adventureControl;
    std::function<void()>             m_tmpRefresh;
};

}
}
