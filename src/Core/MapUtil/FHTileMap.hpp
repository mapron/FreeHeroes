/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryTerrain.hpp"

#include <optional>
#include <cmath>

#include "MapUtilExport.hpp"

namespace FreeHeroes {

namespace Core {
class IRandomGenerator;
}

struct FHPos {
    int m_x{ 0 };
    int m_y{ 0 };
    int m_z{ 0 };

    constexpr size_t getHash() const noexcept
    {
        return (size_t(m_z) << 16) | (size_t(m_y) << 10) | size_t(m_x);
    }

    constexpr auto operator<=>(const FHPos&) const = default;
};

static inline constexpr const FHPos g_invalidPos{ -1, -1, -1 };

constexpr inline FHPos posNeighbour(FHPos point, int dx, int dy)
{
    point.m_x += dx;
    point.m_y += dy;
    return point;
}

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

struct MAPUTIL_EXPORT FHTileMap {
    enum class SubtileType
    {
        Invalid,
        Sand,
        Dirt,
        Native,
    };

    struct Tile {
        Core::LibraryTerrainConstPtr m_terrain   = nullptr;
        FHRiverType                  m_riverType = FHRiverType::Invalid;
        FHRoadType                   m_roadType  = FHRoadType::Invalid;

        bool m_baseSand = false;
        bool m_baseDirt = false;
        bool m_baseNorm = false;

        SubtileType TL = SubtileType::Invalid;
        SubtileType TR = SubtileType::Invalid;
        SubtileType BL = SubtileType::Invalid;
        SubtileType BR = SubtileType::Invalid;

        uint8_t m_view    = 0xffU;
        uint8_t m_viewMin = 0;
        uint8_t m_viewMid = 0; // for center tiles - margin between 'clear' and 'rough' style.
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

        int m_tileOffset     = 0;
        int m_tileCount      = 0;
        int m_tileCountClear = 0;

        bool setViewBorderSpecial(Core::LibraryTerrain::BorderType borderType);
        bool setViewBorderMixed(Core::LibraryTerrain::BorderType borderType);
        bool setViewBorderSandOrDirt(Core::LibraryTerrain::BorderType borderType, bool sandBorder);
        bool setViewCenter();
        bool setView(Core::LibraryTerrain::BorderClass bc, Core::LibraryTerrain::BorderType borderType);
        void updateMinMax();
    };

    int32_t m_width  = 0;
    int32_t m_height = 0;
    int32_t m_depth  = 0; // no underground => depth==1; has underground => depth==2

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
        return get(correctX(x + dx), correctY(y + dy), z);
    }
    const Tile& getNeighbour(int x, int dx, int y, int dy, int z) const
    {
        return get(correctX(x + dx), correctY(y + dy), z);
    }

    [[nodiscard]] int correctX(int x) const
    {
        return std::clamp(x, 0, m_width - 1);
    }
    [[nodiscard]] int correctY(int y) const
    {
        return std::clamp(y, 0, m_height - 1);
    }
    bool inBounds(int x, int y) const
    {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
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
    const Tile& getNeighbour(const FHPos& pos, int dx, int dy, const Tile& def) const
    {
        return inBounds(pos.m_x + dx, pos.m_y + dy) ? getNeighbour(pos.m_x, dx, pos.m_y, dy, pos.m_z) : def;
    }
    size_t totalSize() const
    {
        return m_width * m_height * m_depth;
    }

    void updateSize()
    {
        m_tiles.resize(totalSize());
    }

    void correctTerrainTypes(Core::LibraryTerrainConstPtr dirtTerrain,
                             Core::LibraryTerrainConstPtr sandTerrain,
                             Core::LibraryTerrainConstPtr waterTerrain);

    void correctRoads();
    void correctRivers();

    void rngTiles(Core::IRandomGenerator* rng, int roughTileChancePercent);

    template<class F>
    void eachPos(F&& f) const
    {
        for (int z = 0; z < m_depth; ++z) {
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    const FHPos pos{ x, y, z };
                    f(pos);
                }
            }
        }
    }
    template<class F>
    void eachPosTile(F&& f) const
    {
        for (int z = 0; z < m_depth; ++z) {
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    const FHPos pos{ x, y, z };
                    f(pos, get(x, y, z));
                }
            }
        }
    }
};

struct FHZone {
    int m_id = 0;

    Core::LibraryTerrainConstPtr m_terrainId = nullptr;
    std::vector<FHPos>           m_tiles;
    std::vector<uint8_t>         m_tilesVariants;
    struct Rect {
        FHPos m_pos{ g_invalidPos };
        int   m_width{ 0 };
        int   m_height{ 0 };
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

namespace std {
template<>
struct hash<FreeHeroes::FHPos> {
    constexpr std::size_t operator()(const FreeHeroes::FHPos& p) const noexcept
    {
        return p.getHash();
    }
};
}
