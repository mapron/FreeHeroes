/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtilsQt.hpp"
#include "ISprites.hpp"

namespace FreeHeroes::Conversion {

Gui::SpritePtr loadSpriteLegacy(const Core::std_path& defFilePath);

Gui::SpritePtr loadPcx(const Core::std_path& pcxFilePath);

Gui::SpritePtr loadBmp(const Core::std_path& bmpFilePath);

Gui::SpritePtr postProcessSprite(Gui::SpritePtr sprite, const std::string & routine, const std::vector<std::string> &  params);

}
