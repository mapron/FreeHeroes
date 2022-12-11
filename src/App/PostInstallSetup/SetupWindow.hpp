/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QFrame>

#include <memory>

namespace Ui {
class SetupWindow;
}

namespace FreeHeroes {

class SetupWindow : public QFrame {
public:
    SetupWindow();
    ~SetupWindow();

private:
    std::unique_ptr<Ui::SetupWindow> m_ui;
    bool                             m_accept = false;
};

}
