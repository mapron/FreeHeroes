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

#include "ViewSettingsWidget.hpp"
#include "SceneView.hpp"
#include "InspectorWidget.hpp"
#include "MiniMapWidget.hpp"
#include "MapScene.hpp"
#include "LibraryModels.hpp"

#include "ViewSettings.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QMessageBox>

#include <vector>
#include <set>
#include <compare>

namespace FreeHeroes {

namespace {
const uint32_t g_mapAnimationInterval = 160;

struct LayerOption {
    std::set<SpriteMap::Layer> m_options;
    std::string                m_descr{};
    QString                    getDescr() const
    {
        if (!m_descr.empty())
            return QString::fromStdString(m_descr);
        if (m_options.empty())
            return "";
        return SpriteMap::layerTypeToString(*m_options.begin());
    }
};

const std::vector<LayerOption> g_allLayerTypes{
    { {} },

    { { SpriteMap::Layer::Terrain } },
    { { SpriteMap::Layer::Town } },
    { { SpriteMap::Layer::Hero } },
    { { SpriteMap::Layer::Resource } },
    { { SpriteMap::Layer::Artifact } },
    { { SpriteMap::Layer::Monster } },
    { { SpriteMap::Layer::Dwelling } },
    { { SpriteMap::Layer::Bank } },
    { { SpriteMap::Layer::Mine } },
    { { SpriteMap::Layer::Pandora } },
    { { SpriteMap::Layer::Shrine } },
    { { SpriteMap::Layer::SkillHut } },
    { { SpriteMap::Layer::Scholar } },
    { { SpriteMap::Layer::QuestHut } },
    { { SpriteMap::Layer::GeneralVisitable } },
    { { SpriteMap::Layer::Decoration } },

    { { SpriteMap::Layer::Dwelling, SpriteMap::Layer::Bank, SpriteMap::Layer::Mine, SpriteMap::Layer::Shrine, SpriteMap::Layer::SkillHut, SpriteMap::Layer::Scholar, SpriteMap::Layer::QuestHut, SpriteMap::Layer::GeneralVisitable }, "Any visitable" },
    { { SpriteMap::Layer::Resource, SpriteMap::Layer::Artifact, SpriteMap::Layer::Pandora }, "Any pickable" },
};

struct AttrOption {
    std::set<Core::ScoreAttr> m_options;
    std::string               m_descr{};
    QString                   getDescr() const
    {
        if (!m_descr.empty())
            return QString::fromStdString(m_descr);
        if (m_options.empty())
            return "";
        return QString::fromStdString(FHScoreSettings::attrToString(*m_options.begin()));
    }
};

const std::vector<AttrOption> g_allAttrTypes{
    { {} },

    { { Core::ScoreAttr::Army } },
    { { Core::ScoreAttr::ArmyDwelling } },
    { { Core::ScoreAttr::ArmyAux } },
    { { Core::ScoreAttr::ArtStat } },
    { { Core::ScoreAttr::ArtSupport } },
    { { Core::ScoreAttr::Gold } },
    { { Core::ScoreAttr::Resource } },
    { { Core::ScoreAttr::ResourceGen } },
    { { Core::ScoreAttr::Experience } },
    { { Core::ScoreAttr::Control } },
    { { Core::ScoreAttr::Upgrade } },
    { { Core::ScoreAttr::SpellOffensive } },
    { { Core::ScoreAttr::SpellCommon } },
    { { Core::ScoreAttr::SpellAny } },
    { { Core::ScoreAttr::Support } },

    { { Core::ScoreAttr::Army, Core::ScoreAttr::ArmyDwelling, Core::ScoreAttr::ArmyAux }, "Any army" },
    { { Core::ScoreAttr::ArtStat, Core::ScoreAttr::ArtSupport, Core::ScoreAttr::Control }, "Any art" },
    { { Core::ScoreAttr::SpellOffensive, Core::ScoreAttr::SpellCommon, Core::ScoreAttr::SpellAny }, "Any spell" },
    { { Core::ScoreAttr::Gold, Core::ScoreAttr::Resource, Core::ScoreAttr::ResourceGen, Core::ScoreAttr::Experience, Core::ScoreAttr::Upgrade, Core::ScoreAttr::Support }, "Misc" },
};
}

struct MapEditorWidget::Impl {
    SceneView* m_view  = nullptr;
    MapScene*  m_scene = nullptr;

    MiniMapWidget*      m_minimapWidget      = nullptr;
    ViewSettingsWidget* m_viewSettingsWidget = nullptr;
    InspectorWidget*    m_inspectorWidget    = nullptr;

    QComboBox* m_filterGen = nullptr;

    FHMap        m_map;
    SpriteMap    m_spriteMap;
    ViewSettings m_viewSettings;

    std::vector<SpriteMapItem*> m_mapSprites;

