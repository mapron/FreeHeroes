/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once
#include "GuiBattleExport.hpp"

#include "CasualtiesWidget.hpp"

#include <QDialog>

#include <memory>

namespace FreeHeroes::Gui {
class LibraryModelsProvider;
class GUIBATTLE_EXPORT BattleResultDialog : public QDialog
{
    Q_OBJECT
public:
    BattleResultDialog(LibraryModelsProvider & modelsProvider, QWidget * parent);

    struct SideInfo {
        QString name;
        QPixmap portrait;
        bool win = false;
        CasualtiesWidget::Info loss;
    };

    struct ResultInfo {
        SideInfo sides[2];
        QString resultDescription;
        bool goodResult = true;
    };

    void setResultInfo(const ResultInfo & info);
    ~BattleResultDialog();

    void showEvent(QShowEvent * ev) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}


