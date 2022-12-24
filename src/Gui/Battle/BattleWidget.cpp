/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleWidget.hpp"

#include "BattleControlWidget.hpp"
#include "BattleControlPlan.hpp"
#include "BattleFieldItem.hpp"
#include "BattleView.hpp"

// Gui
#include "IMusicBox.hpp"
#include "ICursorLibrary.hpp"
#include "DialogUtils.hpp"
#include "UnitInfoWidget.hpp"
#include "CustomFrames.hpp"
#include "FormatUtils.hpp"
#include "DependencyInjector.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"

// Core
#include "BattleHero.hpp"
#include "BattleStack.hpp"

#include <QSettings>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QDebug>
#include <QLabel>
#include <QIcon>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QPainter>
#include <QBoxLayout>

namespace FreeHeroes::Gui {
using namespace Core;

class GridScene : public QGraphicsScene {
public:
    struct GridSettings {
        int          step  = 10;
        Qt::PenStyle style = Qt::PenStyle::SolidLine;
        int          width = 1;
        QColor       color = Qt::black;
    };

    using QGraphicsScene::QGraphicsScene;

    void addGrid(const GridSettings& grid)
    {
        m_grids << grid;
    }

private:
    void drawBackground(QPainter* painter, const QRectF& rect)
    {
        QGraphicsScene::drawBackground(painter, rect);

        const int                    rectx = static_cast<int>(rect.left());
        const int                    recty = static_cast<int>(rect.top());
        QVarLengthArray<QLineF, 100> lines;
        for (const auto& grid : m_grids) {
            const int gridSize = grid.step;
            qreal     left     = rectx - (rectx % gridSize);
            qreal     top      = recty - (recty % gridSize);

            lines.clear();

            for (qreal x = left; x < rect.right(); x += gridSize)
                lines << QLineF(x, rect.top(), x, rect.bottom());
            for (qreal y = top; y < rect.bottom(); y += gridSize)
                lines << QLineF(rect.left(), y, rect.right(), y);

            painter->setPen(QPen(QBrush(grid.color), grid.width, grid.style));
            painter->drawLines(lines.data(), lines.size());
        }
    }

private:
    QList<GridSettings> m_grids;
};

class HeroInfoWidget : public QWidget {
    DarkFrameLabelIcon*          m_portrait;
    QList<QLabel*>               m_primaryStats;
    QList<QLabel*>               m_rngStats;
    QList<QLabel*>               m_rngStatsIcons;
    QLabel*                      m_manaValue;
    const LibraryModelsProvider* m_modelsProvider;

public:
    HeroInfoWidget(QStringList paramLabels, QWidget* parent)
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
        , m_modelsProvider(loadDependency<LibraryModelsProvider>(parent))
    {
        setAttribute(Qt::WA_ShowWithoutActivating);

        QFont f = this->font();
        f.setPointSize(f.pointSize() - 1);
        this->setFont(f);

        QFrame* mainWidget = new QFrame(this);
        mainWidget->setProperty("borderStyle", "common2");
        mainWidget->setFrameStyle(QFrame::Box);
        {
            QHBoxLayout* layoutWrap = new QHBoxLayout(this);
            layoutWrap->setMargin(0);
            layoutWrap->setSpacing(0);
            layoutWrap->addWidget(mainWidget);
        }
        QVBoxLayout* layout = new QVBoxLayout(mainWidget);
        layout->setMargin(2);
        layout->setSpacing(0);
        {
            QHBoxLayout* row = new QHBoxLayout();
            row->setMargin(0);
            row->setSpacing(0);
            m_portrait = new DarkFrameLabelIcon(this);
            row->addStretch();
            row->addWidget(m_portrait, Qt::AlignHCenter);
            row->addStretch();
            m_portrait->setFixedSize(70, 70);
            layout->addLayout(row);
        }

        {
            DarkFrame* statWrap = new DarkFrame(this);
            statWrap->setFixedSize(71, 55);
            layout->addWidget(statWrap);
            QVBoxLayout* layoutStat = new QVBoxLayout(statWrap);
            layoutStat->setMargin(2);
            layoutStat->setSpacing(1);
            QStringList statTitles = paramLabels.mid(0, 4);
            for (auto title : statTitles) {
                QHBoxLayout* row = new QHBoxLayout();
                layoutStat->addLayout(row);
                row->setMargin(0);
                row->setSpacing(0);
                QLabel* lblTitle = new QLabel(title, this);
                row->addWidget(lblTitle, Qt::AlignLeft);

                QLabel* lblValue = new QLabel(this);
                lblValue->setAlignment(Qt::AlignRight);
                m_primaryStats << lblValue;
                row->addWidget(lblValue, Qt::AlignRight);
            }
        }

        {
            DarkFrame* rngWrap = new DarkFrame(this);
            layout->addWidget(rngWrap);
            rngWrap->setFixedSize(71, 35);
            QVBoxLayout* layoutRng = new QVBoxLayout(rngWrap);
            layoutRng->setMargin(2);
            layoutRng->setSpacing(1);
            QStringList rngTitles = paramLabels.mid(4, 2);
            for (auto title : rngTitles) {
                QHBoxLayout* row = new QHBoxLayout();
                layoutRng->addLayout(row);
                row->setMargin(0);
                row->setSpacing(0);
                QLabel* lblTitle = new QLabel(title, this);
                row->addWidget(lblTitle, 0);
                row->addStretch(1);

                QLabel* lblIcon = new QLabel(this);
                lblIcon->setFixedSize(22, 12);
                lblIcon->setFrameStyle(QFrame::NoFrame);
                row->addWidget(lblIcon, 0);
                m_rngStatsIcons << lblIcon;
                QLabel* lblValue = new QLabel(this);
                lblValue->setFrameStyle(QFrame::NoFrame);
                m_rngStats << lblValue;
                row->addWidget(lblValue, 0);
            }
        }

        {
            DarkFrame* manaWrap = new DarkFrame(this);
            layout->addWidget(manaWrap);
            manaWrap->setFixedSize(71, 30);
            QVBoxLayout* layoutMana = new QVBoxLayout(manaWrap);
            layoutMana->setMargin(0);
            layoutMana->setSpacing(0);
            QLabel* manaTitle = new QLabel(paramLabels.value(6), this);
            manaTitle->setAlignment(Qt::AlignCenter);
            m_manaValue = new QLabel(this);
            m_manaValue->setAlignment(Qt::AlignCenter);
            layoutMana->addWidget(manaTitle, Qt::AlignCenter);
            layoutMana->addWidget(m_manaValue, Qt::AlignCenter);
        }
        setFixedSize(75, 200);
    }

