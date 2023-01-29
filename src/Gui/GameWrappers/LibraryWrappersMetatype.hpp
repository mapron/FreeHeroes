/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryWrappers.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryHero.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryPlayer.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryUnit.hpp"

#include <QObject>
#include <QVariant>

Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryArtifactConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryFactionConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryHeroConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryMapBankConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryPlayerConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibrarySecondarySkillConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibrarySpellConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryTerrainConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryUnitConstPtr);

Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiArtifactConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiFactionConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiHeroConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiMapBankConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiPlayerConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiSkillConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiSpellConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiTerrainConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiUnitConstPtr);
