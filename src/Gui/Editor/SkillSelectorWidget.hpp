/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "CustomFrames.hpp"

#include <memory>

#include <QStyledItemDelegate>

class QComboBox;
class QSpinBox;

namespace FreeHeroes::Core {
struct LibrarySecondarySkill;
using LibrarySecondarySkillConstPtr = const LibrarySecondarySkill*;
}

namespace FreeHeroes::Gui {

class SkillsComboModel;
class SkillSelectorWidget : public ResizeableComboBox
{
    Q_OBJECT
public:
    SkillSelectorWidget(QWidget * parent = nullptr);
    ~SkillSelectorWidget();

    Core::LibrarySecondarySkillConstPtr getSkill() const;
    void setSkill(Core::LibrarySecondarySkillConstPtr skill);

    void setSkillsModel(QAbstractItemModel * rootSkillsModel);

private:
    SkillsComboModel * m_finalModel = nullptr;
};

class SkillSelectDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    QAbstractItemModel * m_rootSkillsModel = nullptr;
public:
    SkillSelectDelegate(QAbstractItemModel * rootSkillsModel, QObject * parent)
        : QStyledItemDelegate(parent), m_rootSkillsModel(rootSkillsModel) {}

    // QAbstractItemDelegate interface
public:
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& , const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    void commitAndCloseEditor();
};

}
