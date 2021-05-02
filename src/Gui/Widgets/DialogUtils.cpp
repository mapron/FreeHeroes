/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "DialogUtils.hpp"

#include "DependencyInjector.hpp"
#include "CustomFrames.hpp"
#include "UiCommonModel.hpp"

#include <QFrame>
#include <QVariant>
#include <QPushButton>
#include <QDialog>
#include <QGuiApplication>
#include <QScreen>

namespace FreeHeroes::Gui {

QVBoxLayout* DialogUtils::makeMainDialogFrame(QWidget* parent, bool thin)
{
    QFrame*      outerFrame       = new QFrame(parent);
    QVBoxLayout* outerLayoutProxy = new QVBoxLayout(parent);
    outerLayoutProxy->setMargin(0);
    outerLayoutProxy->setSpacing(0);
    outerLayoutProxy->addWidget(outerFrame);
    outerFrame->setFrameStyle(QFrame::Box);
    outerFrame->setLineWidth(5);
    outerFrame->setProperty("borderStyle", "main");

    QVBoxLayout* mainLayoutProxy = new QVBoxLayout(outerFrame);
    mainLayoutProxy->setMargin(0);
    mainLayoutProxy->setSpacing(0);
    QFrame* innerFrame = new QFrame(parent);
    innerFrame->setFrameStyle(QFrame::Box);
    if (thin) {
        innerFrame->setProperty("borderStyle", "common");
        innerFrame->setLineWidth(3);
    } else {
        innerFrame->setProperty("borderStyle", "wide");
        innerFrame->setLineWidth(6);
        innerFrame->setProperty("leftBottomFix", 3);
        innerFrame->setProperty("leftTopFix", 3);
        innerFrame->setProperty("specialColor", QColor(230, 0, 0, 128)); // @todo: specalColors.
    }

    mainLayoutProxy->addWidget(innerFrame);

    QVBoxLayout* mainLayout = new QVBoxLayout(innerFrame);
    mainLayout->setContentsMargins(20, 20, 20, 10);
    if (thin)
        mainLayout->setContentsMargins(0, 0, 0, 0);

    return mainLayout;
}

void DialogUtils::commonDialogSetup(QDialog* parent)
{
    parent->setWindowFlag(Qt::FramelessWindowHint, true);
}

FlatButton* DialogUtils::makeAcceptButton(QDialog* parent, FlatButton* alreadyAllocated, bool isWide)
{
    auto& modelProvider = loadDependency<LibraryModelsProvider>(parent);
    auto& buttons       = modelProvider.ui()->buttons;

    FlatButton* btn = alreadyAllocated ? alreadyAllocated : new FlatButton(parent);
    setupClickSound(btn);
    QObject::connect(btn, &QPushButton::clicked, parent, &QDialog::accept);

    const QSize size = isWide ? QSize(64, 30) : QSize(52, 36);

    btn->setIcon(isWide ? buttons.okWide->get() : buttons.close->get());
    btn->setFixedSize(size);
    btn->enableHoverTracking(true);
    btn->setProperty("hoverName", modelProvider.ui()->getCommonString(UiCommonModel::UIString::Close));

    return btn;
}

FlatButton* DialogUtils::makeRejectButton(QDialog* parent, FlatButton* alreadyAllocated)
{
    auto& modelProvider = loadDependency<LibraryModelsProvider>(parent);
    auto& buttons       = modelProvider.ui()->buttons;

    const QSize size(64, 30);
    FlatButton* btn = alreadyAllocated ? alreadyAllocated : new FlatButton(parent);
    setupClickSound(btn);
    QObject::connect(btn, &QPushButton::clicked, parent, &QDialog::reject);
    QIcon icn = buttons.cancel->get();

    btn->setIcon(icn);
    btn->setFixedSize(size);
    btn->enableHoverTracking(true);
    btn->setProperty("hoverName", modelProvider.ui()->getCommonString(UiCommonModel::UIString::Cancel));

    return btn;
}

void DialogUtils::setupClickSound(QPushButton* button)
{
    auto& modelProvider = loadDependency<LibraryModelsProvider>(button);
    QObject::connect(button, &QPushButton::clicked, button, [&modelProvider] {
        modelProvider.ui()->clickEffect->play();
    });
}

void DialogUtils::moveWidgetWithinVisible(QWidget* dialog, QPoint globalPos)
{
    const QSize s             = dialog->sizeHint();
    const QRect availableRect = QGuiApplication::screenAt(globalPos)->availableGeometry();
    QRect       popupRect(globalPos, globalPos + QPoint(s.width(), s.height()));
    if (popupRect.bottom() > availableRect.bottom())
        popupRect.translate(0, availableRect.bottom() - popupRect.bottom());
    if (popupRect.right() > availableRect.right())
        popupRect.translate(availableRect.right() - popupRect.right(), 0);

    dialog->move(popupRect.x(), popupRect.y());
}

}
