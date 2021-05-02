/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureFwd.hpp"

#include <QDialog>

#include <memory>
#include <functional>

namespace Ui {
class HeroMainDialog;
}

namespace FreeHeroes {
namespace Core {
class IAdventureSquadControl;
class IAdventureHeroControl;
}

namespace Gui {
class UnitInfoWidget;
class HoverHelper;
class GuiAdventureArmy;
class GuiAdventureHero;
class GuiAdventureStack;
class LibraryModelsProvider;

class GUIWIDGETS_EXPORT HeroMainDialog : public QDialog {
    Q_OBJECT
public:
    HeroMainDialog(QWidget* parent = nullptr);
    ~HeroMainDialog();

public:
    void refresh();

    void setSource(const GuiAdventureArmy*       heroArmy,
                   Core::IAdventureSquadControl* adventureSquadControl,
                   Core::IAdventureHeroControl*  adventureHeroControl,
                   LibraryModelsProvider*        modelsProvider);

private:
    void updateHeroAppearence();

    void deleteStack(Core::AdventureStackConstPtr stack);
    void showInfo(const GuiAdventureStack* stack, bool modal);
    void hideInfo();
    void openSpellBook();
    void updateGraphics();

private:
    std::unique_ptr<Ui::HeroMainDialog> m_ui;

    const GuiAdventureArmy*       m_heroArmy              = nullptr;
    const GuiAdventureHero*       m_hero                  = nullptr;
    Core::IAdventureSquadControl* m_adventureSquadControl = nullptr;
    Core::IAdventureHeroControl*  m_adventureHeroControl  = nullptr;
    LibraryModelsProvider*        m_modelsProvider        = nullptr;

    std::unique_ptr<UnitInfoWidget> m_infoWidget;

    std::unique_ptr<HoverHelper> m_hoverHelper;
};

}
}
