/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TemplateSettingsWindow.hpp"

#include "ui_TemplateSettingsWindow.h"

namespace FreeHeroes {

namespace {

}

TemplateSettingsWindow::TemplateSettingsWindow(QWidget* parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::TemplateSettingsWindow>())
{
    m_ui->setupUi(this);
    connect(m_ui->pushButtonOk, &QAbstractButton::clicked, this, [this] {
        accept();
    });
    connect(m_ui->pushButtonCancel, &QAbstractButton::clicked, this, [this] {
        reject();
    });
}

TemplateSettingsWindow::~TemplateSettingsWindow()
{
}

}
