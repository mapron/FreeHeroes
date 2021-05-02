/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include <QLabel>
#include <QVariant>
#include <QPushButton>
#include <QComboBox>

class QTreeView;
namespace FreeHeroes::Gui {
class DarkFrameLabel : public QLabel {
public:
    DarkFrameLabel(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        this->setFrameStyle(QFrame::Panel);
        this->setLineWidth(2);
        this->setProperty("borderStyle", "commonDark");
        this->setProperty("fill", true);
        setAttribute(Qt::WA_Hover);
    }
    DarkFrameLabel(const QString& txt, QWidget* parent = nullptr)
        : DarkFrameLabel(parent)
    {
        setText(txt);
    }
};

class DarkFrameLabelIcon : public DarkFrameLabel {
protected:
    //std::string m_resourceBase;
public:
    DarkFrameLabelIcon(QWidget* parent)
        : DarkFrameLabel(parent)
    {}
    void setPixmap(const QPixmap& pixmap)
    {
        QLabel::setPixmap(pixmap);
        if (!pixmap.size().isNull())
            setFixedSize(pixmap.size() + QSize{ this->lineWidth() * 2, this->lineWidth() * 2 });
    }
};

class FlatLabelIcon : public DarkFrameLabelIcon {
public:
    FlatLabelIcon(QWidget* parent = nullptr)
        : DarkFrameLabelIcon(parent)
    {
        this->setFrameStyle(QFrame::NoFrame);
        this->setLineWidth(0);
        this->setProperty("borderStyle", "");
        this->setProperty("fill", false);
    }
    //FlatLabelIcon(QWidget * parent) : DarkFrameLabelIcon(parent) { }
};

class DarkFrame : public QFrame {
public:
    DarkFrame(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        this->setFrameStyle(QFrame::Panel);
        this->setLineWidth(2);
        this->setProperty("borderStyle", "commonDark");
        this->setProperty("fill", true);
    }
};

class GUIWIDGETS_EXPORT FlatButton : public QPushButton {
    Q_OBJECT
public:
    FlatButton(QWidget* parent = nullptr);

    void enableHoverTracking(bool state);
    void setFocusActiveColor(QColor color);
    void setBorderWidth(int borderWidth, bool adjustSize = false);
    void setAdditionalCheckedHighlight(bool state);

signals:
    void doubleClicked();

protected:
    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    bool event(QEvent* ev) override;

protected:
    bool   m_isPressed                  = false;
    bool   m_isHovered                  = false;
    bool   m_hoverEnabled               = true;
    bool   m_additionalCheckedHighlight = false;
    QColor m_focusActiveColor;
    int    m_borderWidth = 0;

private:
    void setIconSize(const QSize&); // you should not call this;
};

class GUIWIDGETS_EXPORT ResizeableComboBox : public QComboBox {
    Q_OBJECT
public:
    ResizeableComboBox(QWidget* parent = nullptr);

    void showPopup() override;

    int calculateMaxModelWidth() const;
};

class GUIWIDGETS_EXPORT TreeComboBox : public ResizeableComboBox {
    Q_OBJECT
public:
    TreeComboBox(QWidget* parent = nullptr);

    void expandAll();
    void selectIndex(const QModelIndex& index);

    void showPopup() override;
    void hidePopup() override;

private:
    QTreeView* m_view = nullptr;
};

}
