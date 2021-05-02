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
}
namespace Gui {
class IGraphicsLibrary;
}

class MapGenTestWidget : public QDialog {
public:
    MapGenTestWidget(Gui::IGraphicsLibrary& graphicsLibrary, Core::IGameDatabase& gameDatabase);
    ~MapGenTestWidget();

private:
    void generateMap();

private:
    QGraphicsView*  m_view  = nullptr;
    QGraphicsScene* m_scene = nullptr;

    Gui::IGraphicsLibrary& m_graphicsLibrary;
    Core::IGameDatabase&   m_gameDatabase;

    //std::unique_ptr<MapMatrix> m_matrix;
    std::unique_ptr<AdventureMap> m_adventureMap;
};

}
