/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include <memory>

namespace FreeHeroes::Gui {

class CasualtiesWidget : public QWidget {
    Q_OBJECT
public:
    CasualtiesWidget(QWidget* parent);
    ~CasualtiesWidget();

    struct LossInfo {
        QPixmap portrait;
        int     count  = 0;
        bool    isDead = false;
    };
    struct Info {
        int             totalHP    = 0;
        int             totalValue = 0;
        QList<LossInfo> units;
    };

    void setResultInfo(const Info& info);

    void paintEvent(QPaintEvent* ev) override;

private:
    struct Impl;
    std::unique_ptr<Impl>  m_impl;
    CasualtiesWidget::Info m_info;
};

}
