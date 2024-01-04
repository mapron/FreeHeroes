/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SpriteMap.hpp"

#include "MapRenderUtilExport.hpp"

namespace FreeHeroes {

struct FHMap;
namespace Core {
class IGameDatabase;
}
namespace Gui {
class IGraphicsLibrary;
}

class MAPRENDERUTIL_EXPORT MapRenderer {
public:
    MapRenderer(const SpriteRenderSettings& renderSettings)
        : m_settings(renderSettings)
    {}

    SpriteMap render(const FHMap& fhMap, const Gui::IGraphicsLibrary* graphicsLibrary) const;

private:
    const SpriteRenderSettings m_settings;
};

}
