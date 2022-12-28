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
    ArmyConfigWidget(const Gui::LibraryModelsProvider* modelProvider,
                     Core::IRandomGenerator*           randomGenerator,
                     QWidget*                          parent);
    ArmyConfigWidget(const std::tuple<const Gui::LibraryModelsProvider*, Core::IRandomGenerator*>& data, QWidget* parent = nullptr)
        : ArmyConfigWidget(std::get<0>(data), std::get<1>(data), parent)
    {}
    ~ArmyConfigWidget();

    void setAIControl(bool isAI);
    bool isAIControl() const;
    void initFromMapObject(Core::LibraryMapBankConstPtr mapObject, int variant);

signals:
    void makeLevelup(int newLevel);

public:
    void refresh();

    void setModels();
    void setSource(Gui::GuiAdventureArmy* army);
    void initHero();

private:
    void generate();
    void showHeroDialog();
    void makeLevelupInternal();

private:
    std::unique_ptr<Ui::ArmyConfigWidget>   m_ui;
    const Gui::LibraryModelsProvider* const m_modelProvider   = nullptr;
    Core::IRandomGenerator* const           m_randomGenerator = nullptr;

    Gui::GuiAdventureArmy* m_army        = nullptr;
    Gui::UnitsFilterModel* m_unitsFilter = nullptr;

    std::unique_ptr<AdventureControl> m_adventureControl;
    std::function<void()>             m_tmpRefresh;
};

}
}
