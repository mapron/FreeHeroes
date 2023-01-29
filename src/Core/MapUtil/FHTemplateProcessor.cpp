/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTemplateProcessor.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "RmgUtil/TemplateUtils.hpp"
#include "RmgUtil/ObjectGenerator.hpp"
#include "RmgUtil/KMeans.hpp"
#include "RmgUtil/AstarGenerator.hpp"
#include "RmgUtil/ObstacleUtil.hpp"

#include <functional>
#include <stdexcept>
#include <iostream>

namespace Mernel::Reflection {
using namespace FreeHeroes;
ENUM_REFLECTION_STRINGIFY(
    FHTemplateProcessor::Stage,
    Invalid,
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
    PlayerInfo)

}

namespace FreeHeroes {

namespace {

std::string stageToString(FHTemplateProcessor::Stage stage)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(stage);
    return std::string(str.begin(), str.end());
}
FHTemplateProcessor::Stage stringToStage(const std::string& str)
{
    if (str.empty())
        return FHTemplateProcessor::Stage::Invalid;
    auto result = Mernel::Reflection::EnumTraits::stringToEnum<FHTemplateProcessor::Stage>({ str.c_str(), str.size() });
    if (result == FHTemplateProcessor::Stage::Invalid) {
        throw std::runtime_error("Invalid stage name provided:" + str);
    }
    return result;
}

struct DistanceRecord {
    TileZone* m_zoneIndex  = nullptr;
    int64_t   m_distance   = 0;
    int64_t   m_zoneRadius = 0;

    int64_t dbr() const
    {
        const int64_t distanceByRadius = m_distance * 1000 / m_zoneRadius;
        return distanceByRadius;
    }
};

}

FHTemplateProcessor::FHTemplateProcessor(FHMap&                     map,
                                         const Core::IGameDatabase* database,
                                         Core::IRandomGenerator*    rng,
                                         std::ostream&              logOutput)
    : m_map(map)
    , m_database(database)
    , m_rng(rng)
    , m_logOutput(logOutput)
{
    auto& factions = m_database->factions()->records();
    for (auto* faction : factions) {
        if (faction->alignment == Core::LibraryFaction::Alignment::Special)
            continue;
        m_rewardFactions.push_back(faction);
        if (faction->alignment == Core::LibraryFaction::Alignment::Independent)
            continue;
        m_playableFactions.push_back(faction);
        for (auto* hero : faction->heroes) {
            if (!map.m_disabledHeroes.isDisabled(map.m_isWaterMap, hero))
                m_heroPool.insert(hero);
        }
    }
    assert(!m_playableFactions.empty());
    auto& units = m_database->units()->records();
    for (auto* unit : units) {
        if (unit->faction->alignment == Core::LibraryFaction::Alignment::Special)
            continue;
        m_guardUnits.push_back(unit);
    }
}

