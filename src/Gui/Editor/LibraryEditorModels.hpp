/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiEditorExport.hpp"

#include "LibraryModels.hpp"


namespace FreeHeroes::Core {
struct AdventureStack;
struct AdventureSquad;
}

namespace FreeHeroes::Gui {

class GUIEDITOR_EXPORT ZeroElementModel : public QAbstractListModel
{
public:
    ZeroElementModel(QString title, QObject * parent);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
private:
    const QString m_title;
};

class GUIEDITOR_EXPORT ComboModel : public QConcatenateTablesProxyModel
{
public:
    ComboModel(QString emptyText, QSize maxIconSize, QAbstractItemModel * sourceModel, QObject * parent);
    QVariant data(const QModelIndex &index, int role) const override;
private:
    ZeroElementModel * m_zeroModel;
    const QSize m_maxIconSize;
};

class GUIEDITOR_EXPORT ArtifactsComboModel : public ComboModel
{
    Q_OBJECT
public:
    ArtifactsComboModel(QAbstractItemModel * sourceModel, QObject * parent);
};

class GUIEDITOR_EXPORT UnitsComboModel : public ComboModel
{
    Q_OBJECT
public:
    UnitsComboModel(QAbstractItemModel * sourceModel, QObject * parent);
};

class GUIEDITOR_EXPORT HeroesComboModel : public ComboModel
{
    Q_OBJECT
public:
    HeroesComboModel(QAbstractItemModel * sourceModel, QObject * parent);
};

class GUIEDITOR_EXPORT SkillsComboModel : public ComboModel
{
    Q_OBJECT
public:
    SkillsComboModel(QAbstractItemModel * sourceModel, QObject * parent);
};

class GUIEDITOR_EXPORT FactionsComboModel : public ComboModel
{
    Q_OBJECT
public:
    FactionsComboModel(QAbstractItemModel * sourceModel, QObject * parent);
};


}
