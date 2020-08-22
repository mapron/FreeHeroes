/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GeneralPopupDialog.hpp"

#include "FormatUtils.hpp"
#include "DialogUtils.hpp"
#include "CustomFrames.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QVariant>

namespace FreeHeroes::Gui {

GeneralPopupDialog::GeneralPopupDialog(QString description,
                                       const Items & items,
                                       bool isModal,
                                       bool hasCancel,
                                       QWidget* parent)
    : QDialog(parent), m_isModal(isModal)
{
    setWindowFlag(Qt::Popup, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    DialogUtils::commonDialogSetup(this);

    QVBoxLayout * mainLayout = DialogUtils::makeMainDialogFrame(this);

    description = FormatUtils::prepareDescription(description);
    {
        QHBoxLayout * labelWrap = new QHBoxLayout();
        QLabel * mainLabel = new QLabel(description, this);
        mainLabel->setAlignment(Qt::AlignHCenter);
        mainLabel->setWordWrap(true);
        mainLabel->setMaximumWidth(300);
        QFont f = mainLabel->font();
        f.setPointSize(f.pointSize() + 2);
        mainLabel->setFont(f);
        mainLayout->addLayout(labelWrap);
        labelWrap->addStretch(0);
        labelWrap->addWidget(mainLabel, 1);
        labelWrap->addStretch(0);
    }

    if (!items.isEmpty()) {
        mainLayout->addSpacing(20);
        QHBoxLayout * imgWrap = new QHBoxLayout();
        imgWrap->setMargin(0);
        imgWrap->setSpacing(20);
        mainLayout->addLayout(imgWrap);
        imgWrap->addStretch(0);
        imgWrap->addSpacing(100);
        for (const auto & item : items) {
            QVBoxLayout * itemLayout = new QVBoxLayout();
            imgWrap->addLayout(itemLayout);
            QHBoxLayout * imgLayout = new QHBoxLayout();
            itemLayout->addLayout(imgLayout);

            DarkFrameLabelIcon * imgLabel = item.addBorder ?  new DarkFrameLabelIcon(this) : new FlatLabelIcon(this);
            imgLabel->setProperty("fill", false);
            imgLabel->setFixedSize(item.bottomPicture.size());
            imgLabel->setPixmap(item.bottomPicture);
            imgLayout->addStretch();
            imgLayout->addWidget(imgLabel);
            imgLayout->addStretch();

            if (item.bottomText.isEmpty())
                continue;

            QLabel * bottomLabel = new QLabel(item.bottomText, this);
            bottomLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
            bottomLabel->setWordWrap(true);
            bottomLabel->setMaximumWidth(item.bottomPicture.size().width() * 2);
            itemLayout->addWidget(bottomLabel);
        }
        imgWrap->addSpacing(100);
        imgWrap->addStretch(0);
    }
    if (isModal)
    {
        QHBoxLayout * bottomButtons = new QHBoxLayout();
        mainLayout->addLayout(bottomButtons);
        bottomButtons->addStretch();
        if (hasCancel) {
            bottomButtons->addWidget(DialogUtils::makeRejectButton(this));
            bottomButtons->addSpacing(10);
        }
        bottomButtons->addWidget(DialogUtils::makeAcceptButton(this));
        bottomButtons->addStretch();
    }

}

void GeneralPopupDialog::mouseReleaseEvent(QMouseEvent* ev)
{
    QDialog::mouseReleaseEvent(ev);
    if (!m_isModal) {
        close();
        deleteLater();
    }
}

void GeneralPopupDialog::closeNonModal()
{
    if (!m_isModal) {
        close();
        deleteLater();
    }
}

bool GeneralPopupDialog::confirmRequest(const QString& message, QWidget * parent)
{
    QDialog dlg(parent);
    dlg.setWindowFlag(Qt::Popup, true);
    DialogUtils::commonDialogSetup(&dlg);

    QVBoxLayout * mainLayout = DialogUtils::makeMainDialogFrame(&dlg);
   // mainLayout->addWidget(new QLabel("adadasdasd", &dlg));
    mainLayout->setContentsMargins(5,10,5,5);

    auto msg  = FormatUtils::prepareDescription(message);
    {
        QHBoxLayout * labelWrap = new QHBoxLayout();
        QLabel * mainLabel = new QLabel(msg, &dlg);
        mainLabel->setAlignment(Qt::AlignHCenter);
        mainLabel->setWordWrap(true);
        mainLabel->setMaximumWidth(300);

        QFont f = mainLabel->font();
        f.setPointSize(f.pointSize() + 2);
        mainLabel->setFont(f);
        mainLayout->addLayout(labelWrap);
        labelWrap->addStretch(0);
        labelWrap->addWidget(mainLabel, 1);
        labelWrap->addStretch(0);
    }
    mainLayout->addSpacing(10);
    QHBoxLayout * bottomButtons = new QHBoxLayout();
    mainLayout->addLayout(bottomButtons);
    bottomButtons->addStretch();
    bottomButtons->addSpacing(50);
    bottomButtons->addWidget(DialogUtils::makeRejectButton(&dlg));
    bottomButtons->addSpacing(10);

    bottomButtons->addWidget(DialogUtils::makeAcceptButton(&dlg));
    bottomButtons->addSpacing(50);
    bottomButtons->addStretch();
    dlg.resize(dlg.sizeHint());
    int result = dlg.exec();
    return result == QDialog::Accepted;
}

void GeneralPopupDialog::messageBox(const QString& message, QWidget* parent)
{
    GeneralPopupDialog dlg(message, {}, true, false, parent);
    dlg.exec();
}


}
