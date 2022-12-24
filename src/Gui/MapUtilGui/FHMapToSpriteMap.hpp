/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

namespace FreeHeroes {

struct SpriteMap;
struct FHMap;
namespace Gui {
class IGraphicsLibrary;
}

class MapRenderer {
public:
    SpriteMap render(const FHMap& fhMap, const Gui::IGraphicsLibrary* graphicsLibrary, int z) const;
};

}
