/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ArmyControlWidget.hpp"

// Gui
#include "CustomFrames.hpp"
#include "DependencyInjector.hpp"
#include "AdventureWrappers.hpp"

// Core
#include "IAdventureControl.hpp"
#include "AdventureSquad.hpp"
#include "AdventureStack.hpp"

#include <QBoxLayout>
#include <QListView>
#include <QAbstractListModel>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

namespace FreeHeroes::Gui {
using namespace Core;

class ArmyControlWidget::UnitButton : public DarkFrameLabel {
public:
    const GuiAdventureStack* m_stack      = nullptr;
    bool                     m_isSelected = false;
    ArmyControlWidget*       m_parent     = nullptr;

    UnitButton(ArmyControlWidget* parent)
        : DarkFrameLabel(parent)
        , m_parent(parent)
    {
        this->setProperty("fill", false);
        setProperty("borderStyle", "softDark");
        setFixedSize(QSize(58, 64) + QSize(4, 4));
    }

    void refresh()
    {
        update();
    }

    void paintEvent(QPaintEvent* e) override
    {
        DarkFrameLabel::paintEvent(e);
        QPainter painter(this);

        QRect itemRect      = rect().adjusted(0, 0, -1, -1);
        QRect selectionRect = itemRect.adjusted(2, 2, -2, -2);

        const bool isEmpty = !m_stack || !m_stack->isValid();

        if (!isEmpty) {
            QPixmap img = m_stack->getGuiUnit()->getPortraitLarge();
            painter.drawPixmap(selectionRect.x(), selectionRect.y(), img);
        } else {
            auto cm = painter.compositionMode();
            painter.setCompositionMode(QPainter::CompositionMode_ColorBurn);
            painter.fillRect(selectionRect.adjusted(0, 0, 1, 1), QColor(185, 185, 185, 190)); // @todo: specialColor?;
            painter.setCompositionMode(cm);
        }

        if (m_isSelected) {
            painter.setPen(QPen(QColor("#F1E26C"), 1)); // @ highlight() ?
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(selectionRect);
        }
        if (isEmpty)
            return;

        QFont f = m_parent->font();
        f.setPointSize(13);

        QFontMetrics fm(f);

        const QString count     = QString::number(m_stack->getSource()->count);
        int           textWidth = fm.horizontalAdvance(count);
        //int textHeight = fm.height();

        int textX = selectionRect.x() + selectionRect.width() - textWidth - 2;
        int textY = selectionRect.y() + selectionRect.height() - 2;

        QPainterPath myPath;
        myPath.addText(textX, textY, f, count);

        QPainterPathStroker stroker;
        stroker.setWidth(3);
        const QPainterPath stroked = stroker.createStroke(myPath);

        painter.setBrush(Qt::black);
        painter.setPen(Qt::NoPen);
        painter.drawPath(stroked);

        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.setPen(Qt::white);
        painter.setFont(f);
        painter.drawText(textX, textY, count);
    }

    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
};

struct ArmyControlWidget::Impl {
    const GuiAdventureSquad*      m_squad                 = nullptr;
    Core::IAdventureSquadControl* m_adventureSquadControl = nullptr;
    QList<UnitButton*>            m_buttons;
    QHBoxLayout*                  m_layout = nullptr;
};

ArmyControlWidget::ArmyControlWidget(QWidget* parent)
    : QFrame(parent)
    , m_impl(std::make_unique<Impl>())
{
    m_impl->m_layout = new QHBoxLayout(this);
    m_impl->m_layout->setSpacing(4);
    m_impl->m_layout->setMargin(5);
}

void ArmyControlWidget::refresh()
{
    clearState();
}

ArmyControlWidget::~ArmyControlWidget() = default;

void ArmyControlWidget::setSource(const GuiAdventureSquad*      squad,
                                  Core::IAdventureSquadControl* adventureSquadControl)
{
    m_impl->m_squad                 = squad;
    m_impl->m_adventureSquadControl = adventureSquadControl;

    for (size_t i = 0; i < squad->getCount(); ++i) {
        if (i >= (size_t) m_impl->m_buttons.size()) {
            auto* btn = new UnitButton(this);
            m_impl->m_buttons << btn;
            m_impl->m_layout->addWidget(btn);
        }
        m_impl->m_buttons[i]->m_stack = squad->getStack(i);
    }
    Q_ASSERT(squad->getCount() == (size_t) m_impl->m_buttons.size()); // @todo: add removing of buttons if it become smaller.
}

void ArmyControlWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    painter.fillRect(rect(), QBrush(QColor(230, 0, 0, 100))); // @todo: red color sharing!

