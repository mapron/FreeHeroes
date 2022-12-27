/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>
#include <QVBoxLayout>

namespace FreeHeroes {
struct SpriteMap;
class InspectorWidget : public QWidget {
    Q_OBJECT
public:
    InspectorWidget(const SpriteMap* spriteMap, QWidget* parent);
    ~InspectorWidget();

    void clear();
    void displayInfo(int x, int y, int depth);

private:
    const SpriteMap* const m_spriteMap;
    QVBoxLayout*           m_layout = nullptr;
};

}
