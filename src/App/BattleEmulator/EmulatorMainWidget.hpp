/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include <memory>

namespace Ui {
class EmulatorMainWidget;
}
namespace FreeHeroes {
namespace Core {
class IGameDatabase;
class IRandomGeneratorFactory;
class IRandomGenerator;
struct AdventureState;
struct AdventureKingdom;
}
namespace Gui {
class IGraphicsLibrary;
class ICursorLibrary;
class IAppSettings;
class LibraryModelsProvider;
class GuiAdventureArmy;
}
namespace Sound {
class IMusicBox;
}

namespace BattleEmulator {
class ReplayFileManager;
class EmulatorMainWidget : public QWidget {
    Q_OBJECT
public:
    EmulatorMainWidget(const Gui::IGraphicsLibrary*         graphicsLibrary,
                       const Gui::ICursorLibrary*           cursorLibrary,
                       const Core::IGameDatabase*           gameDatabase,
                       const Core::IRandomGeneratorFactory* randomGeneratorFactory,
                       Sound::IMusicBox*                    musicBox,
                       Gui::IAppSettings*                   appSettings,
                       const Gui::LibraryModelsProvider*    modelsProvider,
                       QWidget*                             parent = nullptr);
    ~EmulatorMainWidget();

    void startBattle();
    void startReplay();
    void showSettings();

    void show();

private:
    void onAttDataChanged(bool forcedUpdate = false);
    void onDefDataChanged(bool forcedUpdate = false);
    void makeNewDay();
    void makeManaRegen();
    void onTerrainChanged();
    void onObjectPresetChanged();
    void readAdventureStateFromUI();
    void loadAdventureData();
    void applyCurrentObjectRewards(QString defenderName);

    int execBattle(bool isReplay, bool isQuick);

    void checkForHeroLevelUps(bool fromDebugWidget = true);

private:
    std::unique_ptr<Ui::EmulatorMainWidget> m_ui;

    const Gui::IGraphicsLibrary*         m_graphicsLibrary;
    const Gui::ICursorLibrary*           m_cursorLibrary;
    const Core::IGameDatabase*           m_gameDatabase;
    const Core::IRandomGeneratorFactory* m_randomGeneratorFactory;
    Sound::IMusicBox*                    m_musicBox;
    Gui::IAppSettings*                   m_appSettings;
    const Gui::LibraryModelsProvider*    m_modelsProvider;

    std::unique_ptr<Core::AdventureState>   m_adventureState;
    std::unique_ptr<Core::AdventureState>   m_adventureStatePrev;
    std::unique_ptr<Core::AdventureKingdom> m_adventureKingdom;

    std::unique_ptr<Gui::GuiAdventureArmy> m_guiAdventureArmyAtt;
    std::unique_ptr<Gui::GuiAdventureArmy> m_guiAdventureArmyDef;

    std::shared_ptr<Core::IRandomGenerator> m_uiRng;
    std::unique_ptr<ReplayFileManager>      m_replayManager;
};

}
}
