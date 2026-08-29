/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ArtifactQuickFilterModel.hpp"

#include "LibraryModels.hpp"
#include "LibraryWrappersMetatype.hpp"

namespace FreeHeroes::Gui {

ArtifactQuickFilterModel::ArtifactQuickFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void ArtifactQuickFilterModel::setTreasureClassFilter(Core::LibraryArtifact::TreasureClass treasureClass)
{
    beginFilterChange();
    m_treasureClassEnable = true;
    m_treasureClass       = treasureClass;
    endFilterChange();
}

void ArtifactQuickFilterModel::setOrderCategoryFilter(Core::LibraryArtifact::OrderCategory orderCategory)
{
    beginFilterChange();
    m_orderCategoryEnable = true;
    m_orderCategory       = orderCategory;
    endFilterChange();
}

void ArtifactQuickFilterModel::resetTreasureClassFilter()
{
    beginFilterChange();
    m_treasureClassEnable = false;
    endFilterChange();
}

void ArtifactQuickFilterModel::resetOrderCategoryFilter()
{
    beginFilterChange();
    m_orderCategoryEnable = false;
    endFilterChange();
}

bool ArtifactQuickFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    auto        rec    = index0.data(ArtifactsModel::SourceObject).value<ArtifactsModel::SrcTypePtr>();
    if (!rec)
        return true; // we don't care for "zero element" stuff.

    if (m_treasureClassEnable && rec->treasureClass != m_treasureClass)
        return false;

    if (m_orderCategoryEnable && rec->presentationParams.orderCategory != m_orderCategory)
        return false;

    return true;
}

}
