/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiBattleExport.hpp"

#include "IBattleNotify.hpp"

#include <QDialog>
#include <QStringList>

#include <memory>

class QTableWidgetItem;

namespace FreeHeroes {

namespace Core {
class IBattleView;
class IBattleControl;
class IReplayHandle;
class IAIFactory;
struct BattleFieldGeometry;
}

namespace Sound {
class IMusicBox;
}

namespace Gui {
class ICursorLibrary;
class IAppSettings;
class GridScene;
class BattleFieldItem;
class HeroInfoWidget;
class BattleControlPlan;
class BattleControlWidget;
class LibraryModelsProvider;
class GUIBATTLE_EXPORT BattleWidget : public QDialog
    , public Core::BattleNotifyEmpty {
    Q_OBJECT

public:
    explicit BattleWidget(Core::IBattleView&               battleView,
                          Core::IBattleControl&            battleControl,
                          Core::IAIFactory&                aiFactory,
                          const LibraryModelsProvider*     modelsProvider,
                          const Core::BattleFieldGeometry& battleGeometry,

                          Gui::IAppSettings* appSettings,
                          QWidget*           parent = 0);
    ~BattleWidget();

    void setBackground(QPixmap back);

    void setAI(bool attacker, bool defender);
    void setReplay(Core::IReplayHandle* replayHandle);

    void showSettingsDialog();

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // IBattleNotifiers interface
protected:
    void onBattleFinished(Core::BattleResult result) override;

    void showInfoWidget(Core::BattleStackConstPtr stack, QPoint pos);
    void hideInfoWidget();
    void heroInfoShow(bool attacker, bool defender);

private:
    BattleControlPlan* m_controlPlan = nullptr;

    const ICursorLibrary* m_cursorLibrary;
    Sound::IMusicBox*     m_musicBox;

    Core::IBattleView&           m_battleView;
    Core::IBattleControl&        m_battleControl;
    const LibraryModelsProvider* m_modelsProvider;
    Core::IReplayHandle*         m_replayHandle = nullptr;

    Gui::IAppSettings* m_appSettings;

    std::unique_ptr<GridScene>       m_scene;
    std::unique_ptr<BattleFieldItem> m_battlefield;

    bool m_battleFinished = false;

    std::unique_ptr<QWidget>        m_infoWidget;
    std::unique_ptr<HeroInfoWidget> m_heroInfoWidgetAttacker;
    std::unique_ptr<HeroInfoWidget> m_heroInfoWidgetDefender;

    Gui::BattleControlWidget* m_battleControlWidget = nullptr;
};

}

}
