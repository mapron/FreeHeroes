/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "MapEditorWidget.hpp"

#include "MernelPlatform/Logger.hpp"

#include "FHMap.hpp"
#include "MapConverter.hpp"

#include "SpriteMap.hpp"
#include "SpriteMapItem.hpp"
#include "FHMapToSpriteMap.hpp"

#include "IRandomGenerator.hpp"
#include "IGameDatabase.hpp"

#include "IAppSettings.hpp"
#include "TickTimer.hpp"

#include "ViewSettingsWidget.hpp"
#include "SceneView.hpp"
#include "InspectorWidget.hpp"
#include "SpriteMapPainter.hpp"
#include "MiniMapWidget.hpp"
#include "MapScene.hpp"
#include "LibraryModels.hpp"

#include "ViewSettings.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QMenuBar>
#include <QFileDialog>

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QMessageBox>

#include <vector>
#include <set>
#include <compare>

namespace FreeHeroes {

namespace {
const uint32_t    g_mapAnimationInterval = 160;
const std::string g_customSettingsName   = "mapEditor";

struct LayerOption {
    std::set<SpriteMap::Layer> m_options;
    std::string                m_descr{};
    QString                    getDescr() const
    {
        if (!m_descr.empty())
            return QString::fromStdString(m_descr);
        if (m_options.empty())
            return "";
        return QString::fromStdString(SpriteMap::layerTypeToString(*m_options.begin()));
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
    { { SpriteMap::Layer::QuestGuard } },
    { { SpriteMap::Layer::GeneralVisitable } },
    { { SpriteMap::Layer::Decoration } },
    { { SpriteMap::Layer::Event } },

    { { SpriteMap::Layer::Dwelling, SpriteMap::Layer::Bank, SpriteMap::Layer::Mine, SpriteMap::Layer::Shrine, SpriteMap::Layer::SkillHut, SpriteMap::Layer::Scholar, SpriteMap::Layer::QuestHut, SpriteMap::Layer::QuestGuard, SpriteMap::Layer::GeneralVisitable }, "Any visitable" },
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
                                 Gui::IAppSettings*                   appSettings,

                                 QWidget* parent)
    : QMainWindow(parent)
    , m_impl(std::make_unique<Impl>())
    , m_gameDatabaseContainer(gameDatabaseContainer)
    , m_rngFactory(rngFactory)
    , m_graphicsLibrary(graphicsLibrary)
    , m_modelsProvider(modelsProvider)
    , m_appSettings(appSettings)
{
    QWidget* w = new QWidget(this);
    setCentralWidget(w);

    loadUserSettings();

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

    m_impl->m_view->refreshScale();

    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    {
        QAction* load           = fileMenu->addAction(tr("Load H3M/FH..."));
        QAction* saveH3         = fileMenu->addAction(tr("Save H3M..."));
        QAction* saveFH         = fileMenu->addAction(tr("Save FH..."));
        QAction* saveScreenshot = fileMenu->addAction(tr("Screenshot as png..."));

        connect(load, &QAction::triggered, this, &MapEditorWidget::loadDialog);
        connect(saveH3, &QAction::triggered, this, &MapEditorWidget::saveH3MDialog);
        connect(saveFH, &QAction::triggered, this, &MapEditorWidget::saveFHDialog);
        connect(saveScreenshot, &QAction::triggered, this, &MapEditorWidget::saveScreenshot);
    }
    QMenu* actionsMenu = menuBar()->addMenu(tr("Actions"));
    {
        QAction* derando = actionsMenu->addAction(tr("Replace random objects"));

        connect(derando, &QAction::triggered, this, &MapEditorWidget::derandomize);
    }

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
    saveUserSettings();
}

void MapEditorWidget::loadUserSettings()
{
    Mernel::PropertyTree jdata = m_appSettings->loadCustomJson(g_customSettingsName);
    if (jdata.isMap())
        m_impl->m_viewSettings.fromJson(jdata);
}

void MapEditorWidget::saveUserSettings()
{
    Mernel::PropertyTree jdata;
    m_impl->m_viewSettings.toJson(jdata);

    m_appSettings->saveCustomJson(jdata, g_customSettingsName);
}

bool MapEditorWidget::load(const std::string& filename,
                           bool               strict,
                           bool               silent)
{
    std::ostringstream os;
    auto               fullpath = Mernel::string2path(filename);
    auto               ext      = fullpath.extension();
    const bool         isH3M    = Mernel::path2string(ext) == ".h3m";

    MapConverter::Settings sett;
    if (isH3M)
        sett.m_inputs = { .m_h3m = { .m_binary = fullpath } };
    else
        sett.m_inputs = { .m_fhMap = fullpath };

    try {
        m_impl->m_viewSettings.m_renderSettings.m_strict = strict;
        MapConverter converter(os,
                               m_gameDatabaseContainer,
                               m_rngFactory,
                               sett);

        if (isH3M)
            converter.run(MapConverter::Task::LoadH3M);
        else
            converter.run(MapConverter::Task::LoadFH);

        assert(converter.m_mapFH.m_database);
        m_impl->m_map = std::move(converter.m_mapFH);

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
        return true;
    }
    catch (std::exception& ex) {
        if (!silent)
            QMessageBox::warning(this, tr("Error occured"), QString::fromStdString(ex.what()));
        else
            Mernel::Logger(Mernel::Logger::Err) << ex.what();
        return false;
    }
}

void MapEditorWidget::save(const std::string& filename, bool isH3M)
{
    std::ostringstream     os;
    auto                   fullpath = Mernel::string2path(filename);
    MapConverter::Settings sett;
    if (isH3M)
        sett.m_outputs = { .m_h3m = { .m_binary = fullpath } };
    else
        sett.m_outputs = { .m_fhMap = fullpath };

    try {
        MapConverter converter(os,
                               m_gameDatabaseContainer,
                               m_rngFactory,
                               sett);

        converter.m_mapFH = m_impl->m_map;

        if (isH3M)
            converter.run(MapConverter::Task::SaveH3M);
        else
            converter.run(MapConverter::Task::SaveFH);
    }
    catch (std::exception& ex) {
        QMessageBox::warning(this, tr("Error occured"), QString::fromStdString(ex.what()));
    }
}

void MapEditorWidget::loadDialog()
{
    QString filename = QFileDialog::getOpenFileName(this, "", "", "FH or H3 map (*.json *.h3m)");
    if (filename.isEmpty())
        return;
    load(filename.toStdString(), false);
}

void MapEditorWidget::saveH3MDialog()
{
    QString filename = QFileDialog::getSaveFileName(this, "", "", "H3 map (*.h3m)");
    if (filename.isEmpty())
        return;
    if (!filename.endsWith(".h3m"))
        filename += ".h3m";
    save(filename.toStdString(), true);
}

void MapEditorWidget::saveFHDialog()
{
    QString filename = QFileDialog::getSaveFileName(this, "", "", "FH (*.json)");
    if (filename.isEmpty())
        return;
    if (!filename.endsWith(".json"))
        filename += ".fh.json";
    save(filename.toStdString(), false);
}

void MapEditorWidget::saveScreenshot()
{
    QString filename = QFileDialog::getSaveFileName(this, "", "", "Images (*.png)");
    if (filename.isEmpty())
        return;

    QImage   image(m_impl->m_scene->width(), m_impl->m_scene->height(), QImage::Format_ARGB32);
    QPainter painter(&image);
    m_impl->m_scene->render(&painter);
    image.save(filename);
}

bool MapEditorWidget::saveScreenshots(const ScreenshotTask& task)
{
    Mernel::Logger(Mernel::Logger::Notice) << "Loading " << task.m_filename;
    if (!load(task.m_filename, task.m_strict, task.m_silent))
        return false;

    try {
        for (int d : { 0, 1 }) {
            const std::string& outFilename  = d == 0 ? task.m_outputSurface : task.m_outputUnderground;
            const std::string& miniFilename = d == 0 ? task.m_minimapSurface : task.m_minimapUnderground;

            if (d >= m_impl->m_spriteMap.m_depth)
                break;

            m_impl->m_depth = d;

            m_impl->m_viewSettings.m_paintSettings.m_strict = task.m_strict;
            SpriteMapPainter spainter(&(m_impl->m_viewSettings.m_paintSettings), m_impl->m_depth);

            QSize mainSize(m_impl->m_scene->width(), m_impl->m_scene->height());

            QImage   imageMap(mainSize, QImage::Format_ARGB32);
            QPainter painterMap(&imageMap);

            QImage   imageMini(task.m_minimapSize, task.m_minimapSize, QImage::Format_ARGB32);
            QPainter painterMini(&imageMini);

            spainter.paint(&painterMap, &(m_impl->m_spriteMap), 0, 0);
            spainter.paintMinimap(&painterMini, &(m_impl->m_spriteMap), QSize{ task.m_minimapSize, task.m_minimapSize }, QRectF());

            if (task.m_dryRun)
                continue;

            if (!outFilename.empty()) {
                QImage imageMapSaving = imageMap;
                if (mainSize.width() > task.m_maxSize) {
                    while (mainSize.width() > task.m_maxSize) {
                        mainSize = mainSize / 2.0;
                    }
                    imageMapSaving = imageMapSaving.scaled(mainSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
                Mernel::Logger(Mernel::Logger::Notice) << "Saving map to: " << outFilename;
                imageMapSaving.save(QString::fromStdString(outFilename));
            }
            if (!miniFilename.empty()) {
                Mernel::Logger(Mernel::Logger::Notice) << "Saving minimap to: " << miniFilename;
                imageMini.save(QString::fromStdString(miniFilename));
            }
        }
    }
    catch (std::exception& ex) {
        Mernel::Logger(Mernel::Logger::Err) << ex.what();
        return false;
    }

    return true;
}

void MapEditorWidget::updateMap()
{
    MapRenderer renderer(m_impl->m_viewSettings.m_renderSettings);
    assert(m_impl->m_map.m_database);
    m_impl->m_spriteMap = renderer.render(m_impl->m_map, m_graphicsLibrary);

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

void MapEditorWidget::derandomize()
{
    auto rng = m_rngFactory->create();
    rng->makeGoodSeed();
    m_impl->m_map.derandomize(rng.get());
    updateMap();
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
