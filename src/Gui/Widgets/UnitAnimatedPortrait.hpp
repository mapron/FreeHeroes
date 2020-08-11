/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "ISprites.hpp"

#include <QWidget>

#include <memory>


namespace FreeHeroes::Gui {

class GUIWIDGETS_EXPORT UnitAnimatedPortrait : public QWidget
{
public:
    UnitAnimatedPortrait(SpritePtr spriteUnit,
                             SpritePtr spriteBk,
                             int count,
                             bool animated,
                             QWidget * parent = nullptr);
    ~UnitAnimatedPortrait();
    void startTimer();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
