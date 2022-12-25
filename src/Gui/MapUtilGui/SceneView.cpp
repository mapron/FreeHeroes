/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SceneView.hpp"

#include <QComboBox>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QDebug>

namespace FreeHeroes {

ScaleWidget::ScaleWidget(QWidget* parent)
    : QWidget(parent)
    , m_comboBox(new QComboBox(this))
{
    m_comboBox->setInsertPolicy(QComboBox::NoInsert);
    m_comboBox->setEditable(false);
    auto* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    for (int percent : { 5, 6, 7, 8, 10, 12, 15, 18, 21, 25, 29, 33, 37, 43, 50, 56, 62, 68, 75, 87, 100, 112, 125, 150, 200 })
        m_comboBox->addItem(QString("%1%").arg(percent), percent);

    setScale(100);
    connect(m_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        auto percent = getScale();
        //qWarning() << "scalePercent=" << percent;
        emit scaleChanged(percent);
    });
    layout->addWidget(m_comboBox);
}

int ScaleWidget::getScale() const
{
    const int percent = m_comboBox->currentData().toInt();
    return percent;
}

void ScaleWidget::setScale(int percent)
{
    QSignalBlocker lock(m_comboBox);
    for (int i = 0, cnt = m_comboBox->count(); i < cnt; ++i) {
        if (m_comboBox->itemData(i).toInt() == percent) {
            m_comboBox->setCurrentIndex(i);
            break;
        }
    }
}

void ScaleWidget::scaleIncrease()
{
    if (m_comboBox->currentIndex() < m_comboBox->count() - 1) {
        m_comboBox->setCurrentIndex(m_comboBox->currentIndex() + 1);
    }
}

void ScaleWidget::scaleDecrease()
{
    if (m_comboBox->currentIndex() > 0) {
        m_comboBox->setCurrentIndex(m_comboBox->currentIndex() - 1);
    }
}

void ScaleWidget::resetScale()
{
    setScale(100);
    scaleChanged(100);
}

SceneView::SceneView(QWidget* parent)
    : QGraphicsView(parent)
{
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->setBackgroundBrush(QColor(Qt::black));

    this->setDragMode(NoDrag);
    this->setTransformationAnchor(AnchorUnderMouse);
}

SceneView::~SceneView()
{
}

void SceneView::setScaleWidget(ScaleWidget* scaleWidget)
{
    m_scaleWidget = scaleWidget;
    connect(scaleWidget, &ScaleWidget::scaleChanged, this, [this](int percent) {
        const auto ratio = static_cast<double>(percent) / 100.;
        //qWarning() << "scale=" << ratio;
        this->resetTransform();
        this->scale(ratio, ratio);
    });
}

void SceneView::mousePressEvent(QMouseEvent* event)
{
    if ((event->modifiers() & Qt::ControlModifier) || event->button() == Qt::MiddleButton) {
        this->setDragMode(ScrollHandDrag);
        if (event->button() == Qt::MiddleButton) {
            QMouseEvent pressEvent(QEvent::MouseButtonPress, event->localPos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            mousePressEvent(&pressEvent);
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void SceneView::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);
    this->setDragMode(NoDrag);
}

void SceneView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    if (m_scaleWidget)
        m_scaleWidget->resetScale();
}

void SceneView::wheelEvent(QWheelEvent* event)
{
    if (m_scaleWidget && (event->modifiers() & Qt::ControlModifier)) {
        if (event->angleDelta().y() > 0) {
            m_scaleWidget->scaleIncrease();
        } else if (event->angleDelta().y() < 0) {
            m_scaleWidget->scaleDecrease();
        }
    }
    QFrame::wheelEvent(event);
}

}
