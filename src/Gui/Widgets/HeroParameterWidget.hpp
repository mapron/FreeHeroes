/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureFwd.hpp"

#include <QWidget>

#include <memory>

namespace FreeHeroes::Gui {
class LibraryModelsProvider;

class GUIWIDGETS_EXPORT HeroParameterWidget : public QWidget {
    Q_OBJECT
public:
    HeroParameterWidget(QWidget * parent = nullptr);
    ~HeroParameterWidget();

    void refresh();
    void setSource(Core::AdventureHeroConstPtr hero);
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
