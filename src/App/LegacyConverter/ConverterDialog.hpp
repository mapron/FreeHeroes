/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include <QDialog>
#include <QSettings>

#include <memory>
#include <functional>
#include <thread>

namespace Ui {
class ConverterDialog;
}

namespace FreeHeroes::Core {
class IGameDatabaseContainer;
}

namespace FreeHeroes {
class GameExtract;

class ConverterDialog : public QDialog {
    Q_OBJECT
public:
    ConverterDialog(const Core::IGameDatabaseContainer* databaseContainer, QWidget* parent = nullptr);
    ~ConverterDialog();

signals:
    void progressInternal(int, int);
    void progress(int, int);
    void finished();

private:
    void browsePath();
    void pathChanged();
    void run();
    void cleanUnpackDestination();
    void statusUpdate(QString txt);

private:
    std::unique_ptr<Ui::ConverterDialog>      m_ui;
    std::unique_ptr<GameExtract>              m_extract;
    const Mernel::std_path                    m_appData;
    Mernel::std_path                          m_hotaInstallDir;
    QSettings                                 m_converterSettings;
    const Core::IGameDatabaseContainer* const m_databaseContainer;
    std::thread                               m_thread;
    int                                       m_start = 0;
};

}
