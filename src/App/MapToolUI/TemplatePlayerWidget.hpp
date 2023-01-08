/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHMap.hpp"

#include <QFrame>

#include <memory>

namespace Ui {
class TemplatePlayerWidget;
}

namespace FreeHeroes {
namespace Gui {
class LibraryModelsProvider;
}
class TemplatePlayerWidget : public QFrame {
public:
    TemplatePlayerWidget(const Gui::LibraryModelsProvider* modelsProvider, QWidget* parent);
    ~TemplatePlayerWidget();

    void setConfig(const FHRngUserSettings::UserPlayer& settings);

    FHRngUserSettings::UserPlayer getConfig() const;

    void setPlayerColorText(QString txt);

private:
    std::unique_ptr<Ui::TemplatePlayerWidget> m_ui;
};

}