void FHTemplateProcessor::run(const std::string& stopAfterStage)
{
    std::string baseIndent        = "      ";
    m_indent                      = baseIndent + "  ";
    m_map.m_tileMapUpdateRequired = false;

    const int regionCount = m_map.m_rngZones.size();
    if (regionCount <= 1)
        throw std::runtime_error("need at least two zones");

    m_logOutput << baseIndent << "Start generating map (seed=" << m_map.m_seed << ") " << m_map.m_tileMap.m_width << "x" << m_map.m_tileMap.m_height
                << " " << (m_map.m_tileMap.m_depth == 2 ? "+U" : "no U") << "\n";

    m_mapCanvas.init(m_map.m_tileMap.m_width, m_map.m_tileMap.m_height, m_map.m_tileMap.m_depth);

    m_totalRelativeArea = 0;

    for (const auto& [playerId, info] : m_map.m_players) {
        m_playerInfo[playerId] = {};
    }
    for (const auto& [key, rngZone] : m_map.m_rngZones) {
        if (rngZone.m_player <= FHPlayerId::None)
            continue;

        if (!m_playerInfo.contains(rngZone.m_player))
            throw std::runtime_error("You need to define all players used in zones.");
    }
    for (auto& [playerId, info] : m_playerInfo) {
        if (!m_map.m_rngUserSettings.m_players.contains(playerId))
            continue;
        auto& pl               = m_map.m_rngUserSettings.m_players[playerId];
        info.m_faction         = pl.m_faction;
        info.m_startingHero    = pl.m_startingHero;
        info.m_extraHero       = pl.m_extraHero;
        info.m_startingHeroGen = pl.m_startingHeroGen;
        info.m_extraHeroGen    = pl.m_extraHeroGen;
    }

    auto heroGen = [this](Core::LibraryHeroConstPtr hero, Core::LibraryFactionConstPtr faction, HeroGeneration policy) -> Core::LibraryHeroConstPtr {
        if (policy == HeroGeneration::None)
            return nullptr;
        if (policy == HeroGeneration::FixedAny || policy == HeroGeneration::FixedStarting) {
            m_heroPool.erase(hero);
            return hero;
        }
        return getRandomHero(policy == HeroGeneration::RandomStartingFaction ? faction : nullptr);
    };

    for (auto& [playerId, info] : m_playerInfo) {
        if (!info.m_faction)
            info.m_faction = getRandomFaction(false);
        info.m_startingHero = heroGen(info.m_startingHero, info.m_faction, info.m_startingHeroGen);
        info.m_extraHero    = heroGen(info.m_extraHero, info.m_faction, info.m_extraHeroGen);
    }

    m_tileZones.resize(regionCount);
    for (int i = 0; const auto& [key, rngZone] : m_map.m_rngZones) {
        auto& tileZone             = m_tileZones[i];
        tileZone.m_player          = rngZone.m_player;
        tileZone.m_mainTownFaction = rngZone.m_mainTownFaction;
        tileZone.m_rewardsFaction  = rngZone.m_rewardsFaction;
        if (!tileZone.m_mainTownFaction) {
            if (m_playerInfo.contains(tileZone.m_player)) {
                tileZone.m_mainTownFaction = m_playerInfo[tileZone.m_player].m_faction;
            } else {
                //neutral zones
                tileZone.m_mainTownFaction = getRandomFaction(false);
            }
        }
        if (!tileZone.m_rewardsFaction) {
            tileZone.m_rewardsFaction = tileZone.m_mainTownFaction;
        }
        assert(tileZone.m_mainTownFaction);
        assert(tileZone.m_rewardsFaction);
        tileZone.m_terrain = rngZone.m_terrain;
        if (!tileZone.m_terrain)
            tileZone.m_terrain = tileZone.m_mainTownFaction->nativeTerrain;
        int z = 0;
        if (!z && tileZone.m_terrain->nonUnderground) {
            tileZone.m_terrain = tileZone.m_terrain->nonUnderground;
        }
        tileZone.m_rngZoneSettings = rngZone;
        tileZone.m_id              = key;
        tileZone.m_index           = i;
        tileZone.m_rng             = m_rng;
        tileZone.m_mapCanvas       = &m_mapCanvas;
        tileZone.m_startTile.m_x   = m_rng->genDispersed(rngZone.m_centerAvg.m_x, rngZone.m_centerDispersion.m_x);
        tileZone.m_startTile.m_y   = m_rng->genDispersed(rngZone.m_centerAvg.m_y, rngZone.m_centerDispersion.m_y);
        tileZone.m_relativeArea    = m_rng->genDispersed(rngZone.m_relativeSizeAvg, rngZone.m_relativeSizeDispersion);

        m_totalRelativeArea += tileZone.m_relativeArea;
        if (tileZone.m_relativeArea <= 0)
            throw std::runtime_error("Zone: " + key + " has nonpositive relative size");
        i++;
    }

    m_stopAfter = stringToStage(stopAfterStage);

    for (Stage stage : { Stage::ZoneCenterPlacement,
                         Stage::ZoneTilesInitial,
                         Stage::ZoneTilesExpand,
                         Stage::ZoneTilesRefinement,
                         Stage::TownsPlacement,
                         Stage::BorderRoads,
                         Stage::RoadsPlacement,
                         Stage::Rewards,
                         Stage::Obstacles,
                         Stage::Guards,
                         Stage::PlayerInfo }) {
        m_currentStage = stage;

        Mernel::ScopeTimer timer;
        m_logOutput << baseIndent << "Start stage: " << stageToString(m_currentStage) << "\n";
        runCurrentStage();
        m_logOutput << baseIndent << "End stage: " << stageToString(m_currentStage) << " (" << timer.elapsed() << " us.)\n";
        if (m_currentStage == m_stopAfter) {
            m_logOutput << baseIndent << "stopping further generation, as 'stopAfter' was provided.\n";
            break;
        }
    }
    placeTerrainZones();
    placeDebugInfo();
}

void FHTemplateProcessor::runCurrentStage()
{
    switch (m_currentStage) {
        case Stage::Invalid:
            throw std::runtime_error("Invalid stage, that shouldn't happen");
        case Stage::ZoneCenterPlacement:
            return runZoneCenterPlacement();
        case Stage::ZoneTilesInitial:
            return runZoneTilesInitial();
        case Stage::ZoneTilesExpand:
            return runZoneTilesExpand();
        case Stage::ZoneTilesRefinement:
            return runZoneTilesRefinement();
        case Stage::TownsPlacement:
            return runTownsPlacement();
        case Stage::BorderRoads:
            return runBorderRoads();
        case Stage::RoadsPlacement:
            return runRoadsPlacement();
        case Stage::Rewards:
            return runRewards();
        case Stage::Obstacles:
            return runObstacles();
        case Stage::Guards:
            return runGuards();
        case Stage::PlayerInfo:
            return runPlayerInfo();
    }
}

void FHTemplateProcessor::runZoneCenterPlacement()
{
    const int w = m_map.m_tileMap.m_width;
    const int h = m_map.m_tileMap.m_height;

    if (m_map.m_rngOptions.m_allowFlip) {
        bool vertical   = m_rng->genSmall(1) == 1;
        bool horizontal = m_rng->genSmall(1) == 1;
        for (auto& tileZone : m_tileZones) {
            if (horizontal)
                tileZone.m_startTile.m_x = w - tileZone.m_startTile.m_x - 1;
            if (vertical)
                tileZone.m_startTile.m_y = h - tileZone.m_startTile.m_y - 1;
        }
    }
    if (m_map.m_rngOptions.m_rotationDegreeDispersion) {
        const int rotationDegree = m_rng->genDispersed(0, m_map.m_rngOptions.m_rotationDegreeDispersion);
        m_logOutput << m_indent << "starting rotation of zones to " << rotationDegree << " degrees\n";
        for (auto& tileZone : m_tileZones) {
            auto newPos          = rotateChebyshev(tileZone.m_startTile, rotationDegree, w, h);
            tileZone.m_startTile = newPos;
        }
    }
    if (!m_totalRelativeArea)
        throw std::runtime_error("Total relative area can't be zero");
}

