/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <QDialog>

#include <memory>

#include "SpriteMap.hpp"

#include "MapUtilGuiExport.hpp"

class QGraphicsView;
class QGraphicsScene;

namespace FreeHeroes {
struct FHMap;
namespace Core {
class IGameDatabaseContainer;
class IRandomGeneratorFactory;
}
namespace Gui {
class IGraphicsLibrary;
class LibraryModelsProvider;
}
class SpriteMapItem;

class MAPUTILGUI_EXPORT MapEditorWidget : public QDialog {
public:
    MapEditorWidget(
        const Core::IGameDatabaseContainer*  gameDatabaseContainer,
        const Core::IRandomGeneratorFactory* rngFactory,
        const Gui::IGraphicsLibrary*         graphicsLibrary,
        const Gui::LibraryModelsProvider*    modelsProvider);

    void load(const std::string& filename);

    void updateMap();

    ~MapEditorWidget();

private:
    void generateMap();
    void showCurrentItem();
    void updateAll();

private:
    QGraphicsView*  m_view  = nullptr;
    QGraphicsScene* m_scene = nullptr;

    const Core::IGameDatabaseContainer* const  m_gameDatabaseContainer;
    const Core::IRandomGeneratorFactory* const m_rngFactory;

    const Gui::IGraphicsLibrary*      m_graphicsLibrary;
    const Gui::LibraryModelsProvider* m_modelsProvider;

    std::unique_ptr<FHMap> m_map;
    SpriteMap              m_spriteMap;
    SpritePaintSettings    m_paintSettings;
    SpriteRenderSettings   m_renderSettings;

    std::vector<SpriteMapItem*> m_mapSprites;

    int m_depth = 0;
};

}
