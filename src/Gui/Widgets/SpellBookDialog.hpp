/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiWidgetsExport.hpp"

#include "AdventureHero.hpp"

#include <QDialog>

#include <memory>

namespace FreeHeroes::Gui {
class SpellsModel;
class UiCommonModel;
class GUIWIDGETS_EXPORT SpellBookDialog : public QDialog {
    Q_OBJECT
public:
    SpellBookDialog(const Core::AdventureHero::SpellList& spellList,
                    const SpellsModel*                    spellsModel,
                    const UiCommonModel*                  ui,
                    int                                   mana,
                    bool                                  allowAdventureCast,
                    bool                                  allowBattleCast,
                    QWidget*                              parent);
    ~SpellBookDialog();

    Core::LibrarySpellConstPtr getSelectedSpell() const;

protected:
    void showBattle();
    void showAdventure();
    void nextPage();
    void prevPage();
    void changeCurrentTab(int index);
    void fillCurrentPage();
    void onSpellClick(Core::LibrarySpellConstPtr spell);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
