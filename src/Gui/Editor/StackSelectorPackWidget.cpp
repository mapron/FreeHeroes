/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "StackSelectorPackWidget.hpp"

#include "ui_StackSelectorPackWidget.h"

#include "AdventureWrappers.hpp"
#include "StackSelectorWidget.hpp"

#include "AdventureSquad.hpp"

#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes::Gui {

using namespace Core;

StackSelectorPackWidget::StackSelectorPackWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::StackSelectorPackWidget>())
{
    Mernel::ProfilerScope scope("StackSelectorPackWidget");
    m_ui->setupUi(this);
    m_stackSelectors << m_ui->stackEditor1
                     << m_ui->stackEditor2
                     << m_ui->stackEditor3
                     << m_ui->stackEditor4
                     << m_ui->stackEditor5
                     << m_ui->stackEditor6
                     << m_ui->stackEditor7;

    m_stackLabels << m_ui->label_3
                  << m_ui->label_4
                  << m_ui->label_9
                  << m_ui->label_10
                  << m_ui->label_11
                  << m_ui->label_12
                  << m_ui->label_13; // @todo: label numbers? omg.
}

void StackSelectorPackWidget::setUnitsModel(QAbstractItemModel* unitRootModel)
{
    for (auto* selector : m_stackSelectors)
        selector->setUnitsModel(unitRootModel);
}

void StackSelectorPackWidget::setSource(GuiAdventureSquad* squad)
{
    m_squad          = squad;
    size_t squadSize = squad->getCount();
    Q_ASSERT(m_stackSelectors.size() >= (int) squadSize);
    for (size_t i = 0; i < squadSize; ++i) {
        m_stackSelectors[i]->setSource(m_squad->getStack(i));
        m_stackSelectors[i]->setVisible(true);
        m_stackLabels[i]->setVisible(true);
    }
    for (int i = (int) squadSize; i < m_stackSelectors.size(); ++i) {
        //m_stackSelectors[i]->setSource(nullptr);
        m_stackSelectors[i]->setVisible(false);
        m_stackLabels[i]->setVisible(false);
    }
}

StackSelectorPackWidget::~StackSelectorPackWidget() = default;

}