    int m_depth = 0;
};

MapEditorWidget::MapEditorWidget(const Core::IGameDatabaseContainer*  gameDatabaseContainer,
                                 const Core::IRandomGeneratorFactory* rngFactory,
                                 const Gui::IGraphicsLibrary*         graphicsLibrary,
                                 const Gui::LibraryModelsProvider*    modelsProvider,

                                 QWidget* parent)
    : QMainWindow(parent)
    , m_impl(std::make_unique<Impl>())
    , m_gameDatabaseContainer(gameDatabaseContainer)
    , m_rngFactory(rngFactory)
    , m_graphicsLibrary(graphicsLibrary)
    , m_modelsProvider(modelsProvider)
{
    QWidget* w = new QWidget(this);
    setCentralWidget(w);

    QVBoxLayout* layout       = new QVBoxLayout(w);
    QHBoxLayout* layoutTop    = new QHBoxLayout();
    QHBoxLayout* layoutBottom = new QHBoxLayout();
    QVBoxLayout* layoutSide   = new QVBoxLayout();
    layoutSide->setSpacing(15);

    auto* scaleWidget            = new ScaleWidget(&m_impl->m_viewSettings.m_paintSettings, this);
    m_impl->m_minimapWidget      = new MiniMapWidget(&m_impl->m_viewSettings.m_paintSettings, &m_impl->m_spriteMap, this);
    m_impl->m_viewSettingsWidget = new ViewSettingsWidget(&m_impl->m_viewSettings, this);
    m_impl->m_inspectorWidget    = new InspectorWidget(&m_impl->m_spriteMap, this);
    {
        QCheckBox* viewUnderground = new QCheckBox(tr("Underground"), this);
        QCheckBox* showMinimap     = new QCheckBox(tr("Minimap"), this);
        QCheckBox* showSettings    = new QCheckBox(tr("Display settings"), this);
        QCheckBox* showInspector   = new QCheckBox(tr("Inspector"), this);
        QComboBox* filterType      = new QComboBox(this);
        QComboBox* filterValue     = new QComboBox(this);
        m_impl->m_filterGen        = new QComboBox(this);
        showMinimap->setChecked(true);
        showSettings->setChecked(true);
        showInspector->setChecked(true);

        filterType->addItem(tr("- select layer filter -"));
        for (int i = 1; i < (int) g_allLayerTypes.size(); i++) {
            filterType->addItem(g_allLayerTypes[i].getDescr());
        }
        filterValue->addItem(tr("- select attr filter -"));
        for (int i = 1; i < (int) g_allAttrTypes.size(); i++) {
            filterValue->addItem(g_allAttrTypes[i].getDescr());
        }

        connect(viewUnderground, &QCheckBox::clicked, this, [this](bool state) {
            m_impl->m_depth = state;
            showCurrentItem();
        });
        connect(showMinimap, &QCheckBox::clicked, this, [this](bool state) {
            m_impl->m_minimapWidget->setVisible(state);
        });
        connect(showSettings, &QCheckBox::clicked, this, [this](bool state) {
            m_impl->m_viewSettingsWidget->setVisible(state);
        });
        connect(showInspector, &QCheckBox::clicked, this, [this](bool state) {
            m_impl->m_inspectorWidget->setVisible(state);
        });
        connect(filterType, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, filterType] {
            auto layer                                           = g_allLayerTypes[filterType->currentIndex()];
            m_impl->m_viewSettings.m_paintSettings.m_filterLayer = layer.m_options;
            updateAll();
        });
        connect(filterValue, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, filterValue] {
            auto attr                                           = g_allAttrTypes[filterValue->currentIndex()];
            m_impl->m_viewSettings.m_paintSettings.m_filterAttr = attr.m_options;
            updateAll();
        });
        connect(m_impl->m_filterGen, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
            m_impl->m_viewSettings.m_paintSettings.m_filterGenerationId = m_impl->m_filterGen->currentData().toString().toStdString();
            updateAll();
        });

        layoutTop->addWidget(viewUnderground);
        layoutTop->addWidget(scaleWidget);

        layoutTop->addWidget(showMinimap);
        layoutTop->addWidget(showSettings);
        layoutTop->addWidget(showInspector);
        layoutTop->addWidget(filterType);
        layoutTop->addWidget(filterValue);
        layoutTop->addWidget(m_impl->m_filterGen);
    }

    layout->addLayout(layoutTop);
    layout->addLayout(layoutBottom, 1);
    layoutTop->addStretch();

    m_impl->m_scene = new MapScene(&m_impl->m_viewSettings.m_paintSettings, this);
    m_impl->m_view  = new SceneView(&m_impl->m_viewSettings.m_paintSettings, this);
    m_impl->m_view->setScene(m_impl->m_scene);

    layoutBottom->addWidget(m_impl->m_view, 1);
    layoutBottom->addLayout(layoutSide);
    layoutSide->addWidget(m_impl->m_minimapWidget);
    layoutSide->addWidget(m_impl->m_viewSettingsWidget);
    layoutSide->addWidget(m_impl->m_inspectorWidget);
    layoutSide->addStretch();

    m_impl->m_view->setMinimumSize(320, 320);

    connect(m_impl->m_viewSettingsWidget, &IEditor::dataChanged, this, [this]() {
        updateAll();
    });
    connect(scaleWidget, &ScaleWidget::scaleChanged, m_impl->m_view, &SceneView::refreshScale);
    connect(m_impl->m_view, &SceneView::scaleChangeRequested, scaleWidget, &ScaleWidget::scaleChangeProcess);
    connect(m_impl->m_scene, &MapScene::cellHover, this, [this](int x, int y, int z) {
        if (m_impl->m_viewSettings.m_inspectByHover)
            m_impl->m_inspectorWidget->displayInfo(x, y, z);
    });
    connect(m_impl->m_scene, &MapScene::cellPress, this, [this](int x, int y, int z) {
        if (!m_impl->m_viewSettings.m_inspectByHover)
            m_impl->m_inspectorWidget->displayInfo(x, y, z);
    });
    connect(m_impl->m_view, &SceneView::updateVisible, m_impl->m_minimapWidget, &MiniMapWidget::updateVisible);
    connect(m_impl->m_minimapWidget, &MiniMapWidget::minimapDrag, this, [this](QPointF point) {
        QPointF absPos(m_impl->m_scene->width() * point.x(), m_impl->m_scene->height() * point.y());
        m_impl->m_view->centerOn(absPos);
    });

    resize(1000, 800);

    auto* timer = new Gui::TickTimer(this);
    connect(timer, &Gui::TickTimer::tick, this, [this](uint32_t tick) {
        for (auto* sprite : m_impl->m_mapSprites)
            sprite->tick(tick);
    });

    setWindowIcon(QIcon(":/Application/Logo/64_map.png"));
}
MapEditorWidget::~MapEditorWidget()
{
}