void FHTemplateProcessor::runZoneTilesInitial()
{
    const int w = m_map.m_tileMap.m_width;
    const int h = m_map.m_tileMap.m_height;

    const int64_t area = w * h;

    for (auto& tileZone : m_tileZones) {
        const int greedyPercent   = 90;
        tileZone.m_absoluteArea   = tileZone.m_relativeArea * area * greedyPercent / m_totalRelativeArea / 100;
        tileZone.m_absoluteRadius = static_cast<int64_t>(sqrt(tileZone.m_absoluteArea) / M_PI);

        m_logOutput << m_indent << "zone [" << tileZone.m_id << "] area=" << tileZone.m_absoluteArea
                    << ", radius=" << tileZone.m_absoluteRadius
                    << ", townFaction=" << tileZone.m_mainTownFaction->id
                    << ", rewardFaction=" << tileZone.m_rewardsFaction->id
                    << ", terrain=" << tileZone.m_terrain->id
                    << "\n";
    }

    KMeansSegmentation seg;

    std::map<KMeansSegmentation::Point, size_t> zonePoints;
    for (auto& tileZone : m_tileZones) {
        KMeansSegmentation::Point p{ tileZone.m_startTile };
        zonePoints[p] = tileZone.m_index;
    }

    //std::vector<KMeansSegmentation::Point> points;
    std::vector<size_t> kIndexes(m_tileZones.size());
    seg.m_points.reserve(w * h);
    int z = 0;
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            KMeansSegmentation::Point p(x, y, z);
            if (zonePoints.contains(p)) {
                kIndexes[zonePoints[p]] = seg.m_points.size();
            }
            seg.m_points.push_back(p);
        }
    }
    seg.initClustersByCentroids(kIndexes);
    seg.m_iters = 2;
    for (KMeansSegmentation::Cluster& cluster : seg.m_clusters) {
        size_t idx       = cluster.m_index;
        auto&  tileZone  = m_tileZones[idx];
        cluster.m_radius = tileZone.m_absoluteRadius;
    }

    seg.run(m_logOutput);

    for (KMeansSegmentation::Cluster& cluster : seg.m_clusters) {
        size_t idx          = cluster.m_index;
        auto&  tileZone     = m_tileZones[idx];
        auto   centroid     = FHPos{ .m_x = cluster.getCentroid().m_x, .m_y = cluster.getCentroid().m_y };
        tileZone.m_centroid = centroid;
        for (KMeansSegmentation::Point* point : cluster.m_points) {
            auto p                                = point->toPos();
            m_mapCanvas.m_tileIndex.at(p)->m_zone = &m_tileZones[idx];
        }
    }
    for (auto& tileZone : m_tileZones) {
        tileZone.readFromMap();
    }
}

void FHTemplateProcessor::runZoneTilesExpand()
{
    m_tileZonesPtrs.resize(m_tileZones.size());
    for (size_t i = 0; i < m_tileZonesPtrs.size(); ++i)
        m_tileZonesPtrs[i] = &m_tileZones[i];

    auto fillDeficitIteraction = [this](int thresholdPercent) -> bool {
        std::sort(m_tileZonesPtrs.begin(), m_tileZonesPtrs.end(), [](TileZone* l, TileZone* r) {
            return l->getAreaDeficitPercent() > r->getAreaDeficitPercent();
        });
        TileZone* zoneF = m_tileZonesPtrs.front();
        TileZone* zoneH = m_tileZonesPtrs.back();
        if (zoneF->getAreaDeficitPercent() < thresholdPercent)
            return false;

        for (TileZone* zone : m_tileZonesPtrs) {
            zone->fillDeficit(thresholdPercent, zoneH == zoneF ? nullptr : zoneH);

            for (auto& tileZone : m_tileZones) {
                tileZone.readFromMap();
                tileZone.estimateCentroid();
            }
        }

        for (auto& tileZone : m_tileZones) {
            m_logOutput << m_indent << "(after optimize " << thresholdPercent << "%) zone [" << tileZone.m_id << "] areaDeficit=" << tileZone.getAreaDeficit() << "\n";
        }
        return true;
    };

    for (auto& tileZone : m_tileZones) {
        m_logOutput << m_indent << "(before optimize) zone [" << tileZone.m_id << "] areaDeficit=" << tileZone.getAreaDeficit() << "\n";
    }
    for (int i = 0; i < 5; ++i) {
        if (fillDeficitIteraction(10))
            break;
    }
}

void FHTemplateProcessor::runZoneTilesRefinement()
{
    for (auto& tileZone : m_tileZones) {
        tileZone.readFromMap();
    }
    for (auto& tileZone : m_tileZones) {
        tileZone.fillUnzoned();
    }

    auto checkUnzoned = [this]() {
        bool result = true;
        for (auto& tileZone : m_tileZones) {
            for (auto* cell : tileZone.m_innerArea) {
                if (cell->m_zone != &tileZone) {
                    auto zoneStr = cell->m_zone ? std::to_string(tileZone.m_index) : std::string("NULL");
                    m_logOutput << m_indent << "Invalid zone cell:" << cell->posStr()
                                << ", it placed in [" << tileZone.m_index << "] zone, but cell has [" << zoneStr << "] zone\n";
                    result = false;
                }
            }
        }
        for (auto& cell : m_mapCanvas.m_tiles) {
            if (!cell.m_zone) {
                m_logOutput << m_indent << "Unzoned cell:" << cell.posStr() << "\n";
                result = false;
            }
        }
        if (!result) {
            throw std::runtime_error("All tiles must be zoned!");
        }
    };

    for (int i = 0, limit = 10; i <= limit; ++i) {
        if (m_mapCanvas.fixExclaves()) {
            m_logOutput << m_indent << "exclaves fixed on [" << i << "] iteration\n";
            break;
        }
        if (i == limit) {
            throw std::runtime_error("failed to fix all exclaves after [" + std::to_string(i) + "]  iterations!");
        }
    }
    for (auto& tileZone : m_tileZones) {
        tileZone.readFromMapIfDirty();
    }
    checkUnzoned();

    std::set<MapCanvas::Tile*> placed;
    for (auto& tileZone : m_tileZones) {
        placed.insert(tileZone.m_innerArea.cbegin(), tileZone.m_innerArea.cend());
    }

    m_mapCanvas.checkAllTerrains(placed);
    for (auto& tileZone : m_tileZones) {
        tileZone.estimateCentroid();
    }

    m_map.m_tileMapUpdateRequired = true;
}

