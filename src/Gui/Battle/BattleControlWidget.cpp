/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleControlWidget.hpp"
#include "ui_BattleControlWidget.h"

#include "Logger.hpp"
#include "BattleHero.hpp"
#include "BattleControlPlan.hpp"

#include "IReplayHandle.hpp"

#include "SpellBookDialog.hpp"
#include "DependencyInjector.hpp"
#include "DialogUtils.hpp"
#include "LibraryModels.hpp"
#include "FormatUtils.hpp"
#include "UiCommonModel.hpp"

#include <QButtonGroup>
#include <QMouseEvent>
#include <QPainter>

Q_DECLARE_METATYPE(FreeHeroes::Core::BattlePlanAttackParams::Alteration);

namespace FreeHeroes::Gui {
using namespace Core;

namespace {
QString posToStr(const BattlePosition& pos)
{
    return QString("(%1,%2)").arg(pos.x).arg(pos.y);
}
QString wrapDeaths(int deaths)
{
    return QString("<k>%1</k>").arg(deaths);
}
QString wrapDmg(int dmg)
{
    return QString("<d>%1</d>").arg(dmg);
}
QString wrapInfo(QString info)
{
    return QString("<i>%1</i>").arg(info);
}
QString wrapInfo(int info)
{
    return QString("<i>%1</i>").arg(info);
}
QString wrapInfo(qreal info)
{
    return QString("<i>%1</i>").arg(info, 0, 'f', 1);
}
}

BattleControlWidget::BattleControlWidget(Core::IBattleView&     battleView,
                                         Core::IBattleControl&  battleControl,
                                         Core::IAIFactory&      aiFactory,
                                         BattleControlPlan&     controlPlan,
                                         LibraryModelsProvider& modelsProvider,
                                         QWidget*               parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::BattleControlWidget>())
    , m_appSettings(loadDependency<IAppSettings>(parent))
    , m_battleView(battleView)
    , m_battleControl(battleControl)
    , m_aiFactory(aiFactory)
    , m_controlPlan(controlPlan)
    , m_modelsProvider(modelsProvider)
{
    connect(&m_controlPlan, &BattleControlPlan::planUpdated, this, &BattleControlWidget::planUpdate);
    connect(&m_controlPlan, &BattleControlPlan::altUpdated, this, &BattleControlWidget::altUpdate);

    battleView.addNotify(this);

    m_ui->setupUi(this);

    m_ui->logMessages->setFrameStyle(QFrame::Panel);
    m_ui->logMessages->setProperty("borderStyle", "commonDark");
    m_ui->logMessages->installEventFilter(this);

    for (QWidget* child : this->findChildren<QWidget*>())
        child->setFocusPolicy(Qt::NoFocus);

    connect(m_ui->pushButtonWait, &QPushButton::clicked, this, [this] {if (m_humanControlAvailable) m_battleControl.doWait(); });
    connect(m_ui->pushButtonGuard, &QPushButton::clicked, this, [this] {if (m_humanControlAvailable) m_battleControl.doGuard(); });
    connect(m_ui->pushButtonSettings, &QPushButton::clicked, this, &BattleControlWidget::showSettings);
    connect(m_ui->pushButtonSpellBook, &QPushButton::clicked, this, &BattleControlWidget::showSpellBook);
    //connect(m_ui->pushButtonAuto, &QPushButton::clicked, this, &BattleControlWidget::autoPlay);

    connect(&m_aiTimer, &QTimer::timeout, this, &BattleControlWidget::aiCheckTick);
    QTimer::singleShot(2000, this, [this] {
        m_aiTimer.start(200);
    });
    auto& bcIcons = m_modelsProvider.ui()->battleControl;
    // clang-format off
    const QHash<FlatButton*, IAsyncIconPtr> res {
        {m_ui->pushButtonWait     , bcIcons.wait},
        {m_ui->pushButtonGuard    , bcIcons.guard},
        {m_ui->pushButtonSpellBook, bcIcons.spellBook},
        {m_ui->pushButtonSurrender, bcIcons.surrender},
       // {m_ui->pushButtonAuto     , bcIcons.autoCombat},
        {m_ui->pushButtonSettings , bcIcons.settings},

        {m_ui->pushButtonUnitCast   ,  bcIcons.unitCast},
        {m_ui->pushButtonRangeAttack,  bcIcons.rangeAttack},
        {m_ui->pushButtonMeleeAttack,  bcIcons.meleeAttack},
        {m_ui->pushButtonSplashAttack, nullptr},
    };
    // clang-format on

    for (FlatButton* btn : res.keys()) {
        IAsyncIconPtr icns = res[btn];
        if (icns)
            btn->setIcon(icns->get());

        btn->enableHoverTracking(true);
    }

    m_ui->pushButtonSplashAttack->hide();
    m_ui->pushButtonUnitCast->hide();
    m_ui->pushButtonRangeAttack->hide();
    m_ui->pushButtonMeleeAttack->hide();
    m_controlButtons = new QButtonGroup(this);
    for (auto* btn : {
             m_ui->pushButtonWait,
             m_ui->pushButtonGuard,
             m_ui->pushButtonSpellBook,
         }) {
        m_controlButtons->addButton(btn);
    }
    m_controlButtons->setExclusive(false);
    m_alternateButtons = new QButtonGroup(this);
    for (auto* btn : {
             m_ui->pushButtonRangeAttack,
             m_ui->pushButtonMeleeAttack,
             m_ui->pushButtonSplashAttack,
             m_ui->pushButtonUnitCast,
         }) {
        btn->setCheckable(true);
        btn->setAutoExclusive(false);
        btn->setAdditionalCheckedHighlight(true);
        m_alternateButtons->addButton(btn);
    }
    m_alternateButtons->setExclusive(false);

    m_replayButtons = new QButtonGroup(this);
    for (auto* btn : {
             m_ui->pushButtonReplayPlay,
             m_ui->pushButtonReplayStop,
             m_ui->pushButtonReplayNext,
             m_ui->pushButtonReplayPause,
         }) {
        m_replayButtons->addButton(btn);
    }
    m_replayButtons->setExclusive(false);

    for (auto* pb : m_replayButtons->buttons())
        pb->hide();

    m_ui->pushButtonReplayPlay->setIcon(QIcon(":/Battle/replayPlay.png"));
    m_ui->pushButtonReplayStop->setIcon(QIcon(":/Battle/replayStop.png"));
    m_ui->pushButtonReplayNext->setIcon(QIcon(":/Battle/replayNext.png"));
    m_ui->pushButtonReplayPause->setIcon(QIcon(":/Battle/replayPause.png"));

    connect(m_ui->pushButtonReplayPlay, &QPushButton::clicked, this, [this] {
        m_replayOnPause = false;
        m_ui->pushButtonReplayPlay->setVisible(m_replayOnPause);
        m_ui->pushButtonReplayPause->setVisible(!m_replayOnPause);
    });
    connect(m_ui->pushButtonReplayPause, &QPushButton::clicked, this, [this] {
        m_replayOnPause = true;
        m_ui->pushButtonReplayPlay->setVisible(m_replayOnPause);
        m_ui->pushButtonReplayPause->setVisible(!m_replayOnPause);
    });
    connect(m_ui->pushButtonReplayNext, &QPushButton::clicked, this, &BattleControlWidget::executeReplayStep);
    connect(m_ui->pushButtonReplayStop, &QPushButton::clicked, this, &BattleControlWidget::stopReplay);
    connect(m_alternateButtons, qOverload<QAbstractButton*>(&QButtonGroup::buttonClicked), this, [this](QAbstractButton* btn) {
        if (!btn->isChecked()) {
            btn->setChecked(true);
            return;
        }
        auto index = btn ? btn->property("altIndex").value<Core::BattlePlanAttackParams::Alteration>() : Core::BattlePlanAttackParams::Alteration::None;
        m_controlPlan.setButtonsAltIndex(index);
        m_controlPlan.m_planCastParams.m_isUnitCast = btn == m_ui->pushButtonUnitCast;
        altUpdate();
    });
    m_ui->pushButtonRangeAttack->setAutoExclusive(true);

    connect(m_ui->pushButtonSurrender, &QPushButton::clicked, this, &BattleControlWidget::surrenderBattle);
}

BattleControlWidget::~BattleControlWidget()
{
}

void BattleControlWidget::setAI(bool attacker, bool defender)
{
    m_aiAttacker = attacker;
    m_aiDefender = defender;
    createAIIfNeeded();
}

void BattleControlWidget::setReplay(IReplayHandle* replayHandle)
{
    m_replayHandle   = replayHandle;
    m_replayIsActive = !!replayHandle;
    updateReplayControls();
}

void BattleControlWidget::doWait()
{
    if (m_humanControlAvailable)
        m_ui->pushButtonWait->click();
}

void BattleControlWidget::doGuard()
{
    if (m_humanControlAvailable)
        m_ui->pushButtonGuard->click();
}

void BattleControlWidget::switchSplash()
{
    if (m_humanControlAvailable) {
        if (m_ui->pushButtonSplashAttack->isChecked())
            m_ui->pushButtonRangeAttack->click();
        else
            m_ui->pushButtonSplashAttack->click();
    }
}

void BattleControlWidget::beforeMove(BattleStackConstPtr stack, const BattlePositionPath& path)
{
    if (!m_appSettings.battle().logMoves)
        return;
    addLog(tr("%1 %2 from %3 to %4")
               .arg(localizedNameWithCount(stack),
                    tr("moving", "", stack->count),
                    posToStr(stack->pos.mainPos()),
                    posToStr(path.back())));
}

void BattleControlWidget::beforeAttackMelee(BattleStackConstPtr stack, const AffectedPhysical& affected, bool isRetaliation)
{
    const QString damageStr = tr("%1 %2 total damage (N * %3 * %4)")
                                  .arg(tr("dealing", "", stack->count),
                                       wrapDmg(affected.main.damage.loss.damageTotal),
                                       wrapInfo(qreal(affected.main.damage.damageBaseRoll) / stack->count),
                                       wrapInfo(QString::number(affected.main.damage.damagePercent) + "%"));

    const QString deathsStr = (affected.main.damage.isKilled()) ? tr(", killing all %1 creatures")
                                                                      .arg(wrapDeaths(affected.main.damage.loss.deaths))
                              : (affected.main.damage.loss.deaths > 0) ? tr(", deaths %1 (remain %2)")
                                                                             .arg(wrapDeaths(affected.main.damage.loss.deaths),
                                                                                  wrapInfo(affected.main.damage.loss.remainCount))
                                                                       : "";

    const QString attackDescrStr = (isRetaliation ? tr("retaliate on", "", stack->count) : tr("attack", "", stack->count));

    QString msg;
    if (affected.main.stack)
        msg = tr("%1 %2 %3, %4 %5").arg(localizedNameWithCount(stack), attackDescrStr, localizedNameWithCount(affected.main.stack, true), damageStr, deathsStr);
    else
        msg = tr("%1 %2").arg(localizedNameWithCount(stack), attackDescrStr);

    if (!affected.extra.empty())
        msg += tr(", splash has done damage to: ");

    for (size_t i = 0; i < affected.extra.size(); i++) {
        auto loss     = affected.extra[i].damage.loss;
        auto affstack = affected.extra[i].stack;
        auto name     = m_modelsProvider.units()->find(affstack->library)->getName();
        if (i > 0)
            msg += ", ";
        if (loss.remainCount && loss.deaths)
            msg += tr("%1 - dmg. %2, deaths %3 (remain %4) ")
                       .arg(name,
                            wrapDmg(loss.damageTotal),
                            wrapDeaths(loss.deaths),
                            wrapInfo(loss.remainCount));
        else if (!loss.deaths)
            msg += tr("%1 - dmg. %2").arg(name, wrapDmg(loss.damageTotal));
        else
            msg += tr("%1 - dmg. %2, killing all %3").arg(name, wrapDmg(loss.damageTotal), wrapDeaths(loss.deaths));
    }

    addLog(msg);
}

void BattleControlWidget::beforeAttackRanged(BattleStackConstPtr stack, const AffectedPhysical& affected)
{
    beforeAttackMelee(stack, affected, false);
}

void BattleControlWidget::beforeWait(BattleStackConstPtr stack)
{
    addLog(tr("%1 %2 for better move")
               .arg(localizedNameWithCount(stack))
               .arg(tr("is waiting", "", stack->count)));
}

void BattleControlWidget::beforeGuard(BattleStackConstPtr stack, int bonus)
{
    addLog(tr("%1 %2 defending position and get +%3 defense")
               .arg(localizedNameWithCount(stack))
               .arg(tr("is taking", "", stack->count))
               .arg(bonus));
}

void BattleControlWidget::onStateChanged()
{
    if (!m_humanControlAvailable)
        return;

    m_controlPlan.setButtonsAltIndex(Core::BattlePlanAttackParams::Alteration::None);

    auto available                      = this->m_battleView.getAvailableActions();
    m_controlPlan.m_currentAlternatives = available.alternatives;
    m_ui->pushButtonWait->setEnabled(available.wait);
    m_ui->pushButtonGuard->setEnabled(available.guard);
    m_ui->pushButtonSpellBook->setEnabled(available.heroCast);

    m_ui->pushButtonUnitCast->setVisible(available.cast);
    m_ui->pushButtonSplashAttack->setVisible(available.splashAttack);
    m_ui->pushButtonMeleeAttack->setVisible(available.meleeAttack);
    m_ui->pushButtonRangeAttack->setVisible(available.rangeAttack);

    m_ui->pushButtonUnitCast->setEnabled(available.cast);
    m_ui->pushButtonSplashAttack->setEnabled(available.splashAttack);
    m_ui->pushButtonMeleeAttack->setEnabled(available.meleeAttack);
    m_ui->pushButtonRangeAttack->setEnabled(available.rangeAttack);

    for (auto* pb : m_alternateButtons->buttons()) {
        pb->setProperty("altIndex", QVariant::fromValue(Core::BattlePlanAttackParams::Alteration::None));
    }
    if (available.rangeAttack)
        m_ui->pushButtonMeleeAttack->setProperty("altIndex", QVariant::fromValue(Core::BattlePlanAttackParams::Alteration::ForceMelee));

    if (available.splashAttack) {
        auto guiUnit = m_modelsProvider.units()->find(m_battleView.getActiveStack()->library);
        m_ui->pushButtonSplashAttack->setIcon(guiUnit->getSplashControl());
        m_ui->pushButtonSplashAttack->setProperty("altIndex", QVariant::fromValue(Core::BattlePlanAttackParams::Alteration::FreeAttack));
    }
    altUpdate();
}

void BattleControlWidget::onStartRound(int round)
{
    if (round > 1)
        addLog(" ", false);
    addLog(tr("Starting round <i>%1</i>").arg(round), false);
}

void BattleControlWidget::onStackUnderEffect(BattleStackConstPtr stack, Effect effect)
{
    updateTurnFlags();
    QString fmt;
    // clang-format off
    switch (effect) {
        case Effect::GoodMorale: fmt = tr("%1 - high morale allows one more attack!"); break;
        case Effect::BadMorale : fmt = tr("%1 - low morale forces to skip turn."); break;
        case Effect::GoodLuck  : fmt = tr("%1 - high luck, extra damage!"); break;
        case Effect::BadLuck   : fmt = tr("%1 - low luck, damage penalty."); break;
        case Effect::Resist    : fmt = tr("%1 - resist chance deflects casted spell."); break;
        case Effect::Regenerate: fmt = tr("%1 - regenerates to full health."); break;
    }
    // clang-format on

    addLog(fmt.arg(localizedNameWithCount(stack)), true);
}

void BattleControlWidget::onCast(const Caster& caster, const AffectedMagic& affected, LibrarySpellConstPtr spell)
{
    const QString casterName = makeCasterName(caster);
    QString       msg        = tr("casting %1").arg(wrapInfo(m_modelsProvider.spells()->find(spell)->getName()));
    if (!casterName.isEmpty())
        msg = casterName + ": " + msg;

    if (spell->type == LibrarySpell::Type::Offensive) {
        msg += tr(", as a result:");
        for (size_t i = 0; i < affected.targets.size(); i++) {
            auto loss  = affected.targets[i].loss;
            auto stack = affected.targets[i].stack;
            auto name  = m_modelsProvider.units()->find(stack->library)->getName();
            if (i > 0)
                msg += ", ";
            if (loss.remainCount && loss.deaths)
                msg += tr("%1 - dmg. %2, deaths %3 (remain %4)")
                           .arg(name,
                                wrapDmg(loss.damageTotal),
                                wrapDeaths(loss.deaths),
                                wrapInfo(loss.remainCount));
            else if (!loss.deaths)
                msg += tr("%1 - dmg. %2").arg(name, wrapDmg(loss.damageTotal));
            else
                msg += tr("%1 - dmg. %2, killing all %3").arg(name, wrapDmg(loss.damageTotal), wrapDeaths(loss.deaths));
        }
    } else if (spell->type == LibrarySpell::Type::Rising) {
        msg += tr(", as a result:");
        for (size_t i = 0; i < affected.targets.size(); i++) {
            auto loss  = affected.targets[i].loss;
            auto stack = affected.targets[i].stack;
            auto name  = m_modelsProvider.units()->find(stack->library)->getName();
            if (i > 0)
                msg += ", ";
            msg += tr("%1 - restored %2 units (up to %3)")
                       .arg(name,
                            QString("<r>%1</r>").arg(-loss.deaths),
                            wrapInfo(loss.remainCount));
        }
    }

    addLog(msg);
}

void BattleControlWidget::onSummon(const Caster& caster, LibrarySpellConstPtr, BattleStackConstPtr stack)
{
    const QString casterName = makeCasterName(caster);
    const QString msg        = casterName + ": " + tr("summoning %1").arg(localizedNameWithCount(stack));
    addLog(msg);
}

void BattleControlWidget::onControlAvailableChanged(bool controlAvailable)
{
    updateTurnFlags();
    bool humanAttacker                    = !m_aiAttacker && !m_replayIsActive;
    bool humanDefender                    = !m_aiDefender && !m_replayIsActive;
    m_anyControlAvailable                 = controlAvailable;
    m_humanControlAvailable               = m_anyControlAvailable && ((m_turnIsAttacker && humanAttacker) || (m_turnIsDefender && humanDefender));
    m_controlPlan.m_humanControlAvailable = m_humanControlAvailable;
    for (auto* pb : m_controlButtons->buttons())
        pb->setEnabled(m_humanControlAvailable);
    for (auto* pb : m_alternateButtons->buttons())
        pb->setEnabled(m_humanControlAvailable);
    onStateChanged();
}

void BattleControlWidget::onBattleFinished(Core::BattleResult)
{
    for (auto* pb : m_replayButtons->buttons())
        pb->setEnabled(false);
    for (auto* pb : m_controlButtons->buttons())
        pb->setEnabled(false);
    for (auto* pb : m_alternateButtons->buttons())
        pb->setEnabled(false);
    // m_ui->pushButtonAuto->setEnabled(false);
}

void BattleControlWidget::planUpdate()
{
    QString hint;

    auto formatDamageRoll = [](int low, int high, int avg, bool deaths) -> QString {
        const bool showAverage   = true; // @todo: some sort of settings
        QString    hightlightTag = deaths ? "k" : "d";
        if (low == high)
            return QString("<%1>%2</%1>").arg(hightlightTag).arg(low);

        const QString formatted = showAverage ? QString("<i>%2</i>-<i>%3</i> (~ <%1>%4</%1>)")
                                                    .arg(hightlightTag)
                                                    .arg(low)
                                                    .arg(high)
                                                    .arg(avg)
                                              : QString("<%1>%2</%1>-<%1>%3</%1>")
                                                    .arg(hightlightTag)
                                                    .arg(low)
                                                    .arg(high);
        return formatted;
    };
    auto formatDamageRollSingle = [&formatDamageRoll](int avg, bool deaths) -> QString {
        return formatDamageRoll(avg, avg, avg, deaths);
    };
    auto formatEstimateDamage = [&formatDamageRoll](const DamageEstimate& estimate) -> QString {
        return formatDamageRoll(estimate.lowRoll.loss.damageTotal, estimate.maxRoll.loss.damageTotal, estimate.avgRoll.loss.damageTotal, false);
    };
    auto formatEstimateDeaths = [&formatDamageRoll](const DamageEstimate& estimate) -> QString {
        if (estimate.maxRoll.loss.deaths == 0)
            return "";
        QString str = formatDamageRoll(estimate.lowRoll.loss.deaths, estimate.maxRoll.loss.deaths, estimate.avgRoll.loss.deaths, true);
        return tr(", deaths %1").arg(str);
    };
    auto formatResist = [](const BonusRatio& succRatio) -> QString {
        const int     percent = (succRatio * 100).roundDownInt();
        const QString perc    = percent < 1 ? "<1" : QString("%1").arg(percent);
        return perc;
    };
    auto& movePlan     = m_controlPlan.m_planMove;
    auto& castPlan     = m_controlPlan.m_planCast;
    auto& hoveredStack = m_controlPlan.m_hoveredStack;
    if (movePlan.isValid()) {
        if (!movePlan.m_attackTarget.isEmpty()) {
            auto estimate = movePlan.m_mainDamage;
            if (hoveredStack) {
                hint = tr("Target attack: %1, damage %2%3")
                           .arg(localizedNameWithCount(hoveredStack, true))
                           .arg(formatEstimateDamage(estimate))
                           .arg(formatEstimateDeaths(estimate));
            }
            if (m_appSettings.battle().retaliationHint) {
                auto estimateRet = movePlan.m_retaliationDamage;
                if (estimateRet.isValid) {
                    hint += "<br>" + tr("Possible retaliation: damage %1%2").arg(formatEstimateDamage(estimateRet)).arg(formatEstimateDeaths(estimateRet));
                }
            }
            if (m_appSettings.battle().massDamageHint && movePlan.m_extraAffectedTargets.size() > 0) {
                if (!hint.isEmpty())
                    hint += tr(", extra affected: ");
                for (size_t i = 0; i < movePlan.m_extraAffectedTargets.size(); i++) {
                    auto damageEstimate = movePlan.m_extraAffectedTargets[i].damage;
                    auto stack          = movePlan.m_extraAffectedTargets[i].stack;
                    auto name           = m_modelsProvider.units()->find(stack->library)->getName();
                    if (i > 0)
                        hint += ", ";
                    hint += tr("%1 - damage %2%3").arg(name).arg(formatEstimateDamage(damageEstimate)).arg(formatEstimateDeaths(damageEstimate));
                }
            }
        } else {
            hint = tr("Move to %1").arg(posToStr(movePlan.m_moveTo.mainPos()));
        }
    } else if (castPlan.isValid()) {
        const bool isOffensive     = castPlan.m_spell->type == LibrarySpell::Type::Offensive;
        const int  affectedTargets = castPlan.m_targeted.size();
        hint                       = tr("Cast %1").arg(m_modelsProvider.spells()->find(castPlan.m_spell)->getName());
        if (affectedTargets > 3 || castPlan.m_spell->type == LibrarySpell::Type::Temp) {
            if (isOffensive) {
                hint += " - " + tr("total damage %1, deaths %2").arg(formatDamageRollSingle(castPlan.m_lossTotal.damageTotal, false)).arg(formatDamageRollSingle(castPlan.m_lossTotal.deaths, true));
            }
            BonusRatio magicSuccessChanceMin{ 1, 1 };
            BonusRatio magicSuccessChanceMax{ 1, 100 };
            if (m_appSettings.battle().massDamageHint && isOffensive) {
                hint += tr(", deaths by creature: ");
            }
            int i = 0;
            for (auto& target : castPlan.m_targeted) {
                magicSuccessChanceMin = std::min(magicSuccessChanceMin, target.magicSuccessChance);
                magicSuccessChanceMax = std::max(magicSuccessChanceMax, target.magicSuccessChance);
                if (m_appSettings.battle().massDamageHint && isOffensive) {
                    auto loss  = target.loss;
                    auto stack = target.stack;
                    auto name  = m_modelsProvider.units()->find(stack->library)->getName();
                    if (i++ > 0)
                        hint += ", ";
                    hint += tr("%1 - %2").arg(name).arg(formatDamageRollSingle(loss.deaths, true));
                }
            }

            if (magicSuccessChanceMin != BonusRatio{ 1, 1 } && magicSuccessChanceMax == magicSuccessChanceMin)
                hint += QString(" (<i>%1%</i> ").arg(formatResist(magicSuccessChanceMin)) + tr("succ. ch.") + ")";
            else if (magicSuccessChanceMax != magicSuccessChanceMin)
                hint += QString(" (<i>%1..%2%</i> ").arg(formatResist(magicSuccessChanceMin)).arg(formatResist(magicSuccessChanceMax)) + tr("succ. ch.") + ")";

        } else if (castPlan.m_spell->type == LibrarySpell::Type::Rising && affectedTargets > 0) {
            hint += " - ";
            for (auto& target : castPlan.m_targeted) {
                auto loss  = target.loss;
                auto stack = target.stack;
                auto name  = m_modelsProvider.units()->find(stack->library)->getName();
                hint += tr("%1 - rise %2 (%3 hp)").arg(name, QString("<r>%1</r>").arg(-loss.deaths), QString("<h>%1</h>").arg(-loss.damageTotal));
            }
        } else if (affectedTargets > 0) {
            hint += " - ";
            hint += tr("total damage %1, deaths %2")
                        .arg(formatDamageRollSingle(castPlan.m_lossTotal.damageTotal, false))
                        .arg(formatDamageRollSingle(castPlan.m_lossTotal.deaths, true));

            if (m_appSettings.battle().massDamageHint && castPlan.m_targeted.size() > 1) {
                hint += tr(", affected: ");
                for (size_t i = 0; i < castPlan.m_targeted.size(); i++) {
                    auto loss   = castPlan.m_targeted[i].loss;
                    auto stack  = castPlan.m_targeted[i].stack;
                    auto succ   = castPlan.m_targeted[i].magicSuccessChance;
                    auto reduce = castPlan.m_targeted[i].totalFactor;
                    auto name   = m_modelsProvider.units()->find(stack->library)->getName();
                    if (i > 0)
                        hint += ", ";
                    hint += tr("%1 - damage %2, deaths %3").arg(name, formatDamageRollSingle(loss.damageTotal, false), formatDamageRollSingle(loss.deaths, true));
                    if (succ != BonusRatio{ 1, 1 })
                        hint += " " + tr("(<i>%1%</i> %2)").arg(formatResist(succ), tr("succ. ch."));

                    if (reduce != BonusRatio{ 1, 1 })
                        hint += " " + tr("(<i>%1%</i> %2)").arg(formatResist(reduce), tr("of base"));
                }
            } else if (castPlan.m_targeted.size() == 1) {
                auto succ   = castPlan.m_targeted[0].magicSuccessChance;
                auto reduce = castPlan.m_targeted[0].totalFactor;
                if (succ != BonusRatio{ 1, 1 })
                    hint += " " + tr("(<i>%1%</i> %2)").arg(formatResist(succ), tr("succ. ch."));

                if (reduce != BonusRatio{ 1, 1 })
                    hint += " " + tr("(<i>%1%</i> %2)").arg(formatResist(reduce), tr("of base"));
            }
        } else if (castPlan.m_spell->type == LibrarySpell::Type::Summon) {
            hint += " - ";
            const auto name = m_modelsProvider.units()->find(castPlan.m_spell->summonUnit)->getName(castPlan.m_lossTotal.remainCount);
            hint += " " + tr("<i>%1</i> %2").arg(castPlan.m_lossTotal.remainCount).arg(name);
        }
    } else if (hoveredStack) {
        hint = localizedNameWithCount(hoveredStack);
    }

    hint = FormatUtils::formatBattleLog(hint, false);
    m_ui->hintLabel->setText(hint);
}

void BattleControlWidget::altUpdate()
{
    auto currntAlt = m_controlPlan.getAlt();
    for (auto* pb : m_alternateButtons->buttons()) {
        if (m_ui->pushButtonUnitCast == pb)
            continue;
        auto pbAlt = pb->property("altIndex").value<Core::BattlePlanAttackParams::Alteration>();
        pb->setChecked(pbAlt == currntAlt && !m_controlPlan.m_planCastParams.m_isUnitCast);
    }
    m_ui->pushButtonUnitCast->setChecked(m_controlPlan.m_planCastParams.m_isUnitCast);
}

void BattleControlWidget::addLog(QString msg, bool appendSide)
{
    msg = FormatUtils::formatBattleLog(msg, true);
    if (appendSide && (m_turnIsAttacker || m_turnIsDefender)) {
        msg = FormatUtils::battleSidePrefix(m_turnIsAttacker) + msg;
    }
    m_ui->logMessages->append(msg);
}

void BattleControlWidget::showSpellBook()
{
    auto battleHero = m_battleView.getHero(m_battleView.getCurrentSide());
    if (!battleHero)
        return;
    SpellBookDialog dlg(battleHero->estimated.availableSpells, m_modelsProvider.spells(), m_modelsProvider.ui(), battleHero->mana, false, true, this);
    dlg.exec();

    if (dlg.getSelectedSpell()) {
        m_controlPlan.setHeroSpell(dlg.getSelectedSpell());
    }
}

void BattleControlWidget::showPopupLogs()
{
    QDialog dlg(this);
    dlg.setWindowFlag(Qt::Popup, true);
    DialogUtils::commonDialogSetup(&dlg);

    QVBoxLayout* mainLayout = DialogUtils::makeMainDialogFrame(&dlg);

    {
        QTextEdit* logMessages = new QTextEdit(&dlg);
        logMessages->setFixedSize(QSize(650, 500));
        logMessages->setReadOnly(true);
        logMessages->setFrameStyle(QFrame::Panel);
        logMessages->setProperty("borderStyle", "commonDark");
        logMessages->setHtml(m_ui->logMessages->toHtml());
        mainLayout->addWidget(logMessages);
    }
    mainLayout->addSpacing(10);
    QHBoxLayout* bottomButtons = new QHBoxLayout();
    mainLayout->addLayout(bottomButtons);
    bottomButtons->addStretch();
    bottomButtons->addSpacing(50);
    bottomButtons->addWidget(DialogUtils::makeAcceptButton(&dlg));
    bottomButtons->addSpacing(50);
    bottomButtons->addStretch();
    dlg.updateGeometry();
    dlg.exec();
}

void BattleControlWidget::updateTurnFlags()
{
    m_turnIsAttacker = !m_battleView.isFinished() && m_battleView.getCurrentSide() == BattleStack::Side::Attacker;
    m_turnIsDefender = !m_battleView.isFinished() && m_battleView.getCurrentSide() == BattleStack::Side::Defender;
}

QString BattleControlWidget::localizedNameWithCount(BattleControlWidget::BattleStackConstPtr stack, bool asTarget) const
{
    return m_modelsProvider.units()->find(stack->library)->getNameWithCount(stack->count, asTarget ? GuiUnit::Variation::AsTarget : GuiUnit::Variation::Normal);
}

QString BattleControlWidget::makeCasterName(const Caster& caster) const
{
    QString casterName;
    if (caster.hero)
        casterName = m_modelsProvider.heroes()->find(caster.hero->library)->getName();
    else if (caster.unit)
        casterName = m_modelsProvider.units()->find(caster.unit->library)->getName();
    else if (caster.artifact)
        casterName = m_modelsProvider.artifacts()->find(caster.artifact)->getName();
    return casterName;
}

void BattleControlWidget::autoPlay()
{
    if (m_turnIsAttacker) {
        m_aiAttacker = !m_aiAttacker;
        addLog(m_aiAttacker ? tr("Attacker side now controlled by AI.") : tr("Attacker side now controlled by Player."), false);
    } else if (m_turnIsDefender) {
        m_aiDefender = !m_aiDefender;
        addLog(m_aiDefender ? tr("Defender side now controlled by AI.") : tr("Defender side now controlled by Player."), false);
    }
    createAIIfNeeded();
}

void BattleControlWidget::aiCheckTick()
{
    if (!m_anyControlAvailable)
        return;

    if (m_replayIsActive) {
        if (!m_replayOnPause)
            executeReplayStep();
        return;
    }
    const bool runAi = (m_turnIsAttacker && m_aiAttacker) || (m_turnIsDefender && m_aiDefender);
    if (!runAi)
        return;

    m_ai->runStep();
}

void BattleControlWidget::stopReplay()
{
    m_replayIsActive = false;
    updateReplayControls();
    onStateChanged();
}

void BattleControlWidget::executeReplayStep()
{
    if (!m_replayHandle->executeCurrent()) {
        stopReplay();
    }
}

void BattleControlWidget::updateReplayControls()
{
    for (auto* pb : m_controlButtons->buttons())
        pb->setVisible(!m_replayIsActive);
    for (auto* pb : m_alternateButtons->buttons())
        pb->setVisible(!m_replayIsActive);
    for (auto* pb : m_replayButtons->buttons()) {
        pb->setVisible(m_replayIsActive);
        m_ui->pushButtonReplayPlay->setVisible(m_replayIsActive && m_replayOnPause);
        m_ui->pushButtonReplayPause->setVisible(m_replayIsActive && !m_replayOnPause);
    }
}

void BattleControlWidget::createAIIfNeeded()
{
    if ((m_aiAttacker || m_aiDefender) && !m_ai.get()) {
        IAI::AIParams params;
        m_ai = m_aiFactory.makeAI(params, m_battleControl);
    }
}

bool BattleControlWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::RightButton && me->modifiers() == Qt::NoModifier) {
            showPopupLogs();
            event->ignore();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

}
