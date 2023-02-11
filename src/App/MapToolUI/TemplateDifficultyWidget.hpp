/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QFrame>

#include <memory>
#include <map>

namespace Ui {
class TemplateDifficultyWidget;
}
namespace FreeHeroes {
namespace Gui {
class LibraryModelsProvider;
}
struct FHRngUserSettings;

class TemplateDifficultyWidget : public QFrame {
    Q_OBJECT
public:
    TemplateDifficultyWidget(QWidget* parent);
    ~TemplateDifficultyWidget();

    void updateUI();
    void save();

    void setUserSettings(FHRngUserSettings* userSettings) { m_userSettings = userSettings; }

private:
    std::unique_ptr<Ui::TemplateDifficultyWidget> m_ui;

    FHRngUserSettings* m_userSettings = nullptr;
};

}
