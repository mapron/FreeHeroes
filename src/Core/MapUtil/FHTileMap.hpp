/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryTerrain.hpp"

#include <optional>

namespace FreeHeroes {

namespace Core {
class IRandomGenerator;
}

struct FHPos {
    uint32_t m_x{ 0 };
    uint32_t m_y{ 0 };
    uint8_t  m_z{ 0 };

    constexpr auto operator<=>(const FHPos&) const = default;
};

static inline constexpr const FHPos g_invalidPos{ uint32_t(-1), uint32_t(-1), uint8_t(-1) };

enum class FHRiverType
{
    Invalid,
    Water,
    Ice,
    Mud,
    Lava,
};
enum class FHRoadType
{
    Invalid,
    Dirt,
    Gravel,
    Cobblestone,
};

struct FHTileMap {
    struct Tile {
        Core::LibraryTerrainConstPtr m_terrain   = nullptr;
        FHRiverType                  m_riverType = FHRiverType::Invalid;
        FHRoadType                   m_roadType  = FHRoadType::Invalid;

        uint8_t m_view    = 0xffU;
        uint8_t m_viewMin = 0;
        uint8_t m_viewMax = 0;

        bool m_flipHor  = false;
        bool m_flipVert = false;
        bool m_coastal  = false;

        uint8_t m_roadView     = 0xffU;
        uint8_t m_roadViewMin  = 0;
        uint8_t m_roadViewMax  = 0;
        bool    m_roadFlipHor  = false;
        bool    m_roadFlipVert = false;

        uint8_t m_riverView     = 0xffU;
        uint8_t m_riverViewMin  = 0;
        uint8_t m_riverViewMax  = 0;
        bool    m_riverFlipHor  = false;
        bool    m_riverFlipVert = false;

        int m_tileOffset = 0;
        int m_tileCount  = 0;

        void calculateOffsets(Core::LibraryTerrain::BorderType borderType, bool dirtBorder, bool sandBorder);
    };

    uint32_t m_width  = 0;
    uint32_t m_height = 0;
    uint32_t m_depth  = 0; // no underground => depth==1; has underground => depth==2

    std::vector<Tile> m_tiles;

    Tile& get(int x, int y, int z)
    {
        const int zOffset = m_width * m_height * z;
        const int yOffset = m_width * y;
        return m_tiles[zOffset + yOffset + x];
    }
    const Tile& get(int x, int y, int z) const
    {
        const int zOffset = m_width * m_height * z;
        const int yOffset = m_width * y;
        return m_tiles[zOffset + yOffset + x];
    }

    Tile& getNeighbour(int x, int dx, int y, int dy, int z)
    {
        return get(std::clamp(x + dx, 0, (int) m_width - 1), std::clamp(y + dy, 0, (int) m_height - 1), z);
    }
    const Tile& getNeighbour(int x, int dx, int y, int dy, int z) const
    {
        return get(std::clamp(x + dx, 0, (int) m_width - 1), std::clamp(y + dy, 0, (int) m_height - 1), z);
    }

    Tile& get(const FHPos& pos)
    {
        return get(pos.m_x, pos.m_y, pos.m_z);
    }
    const Tile& get(const FHPos& pos) const
    {
        return get(pos.m_x, pos.m_y, pos.m_z);
    }
    Tile& getNeighbour(const FHPos& pos, int dx, int dy)
    {
        return getNeighbour(pos.m_x, dx, pos.m_y, dy, pos.m_z);
    }
    const Tile& getNeighbour(const FHPos& pos, int dx, int dy) const
    {
        return getNeighbour(pos.m_x, dx, pos.m_y, dy, pos.m_z);
    }

    void updateSize()
    {
        m_tiles.resize(m_width * m_height * m_depth);
    }

    void correctTerrainTypes(Core::LibraryTerrainConstPtr dirtTerrain,
                             Core::LibraryTerrainConstPtr sandTerrain,
                             Core::LibraryTerrainConstPtr waterTerrain);

    void correctRoads();
    void correctRivers();

    void rngTiles(Core::IRandomGenerator* rng);
};

struct FHZone {
    int m_id = 0;

    Core::LibraryTerrainConstPtr m_terrainId = nullptr;
    std::vector<FHPos>           m_tiles;
    std::vector<uint8_t>         m_tilesVariants;
    struct Rect {
        FHPos    m_pos{ g_invalidPos };
        uint32_t m_width{ 0 };
        uint32_t m_height{ 0 };
    };
    std::optional<Rect> m_rect;

    void placeOnMap(FHTileMap& map) const;
};

struct FHRiver {
    FHRiverType          m_type = FHRiverType::Invalid;
    std::vector<FHPos>   m_tiles;
    std::vector<uint8_t> m_tilesVariants;

    void placeOnMap(FHTileMap& map) const;
};

struct FHRoad {
    FHRoadType           m_type = FHRoadType::Invalid;
    std::vector<FHPos>   m_tiles;
    std::vector<uint8_t> m_tilesVariants;

    void placeOnMap(FHTileMap& map) const;
};

}