void FHTemplateProcessor::runTownsPlacement()
{
    auto placeTown = [this](FHTown town, FHPos pos, FHPlayerId player, Core::LibraryFactionConstPtr faction) {
        if (!faction)
            faction = getRandomFaction(false);
        town.m_factionId = faction;

        pos.m_x += 2;
        town.m_pos    = pos;
        town.m_player = player;
        if (town.m_hasFort)
            town.m_defIndex.variant = "FORT";
        if (town.m_isMain && m_playerInfo.contains(player)) {
            if (m_playerInfo[player].m_hasMainTown)
                throw std::runtime_error("Multiple main towns is not allowed");
            m_playerInfo[player].m_hasMainTown      = true;
            m_playerInfo[player].m_mainTownMapIndex = m_map.m_towns.size();
        }
        m_map.m_towns.push_back(town);
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 3; y++) {
                auto townTilePos = pos;
                townTilePos.m_x -= x;
                townTilePos.m_y -= y;
                if (m_mapCanvas.m_tileIndex.contains(townTilePos))
                    m_mapCanvas.m_blocked.insert(m_mapCanvas.m_tileIndex[townTilePos]);
            }
        }
    };

    for (auto& tileZone : m_tileZones) {
        std::vector<FHPos> townPositions;
        const auto&        towns = tileZone.m_rngZoneSettings.m_towns;
        if (towns.size() == 0) {
            continue;
        } else if (towns.size() == 1) {
            townPositions.push_back(tileZone.m_centroid);
        } else {
            KMeansSegmentation seg;
            seg.m_points.reserve(tileZone.m_innerArea.size());
            for (auto* cell : tileZone.m_innerArea)
                seg.m_points.push_back({ cell->m_pos });

            seg.initRandomClusterCentoids(towns.size(), m_rng);
            seg.run(m_logOutput);

            for (KMeansSegmentation::Cluster& cluster : seg.m_clusters) {
                townPositions.push_back(cluster.getCentroid().toPos());
            }
        }

        for (size_t i = 0; i < towns.size(); ++i) {
            auto player  = towns[i].m_playerControlled ? tileZone.m_rngZoneSettings.m_player : FHPlayerId::None;
            auto faction = towns[i].m_useZoneFaction ? tileZone.m_mainTownFaction : nullptr;
            placeTown(towns[i].m_town, townPositions[i], player, faction);
        }

        for (auto&& pos : townPositions) {
            auto pos2 = pos;
            pos2.m_y++;
            tileZone.m_roadNodes.insert(m_mapCanvas.m_tileIndex.at(pos2));
            tileZone.m_roadNodesTowns.insert(m_mapCanvas.m_tileIndex.at(pos2));
            placeRoad({ pos, pos2 });
        }
    }
}

void FHTemplateProcessor::runBorderRoads()
{
    std::map<std::pair<TileZone*, TileZone*>, TileZone::TileRegion> borderTiles;
    auto                                                            makeKey = [](TileZone& f, TileZone& s) {
        std::pair key{ &f, &s };
        if (f.m_index > s.m_index)
            key = std::pair{ &s, &f };
        return key;
    };

    for (auto& tileZoneFirst : m_tileZones) {
        for (auto& tileZoneSecond : m_tileZones) {
            auto key = makeKey(tileZoneFirst, tileZoneSecond);
            if (borderTiles.contains(key))
                continue;
            TileZone::TileRegion twoSideBorder;
            for (auto* cell : tileZoneFirst.m_outsideEdge) {
                if (cell->m_zone == &tileZoneSecond)
                    twoSideBorder.insert(cell);
            }
            for (auto* cell : tileZoneSecond.m_outsideEdge) {
                if (cell->m_zone == &tileZoneFirst)
                    twoSideBorder.insert(cell);
            }
            borderTiles[key] = twoSideBorder;
        }
    }

    TileZone::TileRegion connectionUnblockableCells;

    for (const auto& [connectionId, connections] : m_map.m_rngConnections) {
        auto&                 tileZoneFrom = findZoneById(connections.m_from);
        auto&                 tileZoneTo   = findZoneById(connections.m_to);
        auto                  key          = makeKey(tileZoneFrom, tileZoneTo);
        TileZone::TileRegion& border       = borderTiles[key];
        if (border.empty()) {
            throw std::runtime_error("No border between '" + connections.m_from + "' and '" + connections.m_to + "'");
        }
        FHPos borderCentroid = TileZone::makeCentroid(border); // switch to k-means when we need more than one connection.

        auto             it   = std::min_element(border.cbegin(), border.cend(), [&borderCentroid](MapCanvas::Tile* l, MapCanvas::Tile* r) {
            return posDistance(borderCentroid, l->m_pos) < posDistance(borderCentroid, r->m_pos);
        });
        MapCanvas::Tile* cell = (*it);
        cell->m_zone->m_roadNodes.insert(cell);

        if (connections.m_guard || !connections.m_mirrorGuard.empty()) {
            Guard guard;
            guard.m_id           = connectionId;
            guard.m_value        = connections.m_guard;
            guard.m_mirrorFromId = connections.m_mirrorGuard;
            guard.m_pos          = cell->m_pos;
            guard.m_zone         = &tileZoneFrom;
            m_guards.push_back(guard);
        }
        MapCanvas::Tile* ncellFound = nullptr;

        for (MapCanvas::Tile* ncell : cell->m_allNeighbours) {
            if (!ncellFound && ncell && ncell->m_zone != cell->m_zone) {
                ncell->m_zone->m_roadNodes.insert(ncell);
                ncellFound = ncell;
            }
        }
        assert(ncellFound);
        border.erase(cell);
        border.erase(ncellFound);
        border.erase(cell->m_neighborT);
        border.erase(ncellFound->m_neighborT);
        border.erase(cell->m_neighborL);
        border.erase(ncellFound->m_neighborL);
        connectionUnblockableCells.insert(cell);
    }
    TileZone::TileRegion noExpandTiles;
    for (MapCanvas::Tile& tile : m_mapCanvas.m_tiles) {
        for (auto* cell : connectionUnblockableCells) {
            if (posDistance(tile.m_pos, cell->m_pos) < 4)
                noExpandTiles.insert(&tile);
        }
    }
    for (const auto& [key, border] : borderTiles) {
        m_mapCanvas.m_needBeBlocked.insert(border.cbegin(), border.cend());
    }
    for (const auto& [key, border] : borderTiles) {
        for (auto* cell : border) {
            for (MapCanvas::Tile* ncell : cell->m_allNeighbours) {
                if (m_mapCanvas.m_needBeBlocked.contains(ncell))
                    continue;
                if (noExpandTiles.contains(ncell))
                    continue;
                m_mapCanvas.m_tentativeBlocked.insert(ncell);
            }
        }
    }
}

