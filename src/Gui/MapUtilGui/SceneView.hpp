/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsView>

class QComboBox;

namespace FreeHeroes {

class ScaleWidget : public QWidget {
    Q_OBJECT
public:
    ScaleWidget(QWidget* parent);

    int  getScale() const;
    void setScale(int percent);

    void scaleIncrease();
    void scaleDecrease();
    void resetScale();

signals:
    void scaleChanged(int percent);

private:
    QComboBox* m_comboBox = nullptr;
};

class SceneView : public QGraphicsView {
    Q_OBJECT
public:
    SceneView(QWidget* parent);
    ~SceneView();

    void setScaleWidget(ScaleWidget* scaleWidget);

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

private:
    ScaleWidget* m_scaleWidget = nullptr;
};

}
