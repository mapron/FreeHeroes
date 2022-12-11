/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LauncherWindow.hpp"

#include "ui_LauncherWindow.h"

#include <QProcess>

namespace FreeHeroes {

namespace {
#ifndef NDEBUG
const QString g_battleEmulator = "BattleEmulatorD.exe";
#else
const QString g_battleEmulator = "BattleEmulator.exe";
#endif

}

LauncherWindow::LauncherWindow(Mode mode)
    : m_ui(std::make_unique<Ui::LauncherWindow>())
{
    m_ui->setupUi(this);
    connect(m_ui->pushButtonEmulator, &QAbstractButton::clicked, this, [] {
        QProcess::startDetached(g_battleEmulator, {});
    });
}

LauncherWindow::~LauncherWindow()
{
}

}