    QFrame::paintEvent(e);
}

void ArmyControlWidget::swapItems(AdventureStackConstPtr first, AdventureStackConstPtr second)
{
    IAdventureSquadControl::StackAction act;
    act.type = IAdventureSquadControl::StackActionType::Swap;
    act.from = first;
    act.to   = second;
    m_impl->m_adventureSquadControl->heroStackAction(act);
}

void ArmyControlWidget::equalSplit(AdventureStackConstPtr active)
{
    IAdventureSquadControl::StackAction act;
    act.type = IAdventureSquadControl::StackActionType::EqualSplit;
    act.from = active;
    m_impl->m_adventureSquadControl->heroStackAction(act);
}

void ArmyControlWidget::splitOneUnit(AdventureStackConstPtr active)
{
    IAdventureSquadControl::StackAction act;
    act.type = IAdventureSquadControl::StackActionType::SplitOne;
    act.from = active;
    m_impl->m_adventureSquadControl->heroStackAction(act);
}

void ArmyControlWidget::groupTogether(AdventureStackConstPtr active)
{
    IAdventureSquadControl::StackAction act;
    act.type = IAdventureSquadControl::StackActionType::GroupTogether;
    act.from = active;
    m_impl->m_adventureSquadControl->heroStackAction(act);
}

void ArmyControlWidget::clearState()
{
    for (auto* btn : m_impl->m_buttons) {
        btn->m_isSelected = false;
        btn->refresh();
    }
}

void ArmyControlWidget::UnitButton::mousePressEvent(QMouseEvent* e)
{
    DarkFrameLabel::mousePressEvent(e);

    const bool noMods     = e->modifiers() == Qt::NoModifier;
    const bool shiftMod   = e->modifiers() == Qt::ShiftModifier;
    const bool controlMod = e->modifiers() == Qt::ControlModifier;
    const bool altMod     = e->modifiers() == Qt::AltModifier;

    if (e->button() == Qt::RightButton) {
        if (m_stack->isValid())
            m_parent->showInfo(m_stack, false);
        return;
    }

    if (e->button() != Qt::LeftButton)
        return;

    GuiAdventureStackConstPtr prevSelected = nullptr;
    for (auto* btn : m_parent->m_impl->m_buttons) {
        if (btn->m_isSelected)
            prevSelected = btn->m_stack;
    }

    if (noMods) {
        if (!prevSelected) {
            if (m_stack->isValid()) {
                if (!m_isSelected) {
                    m_parent->clearState();
                    m_isSelected = true;
                    update();
                    return;
                }
                return;
            }
            return;
        }
        if (prevSelected == m_stack) {
            if (m_stack->isValid()) {
                m_parent->showInfo(m_stack, true);
            }
            return;
        }

        if (prevSelected->isValid()) {
            m_parent->clearState();
            m_parent->swapItems(m_stack->getSource(), prevSelected->getSource());
        }
        return;
    }

    if (shiftMod) {
        m_parent->equalSplit(m_stack->getSource());
    } else if (controlMod) {
        m_parent->splitOneUnit(m_stack->getSource());
    } else if (altMod) {
        m_parent->groupTogether(m_stack->getSource());
    }
}

void ArmyControlWidget::UnitButton::mouseReleaseEvent(QMouseEvent* e)
{
    DarkFrameLabel::mouseReleaseEvent(e);
    m_parent->hideInfo();
}

}
