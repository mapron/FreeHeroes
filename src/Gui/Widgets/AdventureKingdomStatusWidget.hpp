/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureKingdom.hpp"

#include <QWidget>

#include <memory>


namespace FreeHeroes::Gui {

class GUIWIDGETS_EXPORT AdventureKingdomStatusWidget : public QWidget
{
    Q_OBJECT
public:
    AdventureKingdomStatusWidget(QWidget * parent);
    ~AdventureKingdomStatusWidget();

    void setResourceIcons(const QMap<QString, QPixmap> & icons);
    void update(const Core::AdventureKingdom & kingdom);

private:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
