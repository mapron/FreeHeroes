/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IAppSettings.hpp"

#include <QDialog>

namespace Ui {
class SettingsWidget;
}

namespace FreeHeroes::Gui {

class SettingsWidget : public QDialog {
public:
    SettingsWidget(QSettings& uiSettings, IAppSettings::AllSettings& settings, QWidget* parent);
    ~SettingsWidget();

    void accept() override;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent* event) override;

private:
    std::unique_ptr<Ui::SettingsWidget> m_ui;
    class WidgetWrapRef;
    std::vector<WidgetWrapRef> m_allRefs;
};

}
