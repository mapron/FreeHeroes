/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"

#include <QFrame>

#include <memory>
#include <map>

namespace Ui {
class TemplateSettingsWidget;
}

namespace Mernel {
class PropertyTree;
}

namespace FreeHeroes {
namespace Gui {
class LibraryModelsProvider;
}
struct FHRngUserSettings;
class TemplatePlayerWidget;

class TemplateSettingsWidget : public QFrame {
    Q_OBJECT
public:
    TemplateSettingsWidget(const Gui::LibraryModelsProvider* modelsProvider, QWidget* parent);
    ~TemplateSettingsWidget();

    void updateUI();
    void save();

    void setUserSettings(FHRngUserSettings* userSettings) { m_userSettings = userSettings; }

private:
    std::unique_ptr<Ui::TemplateSettingsWidget> m_ui;

    FHRngUserSettings*                                           m_userSettings   = nullptr;
    const Gui::LibraryModelsProvider*                            m_modelsProvider = nullptr;
    std::map<Core::LibraryPlayerConstPtr, TemplatePlayerWidget*> m_mapping;
};

}
