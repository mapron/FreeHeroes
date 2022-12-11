/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include <QDialog>

#include <memory>

class QGraphicsView;
class QGraphicsScene;

namespace FreeHeroes {
class AdventureMap;
namespace Core {
class IGameDatabase;
struct AdventureArmy;
}
namespace Gui {
class IGraphicsLibrary;
class LibraryModelsProvider;
}

class MapGenTestWidget : public QDialog {
public:
    MapGenTestWidget(Gui::IGraphicsLibrary&      graphicsLibrary,
                     const Core::IGameDatabase*  gameDatabase,
                     Gui::LibraryModelsProvider& modelsProvider);
    ~MapGenTestWidget();

private:
    void generateMap();

private:
    QGraphicsView*  m_view  = nullptr;
    QGraphicsScene* m_scene = nullptr;

    Gui::IGraphicsLibrary&      m_graphicsLibrary;
    const Core::IGameDatabase*  m_gameDatabase;
    Gui::LibraryModelsProvider& m_modelsProvider;

    //std::unique_ptr<MapMatrix> m_matrix;
    std::unique_ptr<AdventureMap>        m_adventureMap;
    std::unique_ptr<Core::AdventureArmy> m_hero;
};

}
