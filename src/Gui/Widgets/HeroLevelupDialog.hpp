/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "IGuiResource.hpp"

#include <QDialog>

#include <memory>

namespace FreeHeroes::Gui {

class GUIWIDGETS_EXPORT HeroLevelupDialog : public QDialog
{
    Q_OBJECT
public:
    HeroLevelupDialog(QWidget * parent);
    ~HeroLevelupDialog();

    struct Choice {
        QString skillName;
        QString skillLevelName;
        QPixmap icon;

    };

    struct LevelUpDecision {
        QString heroHame;
        QPixmap heroPortrait;
        QString heroClass;
        QPixmap primaryStatIcon;
        QString primaryStatName;
        QPixmap expIcon;
        int level = 0;
        QList<Choice> choices;
    };

    void setInfo(const LevelUpDecision & info);

    void accept() override;
    void reject() override;
    int getChoiceIndex() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}


