/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryArtifact.hpp"

#include <QSortFilterProxyModel>

namespace FreeHeroes::Gui {

class ArtifactQuickFilterModel : public QSortFilterProxyModel {
public:
    ArtifactQuickFilterModel(QObject* parent);

    void setTreasureClassFilter(Core::LibraryArtifact::TreasureClass treasureClass);
    void setOrderCategoryFilter(Core::LibraryArtifact::OrderCategory orderCategory);
    void resetTreasureClassFilter();
    void resetOrderCategoryFilter();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    bool                                 m_treasureClassEnable = false;
    bool                                 m_orderCategoryEnable = false;
    Core::LibraryArtifact::TreasureClass m_treasureClass       = Core::LibraryArtifact::TreasureClass::Special;
    Core::LibraryArtifact::OrderCategory m_orderCategory       = Core::LibraryArtifact::OrderCategory::Special;
};

}
