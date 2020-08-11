/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QWidget>
#include <QLabel>

#include <memory>
#include <functional>


namespace FreeHeroes {
namespace Core {
struct AdventureHero;
}

namespace Gui {
class DarkFrameLabelIcon;
class HoverHelper;
class GuiAdventureHero;
class GUIWIDGETS_EXPORT SkillsGridWidget : public QWidget
{
    Q_OBJECT
public:
    SkillsGridWidget(QWidget* parent = nullptr);
    ~SkillsGridWidget();

    void setParams(const GuiAdventureHero * hero);

    void setHoverLabel(QLabel * hoverLabel);
private:
    QList<DarkFrameLabelIcon*> m_icons;
    QList<QLabel *> m_txts;
    std::unique_ptr<HoverHelper> m_hoverHelper;
};

}
}

