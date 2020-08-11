/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QWidget>
#include <QBoxLayout>


class QPushButton;

namespace FreeHeroes::Gui {
class FlatButton;

struct GUIWIDGETS_EXPORT DialogUtils {

static void commonDialogSetup(QDialog * parent);
static QVBoxLayout * makeMainDialogFrame(QWidget * parent, bool thin = false);

static FlatButton * makeAcceptButton(QDialog * parent, FlatButton * alreadyAllocated = nullptr, bool isWide = true);
static FlatButton * makeRejectButton(QDialog * parent, FlatButton * alreadyAllocated = nullptr);
static void setupClickSound(QPushButton * button);

};

}
