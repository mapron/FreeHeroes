/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QGraphicsItem>

#include <compare>
#include <set>
#include <vector>

namespace FreeHeroes {

namespace Gui {
class LibraryModelsProvider;
}

class AdventureMap;
class AdventureMapItem : public QGraphicsItem {
public:
    AdventureMapItem(AdventureMap& adventureMap, Gui::LibraryModelsProvider& modelsProvider)
        : m_adventureMap(adventureMap)
        , m_modelsProvider(modelsProvider)
    {
    }

    // QGraphicsItem interface
public:
    QRectF boundingRect() const override;
    void   paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    const AdventureMap&         m_adventureMap;
    Gui::LibraryModelsProvider& m_modelsProvider;
    static constexpr qreal      tileWidth      = 32.;
    int                         m_currentDepth = 0;
};

}
