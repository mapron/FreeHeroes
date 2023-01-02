/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtilsQt.hpp"
#include "ISprites.hpp"
#include "MernelPlatform/PropertyTree.hpp"

namespace FreeHeroes::Core {
class IResourceLibrary;
}
namespace FreeHeroes::Conversion {

void saveSpriteLegacy(Gui::SpritePtr sprite, const Mernel::std_path& defFilePath);

Gui::SpritePtr loadSpriteLegacy(const Mernel::std_path& defFilePath);

Gui::SpritePtr loadPcx(const Mernel::std_path& pcxFilePath);

Gui::SpritePtr loadBmp(const Mernel::std_path& bmpFilePath);

Gui::SpritePtr postProcessSprite(const Mernel::std_path& spritePath, Gui::SpritePtr sprite, const Mernel::PropertyTreeMap& params, Core::IResourceLibrary* library);

}
