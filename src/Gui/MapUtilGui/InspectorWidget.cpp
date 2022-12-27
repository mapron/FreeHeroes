/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "InspectorWidget.hpp"

#include "SpriteMap.hpp"

#include <QLabel>
#include <QLineEdit>

namespace FreeHeroes {

namespace {

void cleanupLayout(QLayout* layout)
{
    while (auto item = layout->takeAt(0)) {
        if (auto inner = item->layout())
            cleanupLayout(inner);
        if (auto widget = item->widget())
            delete widget;
        delete item;
    }
}
}

InspectorWidget::InspectorWidget(const SpriteMap* spriteMap, QWidget* parent)
    : QWidget(parent)
    , m_spriteMap(spriteMap)
{
    m_layout = new QVBoxLayout(this);
}

InspectorWidget::~InspectorWidget()
{
}

void InspectorWidget::clear()
{
    cleanupLayout(m_layout);
}

void InspectorWidget::displayInfo(int x, int y, int depth)
{
    cleanupLayout(m_layout);

    auto addHeader = [this](const QString& name) {
        m_layout->addSpacing(10);
        auto* l = new QLabel(name, this);
        auto  f = l->font();
        f.setPointSize(f.pointSize() + 1);

        l->setFont(f);
        m_layout->addWidget(l);
    };
    auto addKeyValue = [this](const QString& name, const QString& value) {
        QHBoxLayout* rowLayout = new QHBoxLayout();
        auto*        l         = new QLabel(name, this);
        auto*        le        = new QLineEdit(this);

        l->setMinimumWidth(35);
        rowLayout->addWidget(l);
        rowLayout->addWidget(le);
        le->setReadOnly(true);
        le->setText(value);
        m_layout->addLayout(rowLayout);
    };

    addKeyValue(tr("Pos"), QString("(%1, %2, %3)").arg(x).arg(y).arg(depth));

    auto allObjectCells = m_spriteMap->findCells(x, y, depth);

    for (const auto* cell : allObjectCells) {
        for (const auto& item : cell->m_items) {
            addHeader(SpriteMap::layerTypeToString(item.m_layer));
            for (const auto& [key, val] : item.m_info)
                addKeyValue(QString::fromStdString(key), QString::fromStdString(val));
        }
    }

    m_layout->addStretch();
}

}
