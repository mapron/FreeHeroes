/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "StackSelectorWidget.hpp"

#include "SignalsBlocker.hpp"
#include "ResizePixmap.hpp"
#include "CustomFrames.hpp"
#include "LibraryEditorModels.hpp"
#include "LibraryWrappersMetatype.hpp"
#include "AdventureWrappers.hpp"

#include "AdventureStack.hpp"

#include "Profiler.hpp"

#include <QLayout>
#include <QComboBox>
#include <QSpinBox>

namespace FreeHeroes::Gui {

using namespace Core;

StackSelectorWidget::StackSelectorWidget(QWidget* parent)
    : QWidget(parent)
{
    m_selectId = new ResizeableComboBox(this);

    m_filter = new UnitsFilterModel(parent);

    m_model = new UnitsComboModel(m_filter, m_selectId);

    m_selectId->setIconSize({22,22});

    m_count = new QSpinBox(this);
    m_count->setRange(0, 65535);

    QHBoxLayout * layout = new QHBoxLayout(this);
    layout->setSpacing(10);
    layout->setMargin(0);
    layout->addWidget(m_selectId);
    layout->addWidget(m_count);
    connect(m_selectId, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](){
        int cur = m_selectId->currentIndex();

        SignalBlocker lock({m_count});
        if (cur == 0)
            m_count->setValue(0);
        else if (m_count->value() == 0)
            m_count->setValue(1);

        auto library = m_selectId->currentData(UnitsModel::SourceObject).value<LibraryUnitConstPtr>();
        if (!m_unit)
            return;

        m_unit->setUnit(library);
    });
    connect(m_count, qOverload<int>(&QSpinBox::valueChanged), this, [this](){
        if (!m_unit)
            return;

        m_unit->setCount(m_count->value());
    });

}

void StackSelectorWidget::setUnitsModel(QAbstractItemModel * unitRootModel)
{
    m_filter->setSourceModel(unitRootModel);
    m_selectId->setModel(m_model);
}

void StackSelectorWidget::setSource(GuiAdventureStack * unit)
{
    m_unit = unit;
    auto update = [this]{

        SignalBlocker lock({m_selectId, m_count});
        int currentIndex = 0;
        auto guiUnit = m_unit->getGuiUnit();
        for (int i = 0, cnt = m_selectId->count(); i < cnt; ++i) {

            // @todo: we can make map here.
            if (m_selectId->itemData(i, UnitsModel::GuiObject).value<GuiUnitConstPtr>() == guiUnit) {
                currentIndex = i;
                break;
            }
        }
        m_selectId->setCurrentIndex(currentIndex);
        m_count->setValue(m_unit->getSource()->count);
    };
    if (m_unit) {
        connect(m_unit, &GuiAdventureStack::dataChanged, this, update);
        update();
    } else {
        SignalBlocker lock({m_selectId, m_count});
        m_selectId->setCurrentIndex(0);
        m_count->setValue(0);
    }
}

StackSelectorWidget::~StackSelectorWidget() = default;

}
