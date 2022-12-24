/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureFwd.hpp"
#include "LibraryFwd.hpp"
#include "LibrarySpell.hpp"
#include "Stat.hpp"

#include <QDialog>

#include <memory>

namespace FreeHeroes::Core {
struct BattleStack;
using BattleStackConstPtr = const BattleStack*;
}

namespace FreeHeroes::Gui {
class LibraryModelsProvider;
class GUIWIDGETS_EXPORT UnitInfoWidget : public QDialog {
    Q_OBJECT
public:
    UnitInfoWidget(Core::BattleStackConstPtr    battle,
                   Core::AdventureStackConstPtr adventure,
                   const LibraryModelsProvider* modelsProvider,
                   bool                         showModal,
                   QWidget*                     parent = nullptr);
    ~UnitInfoWidget();
    bool isShowModal() const;

signals:
    void deleteStack(Core::AdventureStackConstPtr stack);

private:
    QStringList abilitiesText(Core::LibraryUnitConstPtr unit) const;
    QStringList abilitiesTextExtra(Core::AdventureStackConstPtr adventure) const;
    QStringList retaliationsDescription(int countMax) const;
    QStringList resistInfo(const Core::MagicReduce& reduce, const Core::BonusRatio& successRate) const;
    QStringList immuneInfo(const Core::SpellFilter& immunes) const;
    QStringList vulnerabilityInfo(Core::LibraryUnitConstPtr unit) const;
    QStringList castsInfo(Core::BattleStackConstPtr    battle,
                          Core::AdventureStackConstPtr adventure) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
