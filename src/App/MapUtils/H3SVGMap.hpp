/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "H3MMap.hpp"

namespace FreeHeroes {

struct H3SVGMap {
    uint32_t m_versionMajor = 0;
    uint32_t m_versionMinor = 0;

    MapFormat   m_format     = MapFormat::Invalid;
    bool        m_anyPlayers = false;
    std::string m_mapName;
    std::string m_mapDescr;

    MapTileSet m_tiles;

    H3SVGMap();

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);
};

}
