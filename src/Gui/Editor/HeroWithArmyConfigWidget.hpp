/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiEditorExport.hpp"

#include <QWidget>

#include <memory>
#include <functional>


namespace Ui
{
class HeroWithArmyConfigWidget;
}

namespace FreeHeroes::Core {
class IRandomGenerator;
}

namespace FreeHeroes::Gui {

class LibraryModelsProvider;
class GuiAdventureArmy;
class GuiAdventureHero;
class ArtifactQuickFilterModel;

class GUIEDITOR_EXPORT HeroWithArmyConfigWidget : public QWidget
{
    Q_OBJECT
public:
    HeroWithArmyConfigWidget(QWidget * parent = nullptr);
    ~HeroWithArmyConfigWidget();

    void refresh();

    void setModels(LibraryModelsProvider & modelProvider, Core::IRandomGenerator * randomGenerator);
    void setSource(GuiAdventureArmy* adventureArmy);
    void initHero();

signals:
    void showHeroDialog();

private:
    void primaryStatEdited();
    void expEdited();
    void levelEdited();
    void heroIndexChanged();

    void displayHeroSummary();
    void displayHeroPrimary();
    void displayHeroAppearence();
    void displayHeroArtifacts();
    void displayHeroSkills();
    void displayHeroSpells();

    void useHeroDefaultArmy();

    void makeAllSpells();
    void makeNoSpells();
    void makeAllArtifacts();
    void makeNoArtifacts();
    void resetHero();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    std::unique_ptr<Ui::HeroWithArmyConfigWidget> m_ui;

    Core::IRandomGenerator  * m_randomGenerator = nullptr;

    GuiAdventureArmy * m_adventureArmy = nullptr;
    GuiAdventureHero * m_hero = nullptr;
    LibraryModelsProvider * m_modelProvider = nullptr;

    ArtifactQuickFilterModel * m_artifactsFilter = nullptr;
};

}
