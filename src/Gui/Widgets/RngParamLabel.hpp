/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "CustomFrames.hpp"

#include "Stat.hpp"

namespace FreeHeroes::Gui {
class LibraryModelsProvider;
class UiCommonModel;
class GUIWIDGETS_EXPORT RngLabel : public DarkFrameLabelIcon {
protected:
    int            m_value  = 0;
    bool           m_isLuck = false;
    UiCommonModel& m_ui;
    bool           m_displayText = false;
    bool           m_clampText   = false;

public:
    RngLabel(const LibraryModelsProvider* modelProvider, QWidget* parent = nullptr);
    ~RngLabel();

    void setValue(int bonusValue);
};

class LuckLabel : public RngLabel {
    Q_OBJECT
public:
    LuckLabel(const LibraryModelsProvider* modelProvider, QWidget* parent = nullptr);

    void setDetails(const Core::LuckDetails& details);
};

class MoraleLabel : public RngLabel {
    Q_OBJECT
public:
    MoraleLabel(const LibraryModelsProvider* modelProvider, QWidget* parent = nullptr);

    void setDetails(const Core::MoraleDetails& details);
};

}
