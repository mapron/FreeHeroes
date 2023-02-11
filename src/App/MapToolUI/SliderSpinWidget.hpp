/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <QWidget>

class QLabel;
class QSpinBox;
class QSlider;

namespace FreeHeroes {

class SliderSpinWidget : public QWidget {
    Q_OBJECT

public:
    SliderSpinWidget(QWidget* parent);

    void setMinimum(int value);
    void setMaximum(int value);
    void setRange(int min, int max);
    void setValue(int value);

    int getValue() const;

    void setText(const QString& text, int minimumWidth);

private:
    QLabel*   m_label;
    QSpinBox* m_spin;
    QSlider*  m_slider;
};

}
