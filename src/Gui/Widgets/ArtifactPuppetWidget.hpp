/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureFwd.hpp"

#include <QWidget>

#include <memory>
#include <functional>

class QLabel;
namespace FreeHeroes {
namespace Core {
class IAdventureHeroControl;
}

namespace Gui {
class GuiAdventureHero;
class LibraryModelsProvider;

class GUIWIDGETS_EXPORT ArtifactPuppetWidget : public QWidget {
    Q_OBJECT
public:
    ArtifactPuppetWidget(const LibraryModelsProvider* modelProvider, QWidget* parent = nullptr);
    ~ArtifactPuppetWidget();

    void refresh();

    void setHoverLabel(QLabel* hoverLabel);

    void setSource(const GuiAdventureHero*      hero,
                   Core::IAdventureHeroControl* adventureHeroControl);

signals:
    void openSpellBook();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    void scrollBag(int delta);

    class BagListModel;
    class ArtifactButton;
};

}
}
