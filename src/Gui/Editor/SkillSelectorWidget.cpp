/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SkillSelectorWidget.hpp"

#include "LibraryEditorModels.hpp"
#include "LibraryWrappersMetatype.hpp"

#include "Profiler.hpp"

#include <QAbstractItemView>

namespace FreeHeroes::Gui {

using namespace Core;

SkillSelectorWidget::SkillSelectorWidget(QWidget* parent)
    : ResizeableComboBox(parent)
{
}

Core::LibrarySecondarySkillConstPtr SkillSelectorWidget::getSkill() const
{
    auto skill = this->currentData(SkillsModel::SourceObject).value<Core::LibrarySecondarySkillConstPtr>();
    return skill;
}

void SkillSelectorWidget::setSkill(Core::LibrarySecondarySkillConstPtr params)
{
    int currentIndex = 0;
    for (int i = 0, cnt = this->count(); i < cnt; ++i) {
        if (params && this->itemData(i, SkillsModel::SourceObject).value<LibrarySecondarySkillConstPtr>() == params) {
            currentIndex = i;
            break;
        }
    }
    this->setCurrentIndex(currentIndex);
}

void SkillSelectorWidget::setSkillsModel(QAbstractItemModel* rootSkillsModel)
{
    if (m_finalModel)
        m_finalModel->deleteLater();
    m_finalModel = new SkillsComboModel(rootSkillsModel, this);

    this->setModel(m_finalModel);

    this->setIconSize({ 22, 22 });
}

SkillSelectorWidget::~SkillSelectorWidget() = default;

QWidget* SkillSelectDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    if (index.column() == 0) {
        auto* cb = new SkillSelectorWidget(parent);
        cb->setSkillsModel(m_rootSkillsModel);

        connect(cb, QOverload<int>::of(&QComboBox::activated), this, &SkillSelectDelegate::commitAndCloseEditor);

        return cb;
    } else if (index.column() == 1) {
        auto* cb = new ResizeableComboBox(parent);
        cb->addItems(QStringList{ tr("Basic"), tr("Advanced"), tr("Expert") });
        connect(cb, QOverload<int>::of(&QComboBox::activated), this, &SkillSelectDelegate::commitAndCloseEditor);

        return cb;
    }
    return nullptr;
}

void SkillSelectDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (index.column() == 0) {
        auto skill = index.data(SkillsModel::SourceObject).value<LibrarySecondarySkillConstPtr>();
        auto cb    = qobject_cast<SkillSelectorWidget*>(editor);
        cb->setSkill(skill);
        cb->showPopup();
    } else if (index.column() == 1) {
        auto level = index.data(Qt::EditRole).toInt();
        auto cb    = qobject_cast<ResizeableComboBox*>(editor);
        cb->setCurrentIndex(level);
        cb->showPopup();
    }
}

void SkillSelectDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (index.column() == 0) {
        auto cb    = qobject_cast<SkillSelectorWidget*>(editor);
        auto skill = cb->getSkill();
        model->setData(index, QVariant::fromValue(skill), SkillsModel::SourceObject);
    } else if (index.column() == 1) {
        auto cb    = qobject_cast<ResizeableComboBox*>(editor);
        int  level = cb->currentIndex();
        model->setData(index, level, Qt::EditRole);
    }
}

void SkillSelectDelegate::commitAndCloseEditor()
{
    ResizeableComboBox* editor = qobject_cast<ResizeableComboBox*>(sender());

    emit commitData(editor);
    emit closeEditor(editor);
}

}