void FHTemplateProcessor::runRoadsPlacement()
{
    for (auto& tileZone : m_tileZones) {
        if (tileZone.m_rngZoneSettings.m_cornerRoads && tileZone.m_absoluteArea >= 100) {
            const int k = tileZone.m_absoluteArea / 50;

            KMeansSegmentation seg;
            seg.m_points.reserve(tileZone.m_innerArea.size());
            for (auto* cell : tileZone.m_innerArea) {
                if (m_mapCanvas.m_blocked.contains(cell) || m_mapCanvas.m_needBeBlocked.contains(cell) || m_mapCanvas.m_tentativeBlocked.contains(cell))
                    continue;
                seg.m_points.push_back({ cell->m_pos });
            }

            seg.initRandomClusterCentoids(k, m_rng);
            seg.run(m_logOutput);

            KMeansSegmentation seg2;
            seg2.m_points.reserve(k);

            for (KMeansSegmentation::Cluster& cluster : seg.m_clusters) {
                seg2.m_points.push_back(cluster.getCentroid());
            }

            seg2.initRandomClusterCentoids(tileZone.m_rngZoneSettings.m_cornerRoads, m_rng);
            seg2.run(m_logOutput);

            for (KMeansSegmentation::Cluster& cluster : seg2.m_clusters) {
                auto  it  = std::max_element(cluster.m_points.cbegin(), cluster.m_points.cend(), [&tileZone](KMeansSegmentation::Point* l, KMeansSegmentation::Point* r) {
                    return posDistance(tileZone.m_centroid, l->toPos()) < posDistance(tileZone.m_centroid, r->toPos());
                });
                FHPos pos = (*it)->toPos();
                tileZone.m_roadNodes.insert(m_mapCanvas.m_tileIndex.at(pos));
            }
        }

        std::vector<MapCanvas::Tile*> unconnectedRoadNodes(tileZone.m_roadNodes.cbegin(), tileZone.m_roadNodes.cend());
        if (unconnectedRoadNodes.size() <= 1)
            continue;

        std::sort(unconnectedRoadNodes.begin(), unconnectedRoadNodes.end(), [&tileZone](MapCanvas::Tile* l, MapCanvas::Tile* r) {
            const bool ltown = tileZone.m_roadNodesTowns.contains(l);
            const bool rtown = tileZone.m_roadNodesTowns.contains(r);
            if (ltown != rtown) {
                if (ltown && !rtown)
                    return false;
                if (!ltown && rtown)
                    return true;
            }

            return l->m_pos < r->m_pos;
        });

        std::list<MapCanvas::Tile*> connected;
        {
            MapCanvas::Tile* cell = unconnectedRoadNodes.back();
            unconnectedRoadNodes.pop_back();
            connected.push_back(cell);
        }

        while (!unconnectedRoadNodes.empty()) {
            MapCanvas::Tile* cell = unconnectedRoadNodes.back();
            unconnectedRoadNodes.pop_back();

            auto             it       = std::min_element(connected.cbegin(), connected.cend(), [cell](MapCanvas::Tile* l, MapCanvas::Tile* r) {
                return posDistance(cell->m_pos, l->m_pos) < posDistance(cell->m_pos, r->m_pos);
            });
            MapCanvas::Tile* closest  = *it;
            auto             path     = aStarPath(cell, closest);
            const auto       pathCopy = path;
            for (size_t i = 1; i < pathCopy.size(); i++) {
                FHPos prev = pathCopy[i - 1];
                FHPos cur  = pathCopy[i];
                if (i % 10 == 0 && i < pathCopy.size() - 10) {
                    connected.push_back(m_mapCanvas.m_tileIndex.at(cur));
                }
                if (prev.m_x != cur.m_x && prev.m_y != cur.m_y) // diagonal
                {
                    FHPos extra = prev;

                    if (prev.m_x < cur.m_x && prev.m_y < cur.m_y)
                        extra.m_y = cur.m_y;

                    if (prev.m_x < cur.m_x && prev.m_y > cur.m_y)
                        extra.m_x = cur.m_x;

                    if (prev.m_x > cur.m_x && prev.m_y < cur.m_y)
                        extra.m_y = cur.m_y;

                    if (prev.m_x > cur.m_x && prev.m_y > cur.m_y)
                        extra.m_x = cur.m_x;

                    path.push_back(extra);
                }
            }

            connected.push_back(cell);

            placeRoad(std::move(path));
        }
    }
}

