/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once


#include "LibraryWrappers.hpp"

#include "LibraryArtifact.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryUnit.hpp"
#include "LibraryHero.hpp"

#include <QObject>
#include <QVariant>

Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryArtifactConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibrarySecondarySkillConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibrarySpellConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryUnitConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryHeroConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Core::LibraryFactionConstPtr);


Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiArtifactConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiHeroConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiUnitConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiSpellConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiSkillConstPtr);
Q_DECLARE_METATYPE(FreeHeroes::Gui::GuiFactionConstPtr);
