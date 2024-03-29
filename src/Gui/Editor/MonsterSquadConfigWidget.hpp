/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiEditorExport.hpp"

#include <QWidget>

#include <memory>
#include <functional>

namespace Ui {
class MonsterSquadConfigWidget;
}

namespace FreeHeroes {

namespace Gui {
class LibraryModelsProvider;
class GuiAdventureArmy;

class GUIEDITOR_EXPORT MonsterSquadConfigWidget : public QWidget {
    Q_OBJECT
public:
    MonsterSquadConfigWidget(const LibraryModelsProvider* modelProvider, QWidget* parent = nullptr);
    ~MonsterSquadConfigWidget();

public:
    void setModels();
    void setSource(GuiAdventureArmy* adventureArmy);

private:
    std::unique_ptr<Ui::MonsterSquadConfigWidget> m_ui;
    const LibraryModelsProvider* const            m_modelProvider;
};

}
}
