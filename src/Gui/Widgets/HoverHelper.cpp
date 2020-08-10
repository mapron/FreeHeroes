/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "HoverHelper.hpp"

#include "GeneralPopupDialog.hpp"

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QVariant>
#include <QMouseEvent>

namespace  FreeHeroes::Gui {

HoverHelper::HoverHelper(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
{

}

HoverHelper::~HoverHelper()
{

}

void HoverHelper::addWidgets(const QList<QWidget*>& watch)
{
    for (auto * w : watch)
        w->installEventFilter(this);
}

void HoverHelper::addAlias(QWidget* main, QWidget* alias)
{
    m_aliases[alias] = main;
    alias->installEventFilter(this);
    main->installEventFilter(this);
}

void HoverHelper::setHoverLabel(QLabel* label)
{
    m_hoverLabel = label;
}

bool HoverHelper::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave ) {

        QWidget * w = qobject_cast<QWidget*>(watched);
        w = m_aliases.value(w, w);
        if (m_hoverLabel && w) {
            const QString text = w->property("hoverName").toString();
            if (event->type() == QEvent::HoverEnter) {
                if (!text.isEmpty()) {
                    m_hoverLabel->setText(text);
                    w->setProperty("isHovered", true);
                }
            } else if (event->type() == QEvent::HoverLeave) {
                if (!text.isEmpty()) {
                    m_hoverLabel->setText("");
                    w->setProperty("isHovered", false);
                }
            }
        }
    }
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent * me = static_cast<QMouseEvent *>(event);
        if (me->modifiers() == Qt::NoModifier && me->button() == Qt::RightButton) {
            QWidget * w = qobject_cast<QWidget*>(watched);
            w = m_aliases.value(w, w);
            return showPopup(w, false);
        }
        if (me->modifiers() == Qt::NoModifier && me->button() == Qt::LeftButton) {
            QWidget * w = qobject_cast<QWidget*>(watched);
            w = m_aliases.value(w, w);
            return showPopup(w, true);
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent * me = static_cast<QMouseEvent *>(event);
        if (me->modifiers() == Qt::NoModifier && me->button() == Qt::RightButton) {
            QWidget * w = qobject_cast<QWidget*>(watched);
            w = m_aliases.value(w, w);
            return hidePopup(w);
        }
    }
    return QObject::eventFilter(watched, event);
}

bool HoverHelper::showPopup(QWidget * what, bool isModal) {
    if (!what)
        return false;
    QString popupDescr = what->property("popupDescr").toString();
    if (popupDescr.isEmpty())
        return false;

    if (isModal) {
        if (!what->property("popupAllowModal").toBool())
            return false;
    }

   // return true;
    QPixmap popupIcon   = what->property("popupPixmap").value<QPixmap>();
    QString popupBottom = what->property("popupBottom").toString();
    QPoint offset       = what->property("popupOffset").value<QPoint>();
    QString popupOffsetAnchorHor= what->property("popupOffsetAnchorHor").toString();
    QString popupOffsetAnchorVert= what->property("popupOffsetAnchorVert").toString();
    if (m_dialog) {
        m_dialog->deleteLater();
    }
    m_dialog = new GeneralPopupDialog(popupDescr, {{popupIcon, popupBottom}}, isModal, false, m_parent);

    QSize s = m_dialog->sizeHint();
    QPoint globalPos = m_parent->mapToGlobal(offset);
    if (popupOffsetAnchorHor == "center")
        globalPos -= QPoint(s.width() / 2, 0);
    if (popupOffsetAnchorHor == "right")
        globalPos -= QPoint(s.width(), 0);
    if (popupOffsetAnchorVert == "center")
        globalPos -= QPoint(0, s.height() / 2);
    if (popupOffsetAnchorVert == "bottom")
        globalPos -= QPoint(0, s.height());

    m_dialog->move(globalPos );
    if (isModal)
        m_dialog->exec();
    else
        m_dialog->show();
    return true;
}
bool HoverHelper::hidePopup(QWidget * what) {
    QString popupDescr = what->property("popupDescr").toString();
    if (popupDescr.isEmpty())
        return false;

    if (m_dialog) {
        m_dialog->closeNonModal();
    }
    return true;
}

}
