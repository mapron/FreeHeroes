/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QDialog>

#include <memory>
#include <functional>


namespace FreeHeroes {
namespace Core {
}

namespace Gui {

// @todo: will be handy in adventure mode.right panel with heroes portraits.
class GUIWIDGETS_EXPORT HeroSelectorDialogWidget : public QWidget
{
    Q_OBJECT
public:
    HeroSelectorDialogWidget(QWidget* parent = nullptr);
    ~HeroSelectorDialogWidget();
};

}
}

