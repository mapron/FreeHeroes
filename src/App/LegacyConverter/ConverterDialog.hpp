/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include <QDialog>
#include <QSettings>

#include <memory>
#include <functional>

namespace Ui {
class ConverterDialog;
}

namespace FreeHeroes::Core {
class IGameDatabaseContainer;
}

namespace FreeHeroes::Conversion {

class ConverterDialog : public QDialog {
    Q_OBJECT
public:
    ConverterDialog(const Core::IGameDatabaseContainer* databaseContainer, QWidget* parent = nullptr);
    ~ConverterDialog();

private:
    void browsePath();
    void pathChanged();
    void run();
    void cleanUnpackDestination();
    void statusUpdate(QString txt);

private:
    std::unique_ptr<Ui::ConverterDialog>      m_ui;
    Core::std_path                            m_hotaInstallDir;
    QSettings                                 m_converterSettings;
    const Core::IGameDatabaseContainer* const m_databaseContainer;
};

}
