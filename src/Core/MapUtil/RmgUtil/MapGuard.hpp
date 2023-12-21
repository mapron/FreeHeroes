/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapTile.hpp"
#include "MapScore.hpp"

namespace FreeHeroes {

struct MapGuard {
    int64_t        m_value = 0;
    std::string    m_id;
    std::string    m_mirrorFromId;
    MapTilePtr     m_pos  = nullptr;
    TileZone*      m_zone = nullptr;
    Core::MapScore m_score;
    std::string    m_generationId;
    bool           m_joinable = false;
};
using MapGuardList = std::vector<MapGuard>;

}
