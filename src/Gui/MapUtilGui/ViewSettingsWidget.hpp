/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QWidget>

#include "ViewSettings.hpp"

namespace FreeHeroes {

class IEditor : public QWidget {
    Q_OBJECT
public:
    IEditor(QWidget* parent)
        : QWidget(parent)
    {}

    void updateUI() { emit needUpdateUI(); }

signals:
    void dataChanged();
    void needUpdateUI();
};

class CheckBoxEditor : public IEditor {
public:
    CheckBoxEditor(bool* data, QString title, QWidget* parent);
};

class ViewSettingsWidget : public IEditor {
    Q_OBJECT

public:
    ViewSettingsWidget(ViewSettings* settings, QWidget* parent);

private:
    ViewSettings* const   m_settings;
    std::vector<IEditor*> m_editors;
};

}
