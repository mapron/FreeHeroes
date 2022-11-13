/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MapGenTestWidget.hpp"

#include "AdventureMap.hpp"
#include "AdventureMapItem.hpp"
#include "TerrainGenerator.hpp"

#include "IGameDatabase.hpp"

#include <QBoxLayout>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include <vector>
#include <set>
#include <compare>

namespace FreeHeroes {

MapGenTestWidget::MapGenTestWidget(Gui::IGraphicsLibrary&      graphicsLibrary,
                                   const Core::IGameDatabase*  gameDatabase,
                                   Gui::LibraryModelsProvider& modelsProvider)
    : m_graphicsLibrary(graphicsLibrary)
    , m_gameDatabase(gameDatabase)
    , m_modelsProvider(modelsProvider)
{
    QVBoxLayout* layout    = new QVBoxLayout(this);
    QHBoxLayout* layoutTop = new QHBoxLayout();
    layout->addLayout(layoutTop);
    QPushButton* testPB = new QPushButton("test", this);
    connect(testPB, &QPushButton::clicked, this, &MapGenTestWidget::generateMap);
    layoutTop->addWidget(testPB);
    layoutTop->addStretch();

    const int width  = 20;
    const int height = 20;

    m_scene = new QGraphicsScene(0, 0, width * 32, height * 32, this);
    m_view  = new QGraphicsView(this);
    m_view->setScene(m_scene);
    layout->addWidget(m_view);

    m_hero = std::make_unique<Core::AdventureArmy>();
    m_hero->hero.reset(m_gameDatabase->heroes()->find("sod.hero.castle.kn000"));

    m_adventureMap = std::make_unique<AdventureMap>(width, height, 1);
    generateMap();

    AdventureMapItem* item = new AdventureMapItem(*m_adventureMap, m_modelsProvider);

    m_scene->addItem(item);
    //

    m_view->setMinimumSize(400, 400);
}

MapGenTestWidget::~MapGenTestWidget()
{
}

void MapGenTestWidget::generateMap()
{
    const int    width  = m_adventureMap->width();
    const int    height = m_adventureMap->height();
    TerrainPlane plane(width, height);
    generateTerrainPlane(plane);

    static const std::vector<std::string>     s_terrains{ "sod.terrain.dirt",
                                                      "sod.terrain.sand",
                                                      "sod.terrain.grass",
                                                      "sod.terrain.snow",
                                                      "sod.terrain.swamp" };
    std::vector<Core::LibraryTerrainConstPtr> terrainPtrs;
    for (auto str : s_terrains)
        terrainPtrs.push_back(m_gameDatabase->terrains()->find(str));

    for (int w = 0; w < width; ++w) {
        for (int h = 0; h < height; ++h) {
            int terrainData = plane.get(w, h);
            if (terrainData < 0)
                terrainData = 0; // @todo:!
            auto terrainPtr = terrainPtrs[terrainData];
            assert(terrainPtr);

            auto& tile     = m_adventureMap->get(w, h, 0);
            tile.m_terrain = terrainPtr;
            if (rand() % 7 == 0)
                tile.m_terrainVariant = 8 + rand();
            else
                tile.m_terrainVariant = rand() % 8;
        }
    }

    AdventureMapHero h;
    h.m_pos       = { 5, 2, 0 };
    h.m_army      = m_hero.get();
    h.m_direction = HeroDirection::BR;

    m_adventureMap->m_heroes.push_back(h);

    m_scene->update();
}

}
