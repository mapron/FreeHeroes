/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>
#include <QList>
#include <QTimer>

#include "IBattleControl.hpp"
#include "IBattleView.hpp"
#include "IBattleNotify.hpp"
#include "IAIFactory.hpp"

namespace Ui {
class BattleControlWidget;
}
class QButtonGroup;
class QPushButton;
namespace FreeHeroes::Core {
class IReplayHandle;
}
namespace FreeHeroes::Gui {
class BattleControlPlan;
class IAppSettings;
class LibraryModelsProvider;

class BattleControlWidget : public QWidget
    , public Core::BattleNotifyEmpty {
    Q_OBJECT

public:
    explicit BattleControlWidget(Core::IBattleView&     battleView,
                                 Core::IBattleControl&  battleControl,
                                 Core::IAIFactory&      aiFactory,
                                 BattleControlPlan&     controlPlan,
                                 LibraryModelsProvider& modelsProvider,
                                 QWidget*               parent = nullptr);
    ~BattleControlWidget();

    void setAI(bool attacker, bool defender);
    void setReplay(Core::IReplayHandle* replayHandle);

    void doWait();
    void doGuard();
    void switchSplash();

    using BattleStackConstPtr  = Core::BattleStackConstPtr;
    using BattlePosition       = Core::BattlePosition;
    using DamageResult         = Core::DamageResult;
    using LibrarySpellConstPtr = Core::LibrarySpellConstPtr;

    // IBattleNotifiers
    void beforeMove(BattleStackConstPtr stack, const Core::BattlePositionPath& path) override;
    void beforeAttackMelee(BattleStackConstPtr stack, const AffectedPhysical& affected, bool isRetaliation) override;
    void beforeAttackRanged(BattleStackConstPtr stack, const AffectedPhysical& affected) override;
    void beforeWait(BattleStackConstPtr stack) override;
    void beforeGuard(BattleStackConstPtr stack, int bonus) override;

    void onStateChanged() override;
    void onStartRound(int round) override;
    void onStackUnderEffect(BattleStackConstPtr stack, Effect effect) override;
    void onCast(const Caster& caster, const AffectedMagic& affected, LibrarySpellConstPtr spell) override;
    void onControlAvailableChanged(bool controlAvailable) override;
    void onBattleFinished(Core::BattleResult) override;

signals:

    void showSettings();
    void surrenderBattle();

private: /* slots */
    void planUpdate();
    void altUpdate();
    void stopReplay();
    void autoPlay();
    void showSpellBook();
    void showPopupLogs();
    void updateTurnFlags();

private:
    QString localizedNameWithCount(BattleStackConstPtr stack, bool asTarget = false) const;

    void addLog(QString msg, bool appendSide = true);
    void aiCheckTick();

    void executeReplayStep();
    void updateReplayControls();
    void createAIIfNeeded();

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    std::unique_ptr<Ui::BattleControlWidget> m_ui;
    IAppSettings&                            m_appSettings;

    Core::IBattleView&     m_battleView;
    Core::IBattleControl&  m_battleControl;
    Core::IAIFactory&      m_aiFactory;
    BattleControlPlan&     m_controlPlan;
    LibraryModelsProvider& m_modelsProvider;

    Core::IReplayHandle*       m_replayHandle = nullptr;
    std::unique_ptr<Core::IAI> m_ai;
    bool                       m_aiAttacker     = false;
    bool                       m_aiDefender     = false;
    bool                       m_turnIsAttacker = false;
    bool                       m_turnIsDefender = false;

    bool          m_anyControlAvailable   = false;
    bool          m_humanControlAvailable = false;
    bool          m_replayOnPause         = true;
    bool          m_replayIsActive        = false;
    QTimer        m_aiTimer;
    QButtonGroup* m_controlButtons;
    QButtonGroup* m_replayButtons;
    QButtonGroup* m_alternateButtons;
};

}