void FHTemplateProcessor::runRewards()
{
    for (auto& tileZone : m_tileZones) {
        if (tileZone.m_rngZoneSettings.m_score.m_guarded.empty())
            continue;
        ObjectGenerator gen(m_map, m_database, m_rng, m_logOutput);
        gen.generate(tileZone.m_rngZoneSettings.m_score);

        std::vector<FHPos> cells;
        for (auto* cell : tileZone.m_innerArea)
            if (cell->m_pos.m_x > 2)
                cells.push_back({ cell->m_pos });
        std::sort(cells.begin(), cells.end());

        for (auto ptr : gen.m_objects) {
            FHPos pos = cells[m_rng->gen(cells.size() - 1)];
            ptr->setPos(pos);
            ptr->place();
            auto guardValue = ptr->getGuard();
            {
                Guard guard;
                guard.m_value = guardValue;
                guard.m_pos   = pos;
                guard.m_pos.m_y++;
                guard.m_zone = &m_tileZones[0];
                m_guards.push_back(guard);
            }
            //m_logOutput << "place '" << ptr->getId() << "' at " << pos.toPrintableString() << '\n';
        }
        break; // @todo:
    }
}

void FHTemplateProcessor::runObstacles()
{
    ObstacleIndex obstacleIndex;
    using Type = Core::LibraryMapObstacle::Type;
    const std::set<Type> suitableObjTypes{
        Type::BRUSH,
        Type::BUSH,
        Type::CACTUS,
        Type::CANYON,
        Type::CRATER,
        Type::HILL,

        Type::LAKE,
        Type::LAVA_FLOW,
        Type::LAVA_LAKE,
        Type::MANDRAKE,
        Type::MOUNTAIN,
        Type::OAK_TREES,
        Type::PINE_TREES,

        Type::ROCK,
        Type::SAND_DUNE,
        Type::SAND_PIT,
        Type::SHRUB,
        Type::STALAGMITE,
        Type::STUMP,
        Type::TAR_PIT,
        Type::TREES,
        Type::VOLCANIC_VENT,
        Type::VOLCANO,
        Type::WILLOW_TREES,
        Type::YUCCA_TREES,

        Type::DESERT_HILLS,
        Type::DIRT_HILLS,
        Type::GRASS_HILLS,
        Type::ROUGH_HILLS,

        Type::SUBTERRANEAN_ROCKS,
        Type::SWAMP_FOLIAGE,
    };

    for (auto* record : m_database->mapObstacles()->records()) {
        if (!suitableObjTypes.contains(record->type))
            continue;
        obstacleIndex.add(record);
    }
    obstacleIndex.doSort();

    Core::LibraryObjectDef::PlanarMask mapMask;
    mapMask.width  = m_map.m_tileMap.m_width;
    mapMask.height = m_map.m_tileMap.m_height;
    mapMask.data.resize(mapMask.height);
    for (auto& row : mapMask.data)
        row.resize(mapMask.width);

    for (MapCanvas::Tile* cell : m_mapCanvas.m_needBeBlocked)
        mapMask.data[cell->m_pos.m_y][cell->m_pos.m_x] = 1;
    for (MapCanvas::Tile* cell : m_mapCanvas.m_tentativeBlocked) {
        mapMask.data[cell->m_pos.m_y][cell->m_pos.m_x] = 2;

        //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = 0, .m_valueB = 2 });
    }

    const size_t maxMaskLookupWidth  = 8;
    const size_t maxMaskLookupHeight = 6;

    /*
    for (size_t y = 0; y < mapMask.height; ++y) {
        for (size_t x = 0; x < mapMask.width; ++x) {
            if (mapMask.data[y][x] == 0)
                continue;
            FHPos pos{ (int) x, (int) y, 0 };
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 0 });
        }
    }*/

    for (size_t y = 0; y < mapMask.height; ++y) {
        Core::LibraryMapObstacleConstPtr prev = nullptr;
        for (size_t x = 0; x < mapMask.width; ++x) {
            //if (mapMask.data[y][x] == 0)
            //    continue;
            if (obstacleIndex.isEmpty(mapMask, x, y, maxMaskLookupWidth, maxMaskLookupHeight))
                continue;
            std::vector<const ObstacleBucket*> buckets = obstacleIndex.find(mapMask, x, y);
            if (buckets.empty())
                continue;
            assert(!buckets.empty());
            int         z = 0;
            const FHPos mapPos{ (int) x, (int) y, z };
            //buckets = obstacleIndex.find(mapMask, x, y);

            //const ObstacleBucket* firstBucket = buckets.front();

            std::vector<Core::LibraryMapObstacleConstPtr> suitable;
            for (const ObstacleBucket* bucket : buckets) {
                for (auto* obst : bucket->m_objects) {
                    if (obst == prev)
                        continue;
                    auto* def = obst->objectDefs.get({}); // @todo: substitutions?

                    FHPos objPos = mapPos;
                    objPos.m_x += def->blockMapPlanar.width - 1;
                    objPos.m_y += def->blockMapPlanar.height - 1;
                    if (!m_mapCanvas.m_tileIndex.contains(objPos))
                        continue;

                    Core::LibraryTerrainConstPtr requiredTerrain = m_mapCanvas.m_tileIndex.at(objPos)->m_zone->m_terrain;

                    if (def->terrainsSoftCache.contains(requiredTerrain))
                        suitable.push_back(obst);
                }
            }
            //{
            //    FHPos pos{ (int) x, (int) y, 0 };
            //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 2 });
            //}
            if (suitable.empty())
                continue;

            //const auto                       bucketSize = firstBucket->m_objects.size();
            Core::LibraryMapObstacleConstPtr obst = suitable[m_rng->gen(suitable.size() - 1)];
            assert(obst);

            prev = obst;

            auto* def = obst->objectDefs.get({});
            assert(def);

            for (size_t my = 0; my < def->blockMapPlanar.height; ++my) {
                for (size_t mx = 0; mx < def->blockMapPlanar.width; ++mx) {
                    if (def->blockMapPlanar.data[my][mx] == 0)
                        continue;
                    size_t px = x + mx;
                    size_t py = y + my;
                    FHPos  maskBitPos{ (int) px, (int) py, 0 };
                    if (py < mapMask.height && px < mapMask.width) {
                        if (mapMask.data[py][px] == 1)
                            mapMask.data[py][px] = 2;
                        auto* cell = m_mapCanvas.m_tileIndex.at(maskBitPos);
                        m_mapCanvas.m_needBeBlocked.erase(cell);
                        m_mapCanvas.m_tentativeBlocked.erase(cell);
                        m_mapCanvas.m_blocked.insert(cell);
                    }
                    //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 3 });
                }
            }

            FHPos objPos = mapPos;
            objPos.m_x += def->blockMapPlanar.width - 1;
            objPos.m_y += def->blockMapPlanar.height - 1;

            FHObstacle fhOb;
            fhOb.m_id  = obst;
            fhOb.m_pos = objPos;
            m_map.m_objects.m_obstacles.push_back(std::move(fhOb));

            //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = 0, .m_valueB = 1 }); // red

            //if (cnt-- <= 0)
            //    return;
        }
    }

    for (auto* tile : m_mapCanvas.m_needBeBlocked) {
        m_logOutput << m_indent << "still require to be blocked: " << tile->posStr() << "\n";
    }
    m_mapCanvas.m_tentativeBlocked.clear();
    if (!m_mapCanvas.m_needBeBlocked.empty()) {
        throw std::runtime_error("Some block tiles are not set.");
    }
}

