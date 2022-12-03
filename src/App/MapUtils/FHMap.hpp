/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <compare>
#include <set>
#include <optional>

#include "PropertyTree.hpp"
#include "GameConstants.hpp"

#include "AdventureArmy.hpp"

#include "LibraryTerrain.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

enum class FHPlayerId
{
    Invalid = -2,
    None    = -1,
    Red     = 0,
    Blue,
    Tan,
    Green,
    Orange,
    Purple,
    Teal,
    Pink,
};
struct FHPos {
    uint32_t m_x{ 0 };
    uint32_t m_y{ 0 };
    uint8_t  m_z{ 0 };

    constexpr auto operator<=>(const FHPos&) const = default;
};

static inline constexpr const FHPos g_invalidPos{ uint32_t(-1), uint32_t(-1), uint8_t(-1) };

struct FHPlayer {
    //int                      m_id{ 0 };
    bool                                      m_humanPossible{ true };
    bool                                      m_aiPossible{ true };
    std::vector<Core::LibraryFactionConstPtr> m_startingFactions;
};

struct FHCommonObject {
    FHPos m_pos{ g_invalidPos };
};

struct FHPlayerControlledObject : public FHCommonObject {
    FHPlayerId m_player = FHPlayerId::Invalid;
};

struct FHHeroData {
    bool m_hasExp        = false;
    bool m_hasSecSkills  = false;
    bool m_hasPrimSkills = false;
    bool m_hasCustomBio  = false;
    bool m_hasSpells     = false;

    Core::AdventureArmy m_army;
};

struct FHHero : public FHPlayerControlledObject {
    bool        m_isMain{ false };
    std::string m_id;
    uint32_t    m_questIdentifier = 0;
};

struct FHTown : public FHPlayerControlledObject {
    bool        m_isMain{ false };
    std::string m_faction;
    bool        m_hasFort{ false };
    uint32_t    m_questIdentifier = 0;
    bool        m_spellResearch{ false };
    std::string m_defFile; // just for the sake of roundtrip tests.
};

struct FHTileMap {
    struct Tile {
        Core::LibraryTerrainConstPtr m_terrain = nullptr;

        uint8_t m_view    = 0xff;
        uint8_t m_viewMin = 0;
        uint8_t m_viewMax = 0;

        bool m_flipHor  = false;
        bool m_flipVert = false;
        bool m_coastal  = false;

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

    void rngTiles(Core::IRandomGenerator* rng);
};

struct FHZone {
    int m_id = 0;

    Core::LibraryTerrainConstPtr m_terrain = nullptr;
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

struct FHResource : public FHCommonObject {
    uint32_t                      m_amount   = 0;
    Core::LibraryResourceConstPtr m_resource = nullptr;
};

struct FHMap {
    using PlayersMap = std::map<FHPlayerId, FHPlayer>;

    Core::GameVersion m_version = Core::GameVersion::Invalid;
    uint64_t          m_seed{ 0 };

    FHTileMap m_tileMap;

    std::string m_name;
    std::string m_descr;
    uint8_t     m_difficulty = 0;

    PlayersMap          m_players;
    std::vector<FHHero> m_wanderingHeroes;
    std::vector<FHTown> m_towns;
    std::vector<FHZone> m_zones;

    struct Objects {
        std::vector<FHResource> m_resources;
    } m_objects;

    Core::LibraryTerrainConstPtr m_defaultTerrain = nullptr;

    std::vector<std::string> m_disabledHeroes;
    std::vector<std::string> m_disabledArtifacts;
    std::vector<std::string> m_disabledSpells;
    std::vector<std::string> m_disabledSkills;

    std::vector<FHHeroData> m_customHeroes;

    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data, const Core::IGameDatabase* database);
};

}
