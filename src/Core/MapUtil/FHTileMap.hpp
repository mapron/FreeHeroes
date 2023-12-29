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

#include "FHPos.hpp"

namespace FreeHeroes {

namespace Core {
class IRandomGenerator;
class IGameDatabase;
}

struct MAPUTIL_EXPORT FHTileMap {
    enum class SubtileType
    {
        Invalid,
        Sand,
        Dirt,
        Native,
    };

    struct TileView {
        uint8_t m_view     = 0;
        uint8_t m_viewMin  = 0;
        uint8_t m_viewMid  = 0; // for center tiles - margin between 'clear' and 'rough' style.
        uint8_t m_viewMax  = 0;
        bool    m_flipHor  = false;
        bool    m_flipVert = false;

        bool m_recommendedFlipHor  = false;
        bool m_recommendedFlipVert = false;

        void makeRecommendedRotation()
        {
            m_flipHor  = m_recommendedFlipHor;
            m_flipVert = m_recommendedFlipVert;
        }

        void makeRngView(Core::IRandomGenerator* rng, int roughTileChancePercent, bool useMid = false);
    };

    struct Tile {
        Core::LibraryTerrainConstPtr m_terrainId = nullptr;
        FHRiverType                  m_riverType = FHRiverType::Invalid;
        FHRoadType                   m_roadType  = FHRoadType::Invalid;

        SubtileType TL = SubtileType::Invalid;
        SubtileType TR = SubtileType::Invalid;
        SubtileType BL = SubtileType::Invalid;
        SubtileType BR = SubtileType::Invalid;

        TileView m_terrainView;
        TileView m_roadView;
        TileView m_riverView;

        bool m_coastal = false;

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

    void determineTerrainViewRotation(Core::LibraryTerrainConstPtr dirtTerrain,
                                      Core::LibraryTerrainConstPtr sandTerrain,
                                      Core::LibraryTerrainConstPtr waterTerrain);

    void determineRoadViewRotation();
    void determineRiverViewRotation();

    void determineViewRotation(const Core::IGameDatabase* database);

    void makeRecommendedRotation();
    void makeRngView(Core::IRandomGenerator* rng, int roughTileChancePercent);

    void eachPosTile(auto&& f) const
    {
        size_t offset = 0;
        for (int z = 0; z < m_depth; ++z) {
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    const FHPos pos{ x, y, z };
                    f(pos, get(x, y, z), offset);
                    offset++;
                }
            }
        }
    }

    void eachPosTile(auto&& f)
    {
        size_t offset = 0;
        for (int z = 0; z < m_depth; ++z) {
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    const FHPos pos{ x, y, z };
                    f(pos, get(x, y, z), offset);
                    offset++;
                }
            }
        }
    }
};

struct MAPUTIL_EXPORT FHPackedTileMap {
    struct River {
        FHRiverType          m_type = FHRiverType::Invalid;
        std::vector<FHPos>   m_tiles;
        std::vector<uint8_t> m_views;
        std::vector<uint8_t> m_flipHor;
        std::vector<uint8_t> m_flipVert;

        constexpr bool operator==(const River&) const = default;
    };

    struct Road {
        FHRoadType           m_type = FHRoadType::Invalid;
        std::vector<FHPos>   m_tiles;
        std::vector<uint8_t> m_views;
        std::vector<uint8_t> m_flipHor;
        std::vector<uint8_t> m_flipVert;

        constexpr bool operator==(const Road&) const = default;
    };

    std::vector<Core::LibraryTerrainConstPtr> m_terrains;

    std::vector<uint8_t> m_tileTerrianIndexes;
    std::vector<uint8_t> m_tileViews;

    std::vector<uint8_t> m_terrainFlipHor;
    std::vector<uint8_t> m_terrainFlipVert;
    std::vector<uint8_t> m_coastal;

    std::vector<River> m_rivers;
    std::vector<Road>  m_roads;

    constexpr bool operator==(const FHPackedTileMap&) const = default;

    void unpackToMap(FHTileMap& map) const;
    void packFromMap(const FHTileMap& map);
};

}
