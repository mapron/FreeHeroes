/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QDialog>

#include <memory>

class QHBoxLayout;
namespace FreeHeroes::Gui {

class GUIWIDGETS_EXPORT GeneralPopupDialog : public QDialog {
    Q_OBJECT
public:
    struct Item {
        QPixmap bottomPicture;
        QString bottomText;
        bool    addBorder = false;
    };
    using Items = QList<Item>;
    GeneralPopupDialog(QString      description,
                       const Items& items,
                       bool         isModal,
                       bool         hasCancel,
                       QWidget*     parent = nullptr);

    void mouseReleaseEvent(QMouseEvent* ev);

    void closeNonModal();

    static bool confirmRequest(const QString& message, QWidget* parent);
    static void messageBox(const QString& message, QWidget* parent);

private:
    //struct Impl;
    // std::unique_ptr<Impl> m_impl;
    bool         m_isModal = false;
    QHBoxLayout* m_bottomButtons;
};

}
