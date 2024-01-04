/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SpriteMap.hpp"

#include "MapRenderUtilExport.hpp"

namespace Mernel {
class PropertyTree;
}

namespace FreeHeroes {

struct MAPRENDERUTIL_EXPORT ViewSettings {
    SpriteRenderSettings m_renderSettings;
    SpritePaintSettings  m_paintSettings;

    bool m_inspectByHover;

    void toJson(Mernel::PropertyTree& data) const;
    void fromJson(const Mernel::PropertyTree& data);
};

}
