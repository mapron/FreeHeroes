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
const QString g_converter      = "LegacyConverterD.exe";
const QString g_mapTool        = "MapToolUID.exe";
#else
const QString g_battleEmulator = "BattleEmulator.exe";
const QString g_converter      = "LegacyConverter.exe";
const QString g_mapTool        = "MapToolUI.exe";
#endif

}

LauncherWindow::LauncherWindow(Mode mode)
    : m_ui(std::make_unique<Ui::LauncherWindow>())
{
    setWindowFlags(windowFlags() &~ Qt::WindowMinMaxButtonsHint);
    m_ui->setupUi(this);
    connect(m_ui->pushButtonEmulator, &QAbstractButton::clicked, this, [] {
        QProcess::startDetached(g_battleEmulator, {});
    });
    connect(m_ui->pushButtonConverter, &QAbstractButton::clicked, this, [] {
        QProcess::startDetached(g_converter, {});
    });
    connect(m_ui->pushButtonGenerator, &QAbstractButton::clicked, this, [] {
        QProcess::startDetached(g_mapTool, {});
    });

    connect(m_ui->pushButtonClose, &QAbstractButton::clicked, this, [this] {
        close();
    });
    setWindowIcon(QIcon(":/Application/Logo/64.png"));
}

LauncherWindow::~LauncherWindow()
{
}

}
