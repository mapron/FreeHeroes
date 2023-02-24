/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsView>

class QComboBox;
class QCheckBox;

namespace FreeHeroes {

struct SpritePaintSettings;

class ScaleWidget : public QWidget {
    Q_OBJECT
public:
    ScaleWidget(SpritePaintSettings* settings, QWidget* parent);

    void scaleChangeProcess(int delta);

signals:
    void scaleChanged();

private:
    void setBaseScale(int percent);

    void scaleIncrease();
    void scaleDecrease();
    void resetScale();

private:
    SpritePaintSettings* const m_settings = nullptr;
    QComboBox* const           m_comboBox = nullptr;
    QCheckBox* const           m_check    = nullptr;
};

class SceneView : public QGraphicsView {
    Q_OBJECT
public:
    SceneView(SpritePaintSettings* settings, QWidget* parent);
    ~SceneView();

    void refreshScale();

signals:
    void scaleChangeRequested(int delta);
    void updateVisible(QRectF visible);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

    void updateVisibleInner();

private:
    SpritePaintSettings* const m_settings = nullptr;
};

}
