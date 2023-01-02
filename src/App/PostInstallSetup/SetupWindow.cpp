/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SetupWindow.hpp"

#include "ui_SetupWindow.h"

#include "MernelPlatform/FsUtils.hpp"
#include "MernelPlatform/ShellUtils.hpp"

#include <QSettings>
#include <QCoreApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QProcess>

namespace FreeHeroes {

namespace {
const QString     g_regInstall  = "HKEY_CURRENT_USER\\SOFTWARE\\FreeHeroes";
const QString     g_regKey      = "InstallLocation";
const std::string g_needCleanup = "need_cleanup.txt";
#ifndef NDEBUG
const QString g_launcher = "LauncherD.exe";
#else
const QString g_launcher = "Launcher.exe";
#endif

QString guessInstallationPath()
{
    QSettings reg(g_regInstall, QSettings::NativeFormat);
    if (reg.contains(g_regKey))
        return reg.value(g_regKey).toString();

    return QCoreApplication::applicationDirPath();
}
}

SetupWindow::SetupWindow()
    : m_ui(std::make_unique<Ui::SetupWindow>())
{
    m_ui->setupUi(this);
    m_ui->destPath->setText(guessInstallationPath());
    connect(m_ui->pushButtonFinish, &QAbstractButton::clicked, this, [this] { m_accept = true; close(); });
    connect(m_ui->pushButtonBrowse, &QAbstractButton::clicked, this, [this] {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select root folder for FreeHeroes"));
        if (dir.isEmpty())
            return;
        if (!dir.endsWith("FreeHeroes"))
            dir += "/FreeHeroes";
        m_ui->destPath->setText(dir);
    });
}

SetupWindow::~SetupWindow()
{
    if (!m_accept)
        return;

    QSettings reg(g_regInstall, QSettings::Registry32Format);
    auto      qdest = m_ui->destPath->text();
    reg.setValue(g_regKey, qdest);

    const Mernel::std_path src  = QCoreApplication::applicationDirPath().toStdWString();
    const Mernel::std_path dest = qdest.toStdWString();

    const auto copyOptions = Mernel::std_fs::copy_options::overwrite_existing
                             | Mernel::std_fs::copy_options::recursive;

    if (src != dest)
        Mernel::std_fs::copy(src, dest, copyOptions);

    if (src == dest) {
        if (Mernel::std_fs::exists(src / g_needCleanup))
            Mernel::std_fs::remove(src / g_needCleanup);
    }

    if (m_ui->checkBoxShortcut) {
        const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + QDir::separator();
        Mernel::createShortCut(desktopPath.toStdWString(), "FreeHeroes", dest, g_launcher.toStdString(), "--shortcut");
    }
    if (m_ui->checkBoxStartLauncher) {
        QProcess::startDetached(g_launcher, { "--post-install" }, qdest);
    }
}

}
