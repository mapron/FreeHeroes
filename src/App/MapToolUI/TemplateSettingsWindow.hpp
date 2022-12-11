/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QDialog>

#include <memory>

namespace Ui {
class TemplateSettingsWindow;
}

namespace FreeHeroes {

class TemplateSettingsWindow : public QDialog {
public:
    TemplateSettingsWindow(QWidget* parent);
    ~TemplateSettingsWindow();

private:
    std::unique_ptr<Ui::TemplateSettingsWindow> m_ui;
};

}
