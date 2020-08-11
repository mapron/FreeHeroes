/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibraryEditorModels.hpp"

#include "ResizePixmap.hpp"


namespace FreeHeroes::Gui {

ZeroElementModel::ZeroElementModel(QString title, QObject* parent)
    : QAbstractListModel(parent), m_title(title)
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

int ZeroElementModel::rowCount(const QModelIndex& ) const
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

ArtifactsComboModel::ArtifactsComboModel(QAbstractItemModel * sourceModel, QObject* parent)
    : ComboModel(tr("-- Select artifact -- "), {22, 22},  sourceModel, parent)
{
}

UnitsComboModel::UnitsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Empty -- "), {22, 22}, sourceModel, parent)
{

}

HeroesComboModel::HeroesComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel("", {48, 32}, sourceModel, parent)
{
}

SkillsComboModel::SkillsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Select skill -- "), {22, 22},  sourceModel, parent)
{

}

FactionsComboModel::FactionsComboModel(QAbstractItemModel* sourceModel, QObject* parent)
    : ComboModel(tr("-- Any faction -- "), {},  sourceModel, parent)
{

}


}