    void updateInfo(BattleHeroConstPtr hero, const IAppSettings::UI& uiSettings)
    {
        m_portrait->setPixmap(m_modelsProvider->heroes()->find(hero->library)->getPortraitLarge());
        const int currentSP = hero->estimated.primary.magic.spellPower;
        const int advSP     = hero->adventure->estimated.primary.magic.spellPower;

        m_primaryStats[0]->setText(QString("%1").arg(hero->estimated.primary.ad.attack));
        m_primaryStats[1]->setText(QString("%1").arg(hero->estimated.primary.ad.defense));
        m_primaryStats[2]->setText(FormatUtils::formatSequenceInt(advSP, advSP, currentSP));
        m_primaryStats[3]->setText(QString("%1").arg(hero->estimated.primary.magic.intelligence));

        const int morale        = hero->estimated.squadRngParams.morale;
        const int luck          = hero->estimated.squadRngParams.luck;
        const int moraleClamped = std::clamp(morale, -3, 3);
        const int luckClamped   = std::clamp(luck, -3, 3);
        const int moraleDisplay = uiSettings.clampAbsMoraleLuck ? moraleClamped : morale;
        const int luckDisplay   = uiSettings.clampAbsMoraleLuck ? luckClamped : luck;

        if (uiSettings.displayAbsMoraleLuck) {
            m_rngStats[0]->setText(QString("%1").arg(moraleDisplay));
            m_rngStats[1]->setText(QString("%1").arg(luckDisplay));
        }

        m_rngStatsIcons[0]->setPixmap(m_modelsProvider->ui()->morale.small[moraleClamped]->get());
        m_rngStatsIcons[1]->setPixmap(m_modelsProvider->ui()->luck.small[luckClamped]->get());

        m_manaValue->setText(QString("%1/%2").arg(hero->mana).arg(hero->adventure->estimated.maxMana));
    }
};

BattleWidget::BattleWidget(Core::IBattleView&               battleView,
                           Core::IBattleControl&            battleControl,
                           Core::IAIFactory&                aiFactory,
                           const LibraryModelsProvider*     modelsProvider,
                           const Core::BattleFieldGeometry& battleGeometry,

                           Gui::IAppSettings* appSettings,
                           QWidget*           parent)
    : QDialog(parent)
    , m_controlPlan(new BattleControlPlan(this))
    , m_cursorLibrary(loadDependency<ICursorLibrary>(parent))
    , m_musicBox(loadDependency<Sound::IMusicBox>(parent))

