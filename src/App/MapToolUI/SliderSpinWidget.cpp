/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "SliderSpinWidget.hpp"

#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QBoxLayout>

namespace FreeHeroes {

SliderSpinWidget::SliderSpinWidget(QWidget* parent)
    : m_label(new QLabel(parent))
    , m_spin(new QSpinBox(parent))
    , m_slider(new QSlider(Qt::Orientation::Horizontal, parent))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(m_label);
    layout->addWidget(m_slider);
    layout->addWidget(m_spin);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(m_slider, &QSlider::valueChanged, m_spin, &QSpinBox::setValue);
    connect(m_spin, qOverload<int>(&QSpinBox::valueChanged), m_slider, &QSlider::setValue);
}

void SliderSpinWidget::setMinimum(int value)
{
    m_slider->setMinimum(value);
    m_spin->setMinimum(value);
}

void SliderSpinWidget::setMaximum(int value)
{
    m_slider->setMaximum(value);
    m_spin->setMaximum(value);
}

void SliderSpinWidget::setRange(int min, int max)
{
    setMinimum(min);
    setMaximum(max);
}

void SliderSpinWidget::setValue(int value)
{
    m_slider->setValue(value);
    m_spin->setValue(value);
}

int SliderSpinWidget::getValue() const
{
    return m_spin->value();
}

void SliderSpinWidget::setText(const QString& text, int minimumWidth)
{
    m_label->setText(text);
    m_label->setMinimumWidth(minimumWidth);
}

}
