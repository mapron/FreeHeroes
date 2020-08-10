/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include <memory>

class QComboBox;
class QSpinBox;
class QAbstractItemModel;

namespace FreeHeroes::Gui {
class UnitsComboModel;
class UnitsFilterModel;
class GuiAdventureStack;
class StackSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    StackSelectorWidget(QWidget * parent = nullptr);
    ~StackSelectorWidget();

public:
    void setUnitsModel(QAbstractItemModel * unitRootModel);
    void setSource(GuiAdventureStack * unit);

private:
    GuiAdventureStack * m_unit = nullptr;
    UnitsComboModel * m_model = nullptr;
    UnitsFilterModel * m_filter = nullptr;

    QComboBox * m_selectId = nullptr;
    QSpinBox * m_count = nullptr;
};

}
