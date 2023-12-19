/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TileZone.hpp"
#include "MapGuard.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHMap;
class SegmentHelper {
public:
    SegmentHelper(FHMap&                        map,
                  MapTileContainer&             tileContainer,
                  Core::IRandomGenerator* const rng,
                  std::ostream&                 logOutput,
                  bool                          extraLogging);

    void makeInitialZones(std::vector<TileZone>& tileZones);

    MapGuardList makeBorders(std::vector<TileZone>& tileZones);

    void makeSegments(TileZone& tileZone);
    void refineSegments(TileZone& tileZone);
    void makeHeatMap(TileZone& tileZone);

private:
    std::string                   m_indent = "        ";
    FHMap&                        m_map;
    MapTileContainer&             m_tileContainer;
    Core::IRandomGenerator* const m_rng;
    std::ostream&                 m_logOutput;
    const bool                    m_extraLogging;
};

}
