/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QObject>
#include <QList>
#include <QPointer>
#include <QMap>


class QLabel;
class QWidget;
namespace  FreeHeroes::Gui {
class GeneralPopupDialog;
class GUIWIDGETS_EXPORT HoverHelper : public QObject

{
public:
    HoverHelper(QWidget * parent);
    ~HoverHelper();

    void addWidgets(const QList<QWidget*> & watch);
    void addAlias(QWidget* main, QWidget* alias);

    void setHoverLabel(QLabel * label);

    bool eventFilter(QObject *watched, QEvent *event) override;

    bool showPopup(QWidget * what, bool isModal);
    bool hidePopup(QWidget * what);

private:
    QLabel * m_hoverLabel = nullptr;
    QWidget * m_parent = nullptr;
    QPointer<GeneralPopupDialog> m_dialog ;
    QMap<QWidget*, QWidget*> m_aliases;
};

}
