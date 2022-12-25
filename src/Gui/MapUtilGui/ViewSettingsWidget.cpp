/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ViewSettingsWidget.hpp"

#include <QVBoxLayout>
#include <QCheckBox>

namespace FreeHeroes {

CheckBoxEditor::CheckBoxEditor(bool* data, QString title, QWidget* parent)
    : IEditor(parent)
{
    QCheckBox* cb = new QCheckBox(title, this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(cb);
    connect(cb, &QCheckBox::clicked, this, [this, data](bool state) {
        *data = state;
        emit dataChanged();
    });
    connect(this, &IEditor::needUpdateUI, this, [data, cb]() {
        cb->setChecked(*data);
    });
}

ViewSettingsWidget::ViewSettingsWidget(ViewSettings* settings, QWidget* parent)
    : IEditor(parent)
    , m_settings(settings)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    auto addEditor = [layout, this](IEditor* ed) {
        layout->addWidget(ed);
        m_editors.push_back(ed);
    };

    addEditor(new CheckBoxEditor(&settings->m_paintSettings.m_grid, tr("Show grid"), this));
    addEditor(new CheckBoxEditor(&settings->m_paintSettings.m_gridOnTop, tr("Draw grid on top"), this));

    addEditor(new CheckBoxEditor(&settings->m_paintSettings.m_animateObjects, tr("Animate objects"), this));
    addEditor(new CheckBoxEditor(&settings->m_paintSettings.m_animateTerrain, tr("Animate terrain"), this));

    connect(this, &IEditor::needUpdateUI, this, [this]() {
        for (auto* ed : m_editors)
            ed->updateUI();
    });
    for (auto* ed : m_editors)
        connect(ed, &IEditor::dataChanged, this, &IEditor::dataChanged);
}

}