void MapEditorWidget::loadConfig()
{
    m_impl->m_viewSettingsWidget->updateUI();
    /*connect(this, &IEditor::needUpdateUI, this, [this]() {
        for (auto* ed : m_editors)
            ed->updateUI();
    });*/
}

void MapEditorWidget::saveConfig()
{
}

void MapEditorWidget::load(const std::string& filename)
{
    std::ostringstream os;

    MapConverter::Settings sett{
        .m_inputs                  = { .m_fhMap = Mernel::string2path(filename) },
        .m_outputs                 = {},
        .m_dumpUncompressedBuffers = false,
        .m_dumpBinaryDataJson      = false,
    };

    try {
        MapConverter converter(os,
                               m_gameDatabaseContainer,
                               m_rngFactory,
                               sett);

        converter.run(MapConverter::Task::LoadFH);

        m_impl->m_map = std::move(converter.m_mapFH);

        auto rng = m_rngFactory->create();
        rng->setSeed(m_impl->m_map.m_seed);

        auto* db = m_gameDatabaseContainer->getDatabase(m_impl->m_map.m_version);

        m_impl->m_map.initTiles(db);
        m_impl->m_map.m_tileMap.rngTiles(rng.get(), m_impl->m_map.m_template.m_roughTilePercentage);

        m_impl->m_filterGen->clear();
        m_impl->m_filterGen->addItem(tr("-- select gen layer --"));
        auto              allObjects = m_impl->m_map.m_objects.getAllObjects();
        std::set<QString> used;
        for (auto* obj : allObjects) {
            if (!obj->m_generationId.empty())
                used.insert(QString::fromStdString(obj->m_generationId));
        }
        for (const auto& generationId : used)
            m_impl->m_filterGen->addItem(generationId, generationId);

        updateMap();
    }
    catch (std::exception& ex) {
        QMessageBox::warning(this, tr("Error occured"), QString::fromStdString(ex.what()));
    }
}

void MapEditorWidget::updateMap()
{
    MapRenderer renderer(m_impl->m_viewSettings.m_renderSettings);
    m_impl->m_spriteMap = renderer.render(m_impl->m_map, m_graphicsLibrary, m_modelsProvider->database());

    m_impl->m_scene->clear();
    m_impl->m_mapSprites.clear();
    for (int d = 0; d < m_impl->m_spriteMap.m_depth; ++d) {
        SpriteMapItem* item = new SpriteMapItem(&m_impl->m_spriteMap, &m_impl->m_viewSettings.m_paintSettings, d, g_mapAnimationInterval);
        m_impl->m_mapSprites.push_back(item);
        m_impl->m_scene->addItem(item);
    }
    m_impl->m_depth = 0;
    showCurrentItem();

    const auto tileSize = m_impl->m_viewSettings.m_paintSettings.m_tileSize;
    m_impl->m_scene->setSceneRect(QRectF(0, 0, m_impl->m_spriteMap.m_width * tileSize, m_impl->m_spriteMap.m_height * tileSize));
}

void MapEditorWidget::showCurrentItem()
{
    for (auto* s : m_impl->m_mapSprites)
        s->hide();
    if (m_impl->m_depth < (int) m_impl->m_mapSprites.size()) {
        m_impl->m_mapSprites[m_impl->m_depth]->show();
        m_impl->m_minimapWidget->setDepth(m_impl->m_depth);
        m_impl->m_scene->setCurrentDepth(m_impl->m_depth);
    }
}

void MapEditorWidget::updateAll()
{
    for (auto* s : m_impl->m_mapSprites)
        s->update();
    m_impl->m_minimapWidget->update();
}
}
