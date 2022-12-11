/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QFrame>

#include <memory>

namespace Ui {
class MapToolWindow;
}

namespace FreeHeroes {

class MapToolWindow : public QFrame {
public:
    MapToolWindow();
    ~MapToolWindow();

private:
    std::unique_ptr<Ui::MapToolWindow> m_ui;
};

}
