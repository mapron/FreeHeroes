/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include <QFrame>

#include <memory>

namespace Ui {
class TemplateSettingsWidget;
}

namespace FreeHeroes {
namespace Gui {
class LibraryModelsProvider;
}
enum class FHPlayerId;
struct FHRngUserSettings;
class TemplatePlayerWidget;
class TemplateSettingsWidget : public QFrame {
public:
    TemplateSettingsWidget(const Gui::LibraryModelsProvider* modelsProvider, QWidget* parent);
    ~TemplateSettingsWidget();

    void load(const Mernel::std_path& path);
    void updateUI();
    void save();

private:
    std::unique_ptr<Ui::TemplateSettingsWidget> m_ui;
    std::unique_ptr<FHRngUserSettings>          m_userSettings;
    Mernel::std_path                            m_path;
    const Gui::LibraryModelsProvider*           m_modelsProvider = nullptr;
    std::map<FHPlayerId, TemplatePlayerWidget*> m_mapping;
};

}
