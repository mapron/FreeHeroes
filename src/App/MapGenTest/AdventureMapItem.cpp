/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "AdventureMapItem.hpp"
#include "AdventureMap.hpp"

#include "IGraphicsLibrary.hpp"
#include "ISprites.hpp"

#include "LibraryTerrain.hpp"
#include "LibraryWrappers.hpp"
#include "LibraryModels.hpp"

#include <QPainter>

namespace FreeHeroes {

QRectF AdventureMapItem::boundingRect() const
{
    return QRectF{ QPointF{ 0., 0. }, QSizeF(m_adventureMap.width(), m_adventureMap.height()) * tileWidth };
}

void AdventureMapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    // QList<QColor> colors {QColor{82, 56, 8 }, QColor{222, 207, 140}, QColor{ 0, 65, 0}, QColor{ 181, 199, 198 }, QColor{74, 134, 107 }};
    for (int y = 0; y < m_adventureMap.height(); ++y) {
        for (int x = 0; x < m_adventureMap.width(); ++x) {
            const auto& tile       = m_adventureMap.get(x, y, m_currentDepth);
            auto*       terrainGui = m_modelsProvider.terrains()->find(tile.m_terrain);
            auto        pix        = terrainGui->getTile(tile.m_terrainVariant);

            const qreal drawX = x * tileWidth;
            const qreal drawY = y * tileWidth;

            painter->drawPixmap(QPointF{ drawX, drawY }, pix);
        }
    }

    for (auto&& hero : m_adventureMap.m_heroes) {
        const qreal drawX = hero.m_pos.x * tileWidth;
        const qreal drawY = hero.m_pos.y * tileWidth;

        auto& libHero   = hero.m_army.hero.library;
        auto* heroGui   = m_modelsProvider.heroes()->find(libHero);
        auto  advSprite = heroGui->getAdventureSprite();
        auto  seq       = advSprite->getFramesForGroup(2);
        auto  frame     = seq->frames[0];

        painter->drawPixmap(QPointF{ drawX, drawY } + frame.paddingLeftTop, frame.frame);
        //Gui::GuiHero gui({}, m_graphicsLibrary, libHero);
        //libHero->heroClass()->presentationParams.battleSpriteFemale
    }
}

}
