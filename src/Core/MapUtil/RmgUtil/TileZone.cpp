/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TileZone.hpp"

#include "TemplateUtils.hpp"

#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes {

void TileZone::setSegments(MapTileRegionWithEdgeList list)
{
    m_innerAreaSegments.resize(list.size());
    for (size_t i = 0; i < list.size(); ++i) {
        m_innerAreaSegments[i]         = Segment(list[i]);
        m_innerAreaSegments[i].m_index = i;
    }
}

MapTileRegionWithEdgeList TileZone::getSegments() const
{
    MapTileRegionWithEdgeList result;
    result.resize(m_innerAreaSegments.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = m_innerAreaSegments[i];
    }
    return result;
}

void TileZone::updateSegmentIndex()
{
    for (auto* tile : m_innerAreaUsable.m_innerArea)
        tile->m_segmentMedium = nullptr;

    m_innerAreaSegmentsUnited.clear();
    for (auto& seg : m_innerAreaSegments) {
        m_innerAreaSegmentsUnited.insert(seg.m_innerArea);
        for (auto* tile : seg.m_innerArea)
            tile->m_segmentMedium = &seg;
    }
}

}
