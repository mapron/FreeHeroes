/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SceneView.hpp"

#include "SpriteMap.hpp"

#include <QComboBox>
#include <QCheckBox>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QDebug>

namespace FreeHeroes {

ScaleWidget::ScaleWidget(SpritePaintSettings* settings, QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_comboBox(new QComboBox(this))
    , m_check(new QCheckBox("2X", this))
{
    m_comboBox->setInsertPolicy(QComboBox::NoInsert);
    m_comboBox->setEditable(false);
    auto* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    for (int percent : { 5, 6, 7, 8, 10, 12, 15, 18, 21, 25, 29, 33, 37, 43, 50, 56, 62, 68, 75, 87, 100 })
        m_comboBox->addItem(QString("%1%").arg(percent), percent);

    setBaseScale(m_settings->m_viewScalePercent);
    m_check->setChecked(m_settings->m_doubleScale);

    layout->addWidget(m_comboBox);

    layout->addWidget(m_check);

    connect(m_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        const int percent              = m_comboBox->currentData().toInt();
        m_settings->m_viewScalePercent = percent;
        //qWarning() << "scalePercent=" << percent;
        emit scaleChanged();
    });
    connect(m_check, &QCheckBox::clicked, this, [this](bool state) {
        m_settings->m_doubleScale = state;
        emit scaleChanged();
    });
}

void ScaleWidget::scaleChangeProcess(int delta)
{
    if (delta > 0)
        scaleIncrease();
    else if (delta < 0)
        scaleDecrease();
    else
        resetScale();
}

void ScaleWidget::setBaseScale(int percent)
{
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
    setBaseScale(100);
}

SceneView::SceneView(SpritePaintSettings* settings, QWidget* parent)
    : QGraphicsView(parent)
    , m_settings(settings)
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

void SceneView::refreshScale()
{
    const auto ratio = static_cast<double>(m_settings->getEffectiveScale()) / 100.;
    this->resetTransform();
    this->scale(ratio, ratio);
}

void SceneView::mousePressEvent(QMouseEvent* event)
{
    if ((event->modifiers() & Qt::ControlModifier) || event->button() == Qt::MiddleButton) {
        this->setDragMode(ScrollHandDrag);
        if (event->button() == Qt::MiddleButton) {
            if ((event->modifiers() & Qt::ControlModifier)) {
                m_settings->m_doubleScaleTmp = true;
                refreshScale();
                return;
            }
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
    if (m_settings->m_doubleScaleTmp) {
        m_settings->m_doubleScaleTmp = false;
        refreshScale();
    }
}

void SceneView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    if ((event->modifiers() & Qt::ControlModifier) || event->button() == Qt::MiddleButton)
        emit scaleChangeRequested(0);
}

void SceneView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            emit scaleChangeRequested(+1);
        } else if (event->angleDelta().y() < 0) {
            emit scaleChangeRequested(-1);
        }
    }
    QFrame::wheelEvent(event);
}

}
