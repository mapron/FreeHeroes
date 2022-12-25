/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include "SpriteMap.hpp"

namespace FreeHeroes {

class MiniMapWidget : public QWidget {
public:
    MiniMapWidget(const SpritePaintSettings* settings, const SpriteMap* spriteMap, QWidget* parent);
    ~MiniMapWidget();

    void setDepth(int depth);

protected:
    void paintEvent(QPaintEvent* e);

private:
    const SpritePaintSettings* const m_settings;
    const SpriteMap* const           m_spriteMap;

    int m_depth = 0;
};

}
