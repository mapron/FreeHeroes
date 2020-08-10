/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "CustomFrames.hpp"

#include "Stat.hpp"


namespace FreeHeroes::Gui {
class UiCommonModel;
class GUIWIDGETS_EXPORT RngLabel : public DarkFrameLabelIcon {
protected:
    int m_value = 0;
    bool m_isLuck = false;
    UiCommonModel & m_ui;
    bool m_displayText = false;
    bool m_clampText = false;
public:

    RngLabel(QWidget * parent = nullptr);
    ~RngLabel();

    void setValue(int bonusValue);
};

class LuckLabel : public RngLabel {
    Q_OBJECT
public:

    LuckLabel(QWidget * parent = nullptr);

    void setDetails(const Core::LuckDetails & details);
};


class MoraleLabel : public RngLabel {
    Q_OBJECT
public:

    MoraleLabel(QWidget * parent = nullptr);

    void setDetails(const Core::MoraleDetails & details);

};

}
