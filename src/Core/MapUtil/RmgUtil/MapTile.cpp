/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapTile.hpp"
#include "MapTileContainer.hpp"

#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes {

FHPos MapTile::Transform::apply(FHPos pos) const noexcept
{
    if (m_flipHor)
        pos.m_x = -pos.m_x;
    if (m_flipVert)
        pos.m_y = -pos.m_y;
    if (m_transpose)
        std::swap(pos.m_x, pos.m_y);

    return pos;
}

MapTilePtr MapTile::neighbourByOffset(FHPos offset) noexcept
{
    //Mernel::ProfilerScope scope("neighbourByOffset");
    if (offset == FHPos{})
        return this;

    if (offset.m_z == 0) {
        switch (offset.m_y) {
            case -1:
            {
                switch (offset.m_x) {
                    case -1:
                        return m_neighborTL;
                    case 0:
                        return m_neighborT;
                    case +1:
                        return m_neighborTR;
                }
            } break;
            case 0:
            {
                switch (offset.m_x) {
                    case -1:
                        return m_neighborL;
                    case +1:
                        return m_neighborR;
                }
            } break;
            case +1:
            {
                switch (offset.m_x) {
                    case -1:
                        return m_neighborBL;
                    case 0:
                        return m_neighborB;
                    case +1:
                        return m_neighborBR;
                }
            } break;
        }
    }

    const auto pos = m_pos + offset;
    auto       it  = m_container->m_tileIndex.find(pos);
    return it == m_container->m_tileIndex.cend() ? nullptr : it->second;
}

MapTilePtrList MapTile::neighboursByOffsets(const std::vector<FHPos>& offsets, const Transform& transform) noexcept
{
    MapTilePtrList result(offsets.size());
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = neighbourByOffset(transform.apply(offsets[i]));
    return result;
}

std::string MapTile::toPrintableString() const noexcept
{
    return m_pos.toPrintableString();
}

}
