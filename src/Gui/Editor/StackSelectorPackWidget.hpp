/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include <memory>
#include <vector>

class QLabel;
class QAbstractItemModel;

namespace Ui
{
class StackSelectorPackWidget;
}

namespace FreeHeroes::Gui {
class StackSelectorWidget;
class GuiAdventureSquad;
class StackSelectorPackWidget : public QWidget
{
    Q_OBJECT
public:
    StackSelectorPackWidget(QWidget * parent = nullptr);
    ~StackSelectorPackWidget();

public:

    void setUnitsModel(QAbstractItemModel * unitRootModel);
    void setSource(GuiAdventureSquad * squad);

private:
    std::unique_ptr<Ui::StackSelectorPackWidget> m_ui;
    QList<StackSelectorWidget*> m_stackSelectors;
    QList<QLabel*> m_stackLabels;
    GuiAdventureSquad * m_squad = nullptr;
};

}