void FHTemplateProcessor::runGuards()
{
    auto getPossibleCount = [](Core::LibraryUnitConstPtr unit, int64_t value) {
        auto possibleCount = value / unit->value;
        if (possibleCount <= 1)
            return possibleCount;
        int64_t unitValue = unit->value;
        int64_t coef1     = 100;
        int64_t coef100   = 0;
        if (possibleCount >= 100) {
            coef1   = 0;
            coef100 = 100;
        } else {
            coef1   = 100 - possibleCount + 1;
            coef100 = possibleCount - 1;
        }

        unitValue = (coef1 * unit->guardMult1 + coef100 * unit->guardMult100) * unitValue / 10000;

        possibleCount = value / unitValue;
        return possibleCount;
    };

    auto makeCandidates = [this, &getPossibleCount](int64_t value) {
        std::map<int, int> unitLevelScore;

        for (Core::LibraryUnitConstPtr unit : m_guardUnits) {
            auto possibleCount = getPossibleCount(unit, value);
            if (possibleCount < 5)
                continue;
            int score = 0;
            if (possibleCount < 10)
                score = 1;
            else if (possibleCount < 25)
                score = 2;
            else if (possibleCount < 35)
                score = 5;
            else if (possibleCount < 50)
                score = 3;
            else if (possibleCount < 100)
                score = 2;
            else
                score = 1;
            unitLevelScore[unit->level / 10] += score;
        }
        auto      it                = std::max_element(unitLevelScore.begin(), unitLevelScore.end(), [](auto l, auto r) { return l.second < r.second; });
        const int optimalGuardLevel = it->first;

        std::vector<Core::LibraryUnitConstPtr> result;
        for (Core::LibraryUnitConstPtr unit : m_guardUnits) {
            auto possibleCount = getPossibleCount(unit, value);
            if (possibleCount < 5)
                continue;
            const int levelDiff = (unit->level / 10) - optimalGuardLevel;
            if (levelDiff < -1 || levelDiff > 2)
                continue;

            if (unit->level < 80 && possibleCount >= 100)
                continue;
            result.push_back(unit);
        }

        return result;
    };

    std::map<std::string, size_t> nameIndex;

    for (auto& guard : m_guards) {
        if (guard.m_value == 0)
            continue;

        std::vector<Core::LibraryUnitConstPtr> candidates = makeCandidates(guard.m_value);

        if (candidates.empty()) { // value is so low, nobody can guard so low value at least in group of 3
            continue;
        }
        Core::LibraryUnitConstPtr unit = candidates[m_rng->gen(candidates.size() - 1)];

        auto       finalValue = m_rng->genDispersed(guard.m_value, guard.m_valueDispersion);
        const bool upgraded   = unit->upgrades.empty() ? false : m_rng->genSmall(3) == 0;

        FHMonster fhMonster;
        fhMonster.m_pos        = guard.m_pos;
        fhMonster.m_count      = getPossibleCount(unit, finalValue);
        fhMonster.m_id         = unit;
        fhMonster.m_guardValue = finalValue;

        if (upgraded) {
            auto upCount = getPossibleCount(unit->upgrades[0], finalValue);
            // let's say upgraded stack is 1/4 of stacks. Then recalc count taking an account 1/4 of value is upped.
            fhMonster.m_count = fhMonster.m_count * 3 / 4 + upCount * 1 / 4;
        }

        if (guard.m_joinable) {
            fhMonster.m_agressionMin = 4;
            fhMonster.m_agressionMax = 10;
        } else {
            fhMonster.m_agressionMin = 10;
            fhMonster.m_agressionMax = 10;
        }

        fhMonster.m_joinOnlyForMoney = true;
        fhMonster.m_joinPercent      = 50;
        fhMonster.m_upgradedStack    = upgraded ? FHMonster::UpgradedStack::Yes : FHMonster::UpgradedStack::No;

        if (!guard.m_id.empty())
            nameIndex[guard.m_id] = m_map.m_objects.m_monsters.size();

        m_map.m_objects.m_monsters.push_back(std::move(fhMonster));
    }
    for (auto& guard : m_guards) {
        if (guard.m_value == 0 && !guard.m_mirrorFromId.empty()) {
            size_t    mirrorFrom = nameIndex.at(guard.m_mirrorFromId);
            FHMonster fhMonster  = m_map.m_objects.m_monsters[mirrorFrom];
            fhMonster.m_pos      = guard.m_pos;
            m_map.m_objects.m_monsters.push_back(std::move(fhMonster));
        }
    }
}