    , m_battleView(battleView)
    , m_battleControl(battleControl)
    , m_modelsProvider(modelsProvider)
    , m_appSettings(appSettings)

{
    DialogUtils::commonDialogSetup(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    auto* layout = DialogUtils::makeMainDialogFrame(this, true);
    layout->setSpacing(0);

    m_battlefield.reset(new BattleFieldItem(m_cursorLibrary, m_musicBox, m_modelsProvider, m_battleView, m_battleControl, battleGeometry, *m_controlPlan, m_appSettings));

    BattleView* battleViewWidget = new FreeHeroes::Gui::BattleView(this);
    layout->addWidget(battleViewWidget);

    m_battleControlWidget = new FreeHeroes::Gui::BattleControlWidget(battleView, battleControl, aiFactory, *m_controlPlan, m_modelsProvider, this);
    layout->addWidget(m_battleControlWidget);

    this->setCursor(m_cursorLibrary->getOther(ICursorLibrary::Type::PlainArrow));

    m_scene.reset(new GridScene(0, 0, 800, 556));

    m_scene->setBackgroundBrush(Qt::black);

    battleViewWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    battleViewWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    battleViewWidget->setScene(m_scene.get());

    m_scene->addItem(m_battlefield.get());

    QElapsedTimer elapsed;
    qint64        last = 0;

    connect(&m_animationTimer, &QTimer::timeout, this, [this, elapsed, last]() mutable {
        if (!elapsed.isValid()) {
            elapsed.start();
            last = 0;
        }
        const auto elapsedMs = elapsed.elapsed();
        m_battlefield->tick(elapsedMs - last);
        last = elapsedMs;
    });

    m_battlefield->selectCurrentStack();

    connect(m_battlefield.get(), &BattleFieldItem::showInfo, this, &BattleWidget::showInfoWidget);
    connect(m_battlefield.get(), &BattleFieldItem::hideInfo, this, &BattleWidget::hideInfoWidget);
    connect(m_battlefield.get(), &BattleFieldItem::heroInfoShow, this, &BattleWidget::heroInfoShow);

    connect(m_battleControlWidget, &BattleControlWidget::showSettings, this, &BattleWidget::showSettingsDialog);
    connect(m_battleControlWidget, &BattleControlWidget::surrenderBattle, this, &BattleWidget::reject);

    m_battleView.addNotify(this);

    m_battleControlWidget->setFocusPolicy(Qt::NoFocus);

    QStringList paramLabels{
        tr("Att:"),
        tr("Def:"),
        tr("Sp:"),
        tr("Int:"),
        tr("Morale:"),
        tr("Luck:"),
        tr("Mana", "short version")
    };

    m_heroInfoWidgetAttacker = std::make_unique<HeroInfoWidget>(paramLabels, this);
    m_heroInfoWidgetDefender = std::make_unique<HeroInfoWidget>(paramLabels, this);
    m_heroInfoWidgetAttacker->hide();
    m_heroInfoWidgetDefender->hide();
}

void BattleWidget::setBackground(QPixmap back)
{
    if (back.isNull()) {
        QImage   img(800, 556, QImage::Format_RGB888);
        QPainter p(&img);
        p.fillRect(img.rect(), QBrush(Qt::darkGreen));
        back = QPixmap::fromImage(img);
    }

    auto backItem = new QGraphicsPixmapItem(back);
    backItem->setZValue(-1000);
    m_scene->addItem(backItem);
}

void BattleWidget::setAI(bool attacker, bool defender)
{
    m_battleControlWidget->setAI(attacker, defender);
}

void BattleWidget::setReplay(IReplayHandle* replayHandle)
{
    m_battleControlWidget->setReplay(replayHandle);
    m_replayHandle = replayHandle;
}

BattleWidget::~BattleWidget() = default;

void BattleWidget::showSettingsDialog()
{
    m_appSettings->showSettingsEditor(this);
    this->update();
}

void BattleWidget::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_W || event->nativeVirtualKey() == 'W') && event->modifiers() == Qt::NoModifier) {
        event->accept();
        m_battleControlWidget->doWait();
        return;
    }
    if ((event->key() == Qt::Key_G || event->nativeVirtualKey() == 'G') && event->modifiers() == Qt::NoModifier) {
        event->accept();
        m_battleControlWidget->switchSplash();
        return;
    }
    if (((event->key() == Qt::Key_D || event->nativeVirtualKey() == 'D') || event->key() == Qt::Key_Space) && event->modifiers() == Qt::NoModifier) {
        event->accept();
        m_battleControlWidget->doGuard();
        return;
    }
    if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Alt || event->key() == Qt::Key_Control) {
        event->accept();
        if (!m_battleFinished)
            m_controlPlan->setModifiers(event->modifiers());
        return;
    }
    QWidget::keyPressEvent(event);
}

void BattleWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Alt || event->key() == Qt::Key_Control) {
        event->accept();
        m_controlPlan->setModifiers(event->modifiers());
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void BattleWidget::showEvent(QShowEvent* event)
{
    //resize(1050, 720);
    m_animationTimer.start(15); // we really don't care for real timer resolution. if we get at leat 50 fps, that's perfect.
    QDialog::showEvent(event);
    m_musicBox->musicPrepare(Sound::IMusicBox::MusicSet::Battle)->play();
    m_musicBox->effectPrepare({ Sound::IMusicBox::EffectSet::Battle })->play();
}

void BattleWidget::onBattleFinished(BattleResult)
{
    m_battleFinished = true;
    if (!m_replayHandle)
        QTimer::singleShot(1, this, &QDialog::accept);
}

void BattleWidget::showInfoWidget(BattleStackConstPtr stack, QPoint pos)
{
    //this->m_ui->unitInfo->clear();
    if (!stack)
        return;

    m_infoWidget.reset(new UnitInfoWidget(stack,
                                          stack->adventure,
                                          m_modelsProvider,
                                          false,
                                          this));
    DialogUtils::moveWidgetWithinVisible(m_infoWidget.get(), pos + this->pos() + QPoint(40, 100));
    m_infoWidget->show();
}

void BattleWidget::hideInfoWidget()
{
    if (m_infoWidget) {
        m_infoWidget->hide();
        m_infoWidget.release()->deleteLater();
    }
}

void BattleWidget::heroInfoShow(bool attacker, bool defender)
{
    if (attacker) {
        auto hero = m_battleView.getHero(BattleStack::Side::Attacker);
        m_heroInfoWidgetAttacker->updateInfo(hero, m_appSettings->ui());
        m_heroInfoWidgetAttacker->move(this->mapToGlobal({ -m_heroInfoWidgetAttacker->sizeHint().width() - 2, 0 }));
        m_heroInfoWidgetAttacker->show();
    } else {
        m_heroInfoWidgetAttacker->hide();
    }

    if (defender) {
        auto hero = m_battleView.getHero(BattleStack::Side::Defender);
        m_heroInfoWidgetDefender->updateInfo(hero, m_appSettings->ui());
        m_heroInfoWidgetDefender->move(this->mapToGlobal({ this->sizeHint().width() + 2, 0 }));
        m_heroInfoWidgetDefender->show();
    } else {
        m_heroInfoWidgetDefender->hide();
    }
}

}
