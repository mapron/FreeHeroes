/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtilsQt.hpp"
#include "ISprites.hpp"
#include "PropertyTree.hpp"

namespace FreeHeroes::Core {
class IResourceLibrary;
}
namespace FreeHeroes::Conversion {

void saveSpriteLegacy(Gui::SpritePtr sprite, const Core::std_path& defFilePath);

Gui::SpritePtr loadSpriteLegacy(const Core::std_path& defFilePath);

Gui::SpritePtr loadPcx(const Core::std_path& pcxFilePath);

Gui::SpritePtr loadBmp(const Core::std_path& bmpFilePath);

Gui::SpritePtr postProcessSprite(const Core::std_path& spritePath, Gui::SpritePtr sprite, const PropertyTreeMap& params, Core::IResourceLibrary* library);

}