void FHTemplateProcessor::runPlayerInfo()
{
    for (const auto& [playerId, info] : m_playerInfo) {
        FHPlayer& player                = m_map.m_players[playerId];
        player.m_startingFactions       = { info.m_faction };
        player.m_generateHeroAtMainTown = false;

        if (info.m_hasMainTown && info.m_startingHero) {
            const FHTown& mainTown = m_map.m_towns.at(info.m_mainTownMapIndex);
            FHPos         pos      = mainTown.m_pos;
            pos.m_x -= 2;

            FHHero fhhero;
            fhhero.m_player = playerId;
            fhhero.m_pos    = pos;
            fhhero.m_isMain = true;

            FHHeroData& destHero = fhhero.m_data;
            destHero.m_army.hero = Core::AdventureHero(info.m_startingHero);

            m_map.m_wanderingHeroes.push_back(std::move(fhhero));
        }
        if (info.m_hasMainTown && info.m_extraHero) {
            const FHTown& mainTown = m_map.m_towns.at(info.m_mainTownMapIndex);

            FHPos pos = mainTown.m_pos;
            pos.m_x -= 2;
            pos.m_y += 1;

            FHHero fhhero;
            fhhero.m_player = playerId;
            fhhero.m_pos    = pos;

            FHHeroData& destHero = fhhero.m_data;
            destHero.m_army.hero = Core::AdventureHero(info.m_extraHero);

            m_map.m_wanderingHeroes.push_back(std::move(fhhero));
        }
    }
}

void FHTemplateProcessor::placeTerrainZones()
{
    for (auto& tileZone : m_tileZones) {
        FHZone fhZone;
        fhZone.m_tiles.reserve(tileZone.m_innerArea.size());
        for (auto* cell : tileZone.m_innerArea)
            fhZone.m_tiles.push_back(cell->m_pos);
        fhZone.m_terrainId = tileZone.m_terrain;
        assert(fhZone.m_terrainId);
        m_map.m_zones.push_back(std::move(fhZone));
    }
}

void FHTemplateProcessor::placeDebugInfo()
{
    if (m_stopAfter == Stage::Invalid)
        return;
    for (auto& tileZone : m_tileZones) {
        if (m_stopAfter <= Stage::TownsPlacement) {
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_startTile, .m_valueA = tileZone.m_index, .m_valueB = 1 }); // red
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_centroid, .m_valueA = tileZone.m_index, .m_valueB = 3 });
        }

        if (m_stopAfter <= Stage::ZoneTilesRefinement) {
            for (auto* cell : tileZone.m_innerEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 2 });
            }
            for (auto* cell : tileZone.m_outsideEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
            }
        }

        if (m_stopAfter <= Stage::RoadsPlacement) {
            for (auto* cell : tileZone.m_roadNodes) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 1 });
            }
        }
    }
}

void FHTemplateProcessor::placeRoad(std::vector<FHPos> path)
{
    if (path.empty())
        return;
    FHRoad road;
    road.m_type  = FHRoadType::Cobblestone;
    road.m_tiles = std::move(path);
    m_map.m_roads.push_back(std::move(road));
}

Core::LibraryFactionConstPtr FHTemplateProcessor::getRandomFaction(bool rewardOnly)
{
    auto& factions = rewardOnly ? m_rewardFactions : m_playableFactions;
    return factions[m_rng->genSmall(factions.size() - 1)];
}

Core::LibraryHeroConstPtr FHTemplateProcessor::getRandomHero(Core::LibraryFactionConstPtr faction)
{
    std::vector<Core::LibraryHeroConstPtr> heroes;
    if (faction) {
        for (auto* hero : faction->heroes) {
            if (!m_heroPool.contains(hero))
                continue;
            heroes.push_back(hero);
        }
    } else {
        heroes = std::vector<Core::LibraryHeroConstPtr>(m_heroPool.cbegin(), m_heroPool.cend());
    }

    if (heroes.empty())
        return nullptr;

    auto* hero = heroes[m_rng->gen(heroes.size() - 1)];
    m_heroPool.erase(hero);
    return hero;
}

TileZone& FHTemplateProcessor::findZoneById(const std::string& id)
{
    auto it = std::find_if(m_tileZones.begin(), m_tileZones.end(), [&id](const TileZone& zone) { return zone.m_id == id; });
    if (it == m_tileZones.end())
        throw std::runtime_error("Invalid zone id:" + id);
    return *it;
}

std::vector<FHPos> FHTemplateProcessor::aStarPath(MapCanvas::Tile* start, MapCanvas::Tile* end)
{
    const int w = m_map.m_tileMap.m_width;
    const int h = m_map.m_tileMap.m_height;

    AstarGenerator generator;
    generator.setWorldSize({ w, h });

    for (MapCanvas::Tile* blocked : m_mapCanvas.m_blocked)
        generator.addCollision({ blocked->m_pos.m_x, blocked->m_pos.m_y });

    auto result = generator.findPath({ start->m_pos.m_x, start->m_pos.m_y }, { end->m_pos.m_x, end->m_pos.m_y });

    return result;
}

}
