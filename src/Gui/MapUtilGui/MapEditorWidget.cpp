/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MapEditorWidget.hpp"

#include "FHMap.hpp"
#include "MapConverter.hpp"

#include "SpriteMap.hpp"
#include "SpriteMapItem.hpp"
#include "FHMapToSpriteMap.hpp"

#include "IRandomGenerator.hpp"
#include "IGameDatabase.hpp"

#include "TickTimer.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include <vector>
#include <set>
#include <compare>

namespace FreeHeroes {

namespace {
const uint32_t g_mapAnimationInterval = 125;
}

MapEditorWidget::MapEditorWidget(const Core::IGameDatabaseContainer*  gameDatabaseContainer,
                                 const Core::IRandomGeneratorFactory* rngFactory,
                                 const Gui::IGraphicsLibrary*         graphicsLibrary,
                                 const Gui::LibraryModelsProvider*    modelsProvider)
    : QDialog(nullptr)
    , m_gameDatabaseContainer(gameDatabaseContainer)
    , m_rngFactory(rngFactory)
    , m_graphicsLibrary(graphicsLibrary)
    , m_modelsProvider(modelsProvider)
    , m_map(std::make_unique<FHMap>())
{
    QVBoxLayout* layout    = new QVBoxLayout(this);
    QHBoxLayout* layoutTop = new QHBoxLayout();
    layout->addLayout(layoutTop);
    {
        QCheckBox* grid        = new QCheckBox("Show grid", this);
        QCheckBox* animateTerr = new QCheckBox("Animate terrain", this);
        QCheckBox* animateObj  = new QCheckBox("Animate objects", this);
        connect(grid, &QCheckBox::toggled, this, [this](bool state) {
            m_paintSettings.m_grid = state;
            updateAll();
        });
        connect(animateTerr, &QCheckBox::toggled, this, [this](bool state) {
            m_paintSettings.m_animateTerrain = state;
            updateAll();
        });
        connect(animateObj, &QCheckBox::toggled, this, [this](bool state) {
            m_paintSettings.m_animateObjects = state;
            updateAll();
        });
        layoutTop->addWidget(grid);
        layoutTop->addWidget(animateTerr);
        layoutTop->addWidget(animateObj);
    }
    layoutTop->addStretch();

    m_scene = new QGraphicsScene(this);
    m_view  = new QGraphicsView(this);
    m_view->setScene(m_scene);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_view->setBackgroundBrush(QColor(Qt::black));
    m_view->setDragMode(QGraphicsView::DragMode::ScrollHandDrag);

    layout->addWidget(m_view);

    m_view->setMinimumSize(320, 320);

    resize(1000, 800);
}

void MapEditorWidget::load(const std::string& filename)
{
    std::ostringstream os;

    MapConverter::Settings sett{
        .m_inputs                  = { .m_fhMap = filename },
        .m_outputs                 = {},
        .m_dumpUncompressedBuffers = false,
        .m_dumpBinaryDataJson      = false,
    };

    MapConverter converter(os,
                           m_gameDatabaseContainer,
                           m_rngFactory,
                           sett);

    converter.run(MapConverter::Task::LoadFH);

    *m_map = std::move(converter.m_mapFH);

    auto rng = m_rngFactory->create();
    rng->setSeed(m_map->m_seed);

    auto* db = m_gameDatabaseContainer->getDatabase(m_map->m_version);

    m_map->initTiles(db);
    m_map->m_tileMap.rngTiles(rng.get());

    updateMap();

    auto* timer = new Gui::TickTimer(this);
    connect(timer, &Gui::TickTimer::tick, this, [this](uint32_t tick) {
        for (auto* sprite : m_mapSprites)
            sprite->tick(tick);
    });
}

void MapEditorWidget::updateMap()
{
    MapRenderer renderer(m_renderSettings);
    m_spriteMap = renderer.render(*m_map, m_graphicsLibrary);

    m_scene->clear();
    for (int d = 0; d < m_spriteMap.m_depth; ++d) {
        SpriteMapItem* item = new SpriteMapItem(&m_spriteMap, &m_paintSettings, d, g_mapAnimationInterval);
        m_mapSprites.push_back(item);
        m_scene->addItem(item);
    }
    m_depth = 0;
    showCurrentItem();

    m_scene->setSceneRect(QRectF(0, 0, m_spriteMap.m_width * m_paintSettings.m_tileSize, m_spriteMap.m_height * m_paintSettings.m_tileSize));
}

MapEditorWidget::~MapEditorWidget()
{
}

void MapEditorWidget::generateMap()
{
    /*
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

    m_adventureMap->m_heroes.push_back(h);*/

    m_scene->update();
}

void MapEditorWidget::showCurrentItem()
{
    for (auto* s : m_mapSprites)
        s->hide();
    m_mapSprites[m_depth]->show();
}

void MapEditorWidget::updateAll()
{
    for (auto* s : m_mapSprites)
        s->update();
}

}
