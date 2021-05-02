/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibraryEditorModels.hpp"

#include "ResizePixmap.hpp"

namespace FreeHeroes::Gui {

ZeroElementModel::ZeroElementModel(QString title, QObject* parent)
    : QAbstractListModel(parent)
    , m_title(title)
{
}

QVariant ZeroElementModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole)
        return m_title;

    return QVariant();
}

int ZeroElementModel::rowCount(const QModelIndex&) const
{
    return 1;
}

ComboModel::ComboModel(QString emptyText, QSize maxIconSize, QAbstractItemModel* sourceModel, QObject* parent)
    : QConcatenateTablesProxyModel(parent)
    , m_zeroModel(new ZeroElementModel(emptyText, parent))
    , m_maxIconSize(maxIconSize)
{
    if (!emptyText.isEmpty())
        addSourceModel(m_zeroModel);
    addSourceModel(sourceModel);
}

QVariant ComboModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole) {
        QVariant imgVar = QConcatenateTablesProxyModel::data(index, role);
        if (imgVar.isValid()) {
            QPixmap img = imgVar.value<QPixmap>();
            if (!img.isNull()) {
                return resizePixmap(img, m_maxIconSize, false);
            }
        }
        return imgVar;
    }
    return QConcatenateTablesProxyModel::data(index, role);
}

ArtifactsComboModel::ArtifactsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Select artifact -- "), { 22, 22 }, sourceModel, parent)
{
}

UnitsComboModel::UnitsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Empty -- "), { 22, 22 }, sourceModel, parent)
{
}

HeroesComboModel::HeroesComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel("", { 48, 32 }, sourceModel, parent)
{
}

SkillsComboModel::SkillsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Select skill -- "), { 22, 22 }, sourceModel, parent)
{
}

FactionsComboModel::FactionsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Any faction -- "), {}, sourceModel, parent)
{
}

TerrainsComboModel::TerrainsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel("", { 32, 32 }, sourceModel, parent)
{
}

MapObjectsComboModel::MapObjectsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- No creature bank --"), { 32, 24 }, sourceModel, parent)
{
}

MapObjectsTreeModel::MapObjectsTreeModel(QAbstractItemModel* source, QObject* parent)
    : QAbstractItemModel(parent)
    , m_source(source)
{
}

QVariant MapObjectsTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.parent().isValid()) {
        return m_source->data(m_source->index(index.row(), 0), role);
    }
    if (role == Qt::DisplayRole) {
        auto        parentIndex = m_source->index(index.parent().row(), 0);
        QString     baseName    = m_source->data(parentIndex, Qt::DisplayRole).toString();
        QStringList children    = m_source->data(parentIndex, MapObjectsModel::VaraintNames).toStringList();
        return baseName + " " + children.value(index.row());
    }
    if (role == Qt::DecorationRole || role == MapObjectsModel::SourceObject) {
        auto parentIndex = m_source->index(index.parent().row(), 0);
        return m_source->data(parentIndex, role);
    }
    return QVariant();
}

QModelIndex MapObjectsTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto parentRow = parent.isValid() ? (quintptr) parent.row() : quintptr(-1);

    return createIndex(row, column, parentRow);
}

QModelIndex MapObjectsTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    quintptr parentRow = child.internalId();
    if (parentRow == quintptr(-1))
        return QModelIndex();

    return createIndex((int) parentRow, 0, quintptr(-1));
}

int MapObjectsTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_source->rowCount();
    }
    int children = m_source->data(m_source->index(parent.row(), 0), MapObjectsModel::VaraintNames).toStringList().size();
    return children;
}

int MapObjectsTreeModel::columnCount(const QModelIndex&) const
{
    return 1;
}

Qt::ItemFlags MapObjectsTreeModel::flags(const QModelIndex& index) const
{
    if (index.parent().isValid()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    }
    if (index.row() == 0) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    }
    return Qt::ItemIsEnabled;
}

}
