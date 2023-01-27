/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "FHMap.hpp"

#include "FHTemplateZone.hpp"

#include <stdexcept>
#include <functional>

namespace FreeHeroes {

class FHTemplateProcessor {
public:
    FHTemplateProcessor(FHMap&                     map,
                        const Core::IGameDatabase* database,
                        Core::IRandomGenerator*    rng,
                        std::ostream&              logOutput);

    enum class Stage
    {
        Invalid,
        ZoneCenterPlacement,
        ZoneTilesInitial,
        ZoneTilesExpand,
        ZoneTilesRefinement,
        TownsPlacement,
        BorderRoads,
        RoadsPlacement,
        Rewards,
        Obstacles,
        Guards,
        PlayerInfo,
    };

    void run(const std::string& stopAfterStage);

private:
    void runCurrentStage();
    void runZoneCenterPlacement();
    void runZoneTilesInitial();
    void runZoneTilesExpand();
    void runZoneTilesRefinement();
    void runTownsPlacement();
    void runBorderRoads();
    void runRoadsPlacement();
    void runRewards();
    void runObstacles();
    void runGuards();
    void runPlayerInfo();

    void placeTerrainZones();
    void placeDebugInfo();
    void placeRoad(std::vector<FHPos> path);

    Core::LibraryFactionConstPtr getRandomFaction(bool rewardOnly);
    Core::LibraryHeroConstPtr    getRandomHero(Core::LibraryFactionConstPtr faction);
    TileZone&                    findZoneById(const std::string& id);
    std::vector<FHPos>           aStarPath(MapCanvas::Tile* start, MapCanvas::Tile* end);

private:
    MapCanvas              m_mapCanvas;
    std::vector<TileZone>  m_tileZones;
    std::vector<TileZone*> m_tileZonesPtrs;

    int64_t m_totalRelativeArea = 0;

    Stage m_currentStage = Stage::Invalid;
    Stage m_stopAfter    = Stage::Invalid;

    FHMap&                           m_map;
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::string                      m_indent;
    std::ostream&                    m_logOutput;

    std::vector<Core::LibraryFactionConstPtr> m_playableFactions;
    std::vector<Core::LibraryFactionConstPtr> m_rewardFactions;
    std::vector<Core::LibraryUnitConstPtr>    m_guardUnits;

    struct Guard {
        int64_t     m_value           = 0;
        int64_t     m_valueDispersion = 0;
        std::string m_id;
        std::string m_mirrorFromId;
        FHPos       m_pos;
        TileZone*   m_zone     = nullptr;
        bool        m_joinable = false;
    };
    std::vector<Guard> m_guards;

    using HeroGeneration = FHRngUserSettings::HeroGeneration;
    struct PlayerInfo {
        Core::LibraryFactionConstPtr m_faction = nullptr;

        bool   m_hasMainTown      = false;
        size_t m_mainTownMapIndex = 0;

        Core::LibraryHeroConstPtr m_startingHero    = nullptr;
        Core::LibraryHeroConstPtr m_extraHero       = nullptr;
        HeroGeneration            m_startingHeroGen = HeroGeneration::RandomStartingFaction;
        HeroGeneration            m_extraHeroGen    = HeroGeneration::None;
    };
    std::map<FHPlayerId, PlayerInfo> m_playerInfo;

    std::set<Core::LibraryHeroConstPtr> m_usedHeroes;
};

class AstarGenerator {
public:
    using CoordinateList = std::vector<FHPos>;
    struct Node {
        uint64_t m_G = 0, m_H = 0;
        FHPos    m_pos;
        Node*    m_parent = nullptr;

        Node(FHPos pos, Node* parent = nullptr);
        uint64_t getScore();
    };

    using NodeSet = std::vector<std::shared_ptr<Node>>;

    bool  detectCollision(FHPos coordinates_);
    Node* findNodeOnList(NodeSet& nodes_, FHPos coordinates_);

public:
    AstarGenerator();
    void           setWorldSize(FHPos worldSize_);
    CoordinateList findPath(FHPos source_, FHPos target_);
    void           addCollision(FHPos coordinates_);
    void           removeCollision(FHPos coordinates_);
    void           clearCollisions();

private:
    CoordinateList m_directions, m_collisions;
    FHPos          m_worldSize;
};

}
