/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CustomFrames.hpp"

#include <QAbstractItemView>
#include <QEvent>
#include <QHeaderView>
#include <QListView>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOption>
#include <QStylePainter>
#include <QTreeView>

namespace FreeHeroes::Gui {

FlatButton::FlatButton(QWidget* parent)
    : QPushButton(parent)
{
    setFlat(true);
    m_focusActiveColor = QColor("#E8D478");
}

void FlatButton::enableHoverTracking(bool state)
{
    m_hoverEnabled = state;
}

void FlatButton::setFocusActiveColor(QColor color)
{
    m_focusActiveColor = color;
    update();
}

void FlatButton::setBorderWidth(int borderWidth, bool adjustSize)
{
    m_borderWidth = borderWidth;
    if (adjustSize) {
        int b = borderWidth - m_borderWidth;
        this->setFixedSize(this->size().width() + b * 2, this->size().height() + b * 2);
    }
    update();
}

void FlatButton::setAdditionalCheckedHighlight(bool state)
{
    m_additionalCheckedHighlight = state;
}

void FlatButton::paintEvent(QPaintEvent*)
{
    QIcon   icn = this->icon();
    QPixmap pix;
    // normal, pressed, disabled, hovered.
    // normal    = QIcon::Off +  QIcon::Normal
    // pressed   = QIcon::On  +  QIcon::Normal
    // disabled  = QIcon::Off +  QIcon::Disabled
    // hovered   = QIcon::Off +  QIcon::Selected
    QRect boundingRect = this->rect();
    QRect pixmapRect   = boundingRect.adjusted(m_borderWidth, m_borderWidth, -m_borderWidth, -m_borderWidth);
    bool  isPressed    = m_isPressed || (this->isCheckable() && this->isChecked());
    if (!this->isEnabled()) {
        pix = icn.pixmap(pixmapRect.size(), QIcon::Disabled, QIcon::Off);
    } else if (isPressed) {
        pix = icn.pixmap(pixmapRect.size(), QIcon::Normal, QIcon::On);
    } else if (m_isHovered) {
        //pix = icn.pixmap(iconSize(), QIcon::Selected, QIcon::Off);
        pix = icn.pixmap(pixmapRect.size(), QIcon::Normal, QIcon::Off);
    } else {
        pix = icn.pixmap(pixmapRect.size(), QIcon::Normal, QIcon::Off);
    }
    QPainter p(this);
    p.drawPixmap(pixmapRect.x(), pixmapRect.y(), pix);

    if (isEnabled() && !isChecked() && m_additionalCheckedHighlight) {
        p.setPen(Qt::NoPen);
        p.setBrush(QBrush(QColor(0, 0, 0, 30)));
        p.drawRect(pixmapRect.adjusted(0, 0, -1, -1));
    }
    if (isEnabled() && ((m_isHovered && !m_isPressed) || (isChecked() && m_additionalCheckedHighlight))) {
        QRect          rDraw     = pixmapRect.adjusted(0, 0, -1, -1);
        const qreal    boxWidth  = rDraw.width();
        const qreal    boxHeight = rDraw.height();
        qreal          thick     = 4;
        QList<QPointF> startPoints{ QPointF{ 0, 0 }, QPointF{ 0, 0 }, QPointF{ boxWidth, boxHeight }, QPointF{ boxWidth, boxHeight } };
        QList<QPointF> endPoints{ QPointF{ 0, thick }, QPointF{ thick, 0 }, QPointF{ boxWidth - thick, boxHeight }, QPointF{ boxWidth, boxHeight - thick } };
        for (int i = 0; i < 4; i++) {
            QLinearGradient grad(startPoints[i], endPoints[i]);
            grad.setColorAt(0, QColor(255, 255, 255, 50));
            grad.setColorAt(1, QColor(255, 255, 255, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(QBrush(grad));
            p.drawRect(rDraw);
        }
    }
    if (m_borderWidth && this->hasFocus()) {
        p.setPen(QPen(m_focusActiveColor, m_borderWidth));
        p.setBrush(Qt::NoBrush);
        p.drawRect(boundingRect.adjusted(0, 0, -1, -1));
    }
}

void FlatButton::mousePressEvent(QMouseEvent* ev)
{
    m_isPressed = true;
    repaint();
    QPushButton::mousePressEvent(ev);
}

void FlatButton::mouseReleaseEvent(QMouseEvent* ev)
{
    QPushButton::mouseReleaseEvent(ev);
    m_isPressed = false;
    update();
}

void FlatButton::mouseDoubleClickEvent(QMouseEvent* ev)
{
    QPushButton::mouseDoubleClickEvent(ev);
    emit doubleClicked();
}

bool FlatButton::event(QEvent* ev)
{
    auto res = QPushButton::event(ev);
    if (m_hoverEnabled) {
        if (ev->type() == QEvent::Enter)
            m_isHovered = true;
        else if (ev->type() == QEvent::Leave)
            m_isHovered = false;
        else
            return res;
        update();
    } else {
        m_isHovered = false;
    }
    return res;
}

ResizeableComboBox::ResizeableComboBox(QWidget* parent)
    : QComboBox(parent)
{
    this->setMinimumContentsLength(20);
    this->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
}

void ResizeableComboBox::showPopup()
{
    int iconWidth = this->iconSize().width();
    this->view()->setMinimumWidth(calculateMaxModelWidth() + iconWidth + 30);
    QComboBox::showPopup();
}

int ResizeableComboBox::calculateMaxModelWidth() const
{
    auto srcModel = this->model();
    Q_ASSERT(srcModel);
    int          rc       = srcModel->rowCount();
    int          maxWidth = 0;
    QFontMetrics fm       = this->fontMetrics();
    for (int i = 0; i < rc; ++i) {
        const QString text = srcModel->data(srcModel->index(i, 0)).toString();
        maxWidth           = std::max(maxWidth, fm.horizontalAdvance(text));
    }
    return maxWidth;
}

TreeComboBox::TreeComboBox(QWidget* parent)
    : ResizeableComboBox(parent)
{
    m_view = new QTreeView(parent);

    QStandardItemModel* smodel = new QStandardItemModel(this);
    setModel(smodel);
    m_view->setModel(smodel);

    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setEditTriggers(QTreeView::NoEditTriggers);
    m_view->setAlternatingRowColors(false);
    m_view->setSelectionBehavior(QTreeView::SelectRows);
    m_view->setRootIsDecorated(false);
    m_view->setAllColumnsShowFocus(false);
    m_view->setItemsExpandable(false);
    setView(m_view);
    m_view->header()->setVisible(false);
}

void TreeComboBox::expandAll()
{
    m_view->expandAll();
}

void TreeComboBox::selectIndex(const QModelIndex& index)
{
    setRootModelIndex(index.parent());
    setCurrentIndex(index.row());
    m_view->setCurrentIndex(index);
}

void TreeComboBox::showPopup()
{
    setRootModelIndex(QModelIndex());
    // @todo: calculateMaxModelWidth not accurate here, it does not walk recursive.
    this->view()->setMinimumWidth(calculateMaxModelWidth() + this->iconSize().width() * 2 + 50);
    QComboBox::showPopup();
}

void TreeComboBox::hidePopup()
{
    setRootModelIndex(m_view->currentIndex().parent());
    setCurrentIndex(m_view->currentIndex().row());
    ResizeableComboBox::hidePopup();
}

}
