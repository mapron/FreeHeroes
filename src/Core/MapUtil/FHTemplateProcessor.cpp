/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTemplateProcessor.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "LibraryDwelling.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryPlayer.hpp"

#include "RmgUtil/KMeans.hpp"
#include "RmgUtil/ObjectBundle.hpp"
#include "RmgUtil/ObjectGenerator.hpp"
#include "RmgUtil/ObstacleHelper.hpp"
#include "RmgUtil/TemplateUtils.hpp"
#include "RmgUtil/RoadHelper.hpp"

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
    BorderRoads,
    TownsPlacement,
    RoadsPlacement,
    Rewards,
    CorrectObjectTerrains,
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

    const int regionCount = m_map.m_template.m_zones.size();
    if (regionCount <= 1)
        throw std::runtime_error("need at least two zones");

    m_logOutput << baseIndent << "Start generating map (seed=" << m_map.m_seed << ") " << m_map.m_tileMap.m_width << "x" << m_map.m_tileMap.m_height
                << " " << (m_map.m_tileMap.m_depth == 2 ? "+U" : "no U") << "\n";

    m_tileContainer.init(m_map.m_tileMap.m_width, m_map.m_tileMap.m_height, m_map.m_tileMap.m_depth);

    m_totalRelativeArea = 0;

    for (const auto& [playerId, info] : m_map.m_players) {
        m_playerInfo[playerId] = {};
    }
    for (const auto& [key, rngZone] : m_map.m_template.m_zones) {
        if (rngZone.m_player == nullptr || !rngZone.m_player->isPlayable)
            continue;

        if (!m_playerInfo.contains(rngZone.m_player))
            throw std::runtime_error("You need to define all players used in zones.");
    }
    for (auto& [playerId, info] : m_playerInfo) {
        if (!m_map.m_template.m_userSettings.m_players.contains(playerId))
            continue;
        auto& pl               = m_map.m_template.m_userSettings.m_players[playerId];
        info.m_faction         = pl.m_faction;
        info.m_startingHero    = pl.m_startingHero;
        info.m_extraHero       = pl.m_extraHero;
        info.m_startingHeroGen = pl.m_startingHeroGen;
        info.m_extraHeroGen    = pl.m_extraHeroGen;
        info.m_stdStats        = pl.m_stdStats;
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

    auto factionSetup = [this](TileZone& tileZone) {
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
        if (!tileZone.m_dwellFaction) {
            tileZone.m_dwellFaction = tileZone.m_mainTownFaction;
        }
        assert(tileZone.m_mainTownFaction);
        assert(tileZone.m_rewardsFaction);
        tileZone.m_terrain = tileZone.m_rngZoneSettings.m_terrain;
        if (!tileZone.m_terrain)
            tileZone.m_terrain = tileZone.m_mainTownFaction->nativeTerrain;
        int z = 0;
        if (!z && tileZone.m_terrain->nonUnderground) {
            tileZone.m_terrain = tileZone.m_terrain->nonUnderground;
        }
    };

    m_tileZones.resize(regionCount);
    for (int i = 0; const auto& [key, rngZone] : m_map.m_template.m_zones) {
        auto& tileZone             = m_tileZones[i];
        tileZone.m_player          = rngZone.m_player;
        tileZone.m_mainTownFaction = rngZone.m_mainTownFaction;
        tileZone.m_rewardsFaction  = rngZone.m_rewardsFaction;
        tileZone.m_dwellFaction    = rngZone.m_dwellingFaction;
        tileZone.m_rngZoneSettings = rngZone;
        if (rngZone.m_excludeFactionZones.empty())
            factionSetup(tileZone);

        tileZone.m_id            = key;
        tileZone.m_index         = i;
        tileZone.m_rng           = m_rng;
        tileZone.m_tileContainer = &m_tileContainer;
        FHPos startTile;
        startTile.m_x        = m_rng->genDispersed(rngZone.m_centerAvg.m_x, rngZone.m_centerDispersion.m_x);
        startTile.m_y        = m_rng->genDispersed(rngZone.m_centerAvg.m_y, rngZone.m_centerDispersion.m_y);
        tileZone.m_startTile = m_tileContainer.m_tileIndex.at(startTile);

        tileZone.m_relativeArea = m_rng->genDispersed(rngZone.m_relativeSizeAvg, rngZone.m_relativeSizeDispersion);

        m_totalRelativeArea += tileZone.m_relativeArea;
        if (tileZone.m_relativeArea <= 0)
            throw std::runtime_error("Zone: " + key + " has nonpositive relative size");
        i++;
    }
    for (auto& tileZone : m_tileZones) {
        if (!tileZone.m_rngZoneSettings.m_excludeFactionZones.empty()) {
            tileZone.m_mainTownFaction = getRandomPlayableFaction(tileZone.m_rngZoneSettings.m_excludeFactionZones);
            factionSetup(tileZone);
        }
    }

    m_stopAfter = stringToStage(stopAfterStage);
    //m_stopAfter = Stage::RoadsPlacement;

    Mernel::ProfilerContext                profileContext;
    Mernel::ProfilerDefaultContextSwitcher switcher(profileContext);

    for (Stage stage : { Stage::ZoneCenterPlacement,
                         Stage::ZoneTilesInitial,
                         Stage::ZoneTilesExpand,
                         Stage::ZoneTilesRefinement,
                         Stage::BorderRoads,
                         Stage::TownsPlacement,
                         Stage::RoadsPlacement,
                         Stage::Rewards,
                         Stage::CorrectObjectTerrains,
                         Stage::Obstacles,
                         Stage::Guards,
                         Stage::PlayerInfo }) {
        m_currentStage = stage;

        Mernel::ScopeTimer timer;
        m_logOutput << baseIndent << "Start stage: " << stageToString(m_currentStage) << "\n";
        runCurrentStage();
        m_logOutput << baseIndent << "End stage: " << stageToString(m_currentStage) << " (" << timer.elapsed() << " us.)\n";
        {
            auto profilerStr = profileContext.printToStr();
            profileContext.clearAll();
            if (!profilerStr.empty())
                m_logOutput << baseIndent << "Profiler data:\n"
                            << profilerStr;
        }
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
        case Stage::BorderRoads:
            return runBorderRoads();
        case Stage::TownsPlacement:
            return runTownsPlacement();
        case Stage::RoadsPlacement:
            return runRoadsPlacement();
        case Stage::Rewards:
            return runRewards();
        case Stage::CorrectObjectTerrains:
            return runCorrectObjectTerrains();
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

    if (m_map.m_template.m_allowFlip) {
        bool vertical   = m_rng->genSmall(1) == 1;
        bool horizontal = m_rng->genSmall(1) == 1;
        for (auto& tileZone : m_tileZones) {
            auto newPos = tileZone.m_startTile->m_pos;
            if (horizontal)
                newPos.m_x = w - newPos.m_x - 1;
            if (vertical)
                newPos.m_y = h - newPos.m_y - 1;
            tileZone.m_startTile = m_tileContainer.m_tileIndex.at(newPos);
        }
    }
    if (m_map.m_template.m_rotationDegreeDispersion) {
        const int rotationDegree = m_rng->genDispersed(0, m_map.m_template.m_rotationDegreeDispersion);
        m_logOutput << m_indent << "starting rotation of zones to " << rotationDegree << " degrees\n";
        for (auto& tileZone : m_tileZones) {
            auto newPos          = rotateChebyshev(tileZone.m_startTile->m_pos, rotationDegree, w, h);
            tileZone.m_startTile = m_tileContainer.m_tileIndex.at(newPos);
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
                    << ", startTile=" << tileZone.m_startTile->toPrintableString()
                    << ", townFaction=" << tileZone.m_mainTownFaction->id
                    << ", rewardFaction=" << tileZone.m_rewardsFaction->id
                    << ", terrain=" << tileZone.m_terrain->id
                    << ", zoneGuardPercent=" << tileZone.m_rngZoneSettings.m_zoneGuardPercent
                    << "\n";
    }

    KMeansSegmentation seg;

    std::map<MapTilePtr, size_t> zonePoints;
    for (auto& tileZone : m_tileZones) {
        zonePoints[tileZone.m_startTile] = tileZone.m_index;
    }

    std::vector<size_t> kIndexes(m_tileZones.size());
    seg.m_points.reserve(w * h);
    int z = 0;
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            FHPos pos{ x, y, z };
            auto* tile = m_tileContainer.m_tileIndex.at(pos);
            if (zonePoints.contains(tile)) {
                kIndexes[zonePoints[tile]] = seg.m_points.size();
            }
            seg.m_points.push_back({ tile });
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
        FHPos  centroid     = cluster.m_centroid;
        tileZone.m_centroid = m_tileContainer.m_tileIndex.at(centroid);
        for (KMeansSegmentation::Point* point : cluster.m_points) {
            auto* tile   = point->m_pos;
            tile->m_zone = &tileZone;
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
            for (auto* cell : tileZone.m_area.m_innerArea) {
                if (cell->m_zone != &tileZone) {
                    auto zoneStr = cell->m_zone ? std::to_string(cell->m_zone->m_index) : std::string("NULL");
                    m_logOutput << m_indent << "Invalid zone cell:" << cell->toPrintableString()
                                << ", it placed in [" << tileZone.m_index << "] zone, but cell has [" << zoneStr << "] zone\n";
                    result = false;
                }
            }
        }
        for (auto& cell : m_tileContainer.m_tiles) {
            if (!cell.m_zone) {
                m_logOutput << m_indent << "Unzoned cell:" << cell.toPrintableString() << "\n";
                result = false;
            }
        }
        if (!result) {
            throw std::runtime_error("All tiles must be zoned!");
        }
    };

    for (int i = 0, limit = 10; i <= limit; ++i) {
        if (m_tileContainer.fixExclaves()) {
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

    MapTileRegion placed;
    for (auto& tileZone : m_tileZones) {
        placed.insert(tileZone.m_area.m_innerArea);
    }
    placed.doSort();

    m_tileContainer.checkAllTerrains(placed);
    for (auto& tileZone : m_tileZones) {
        tileZone.estimateCentroid();
    }

    m_map.m_tileMapUpdateRequired = true;
    placeTerrainZones();
}

void FHTemplateProcessor::runBorderRoads()
{
    RoadHelper roadHelper(m_map, m_tileContainer, m_rng, m_logOutput);
    roadHelper.makeBorders(m_tileZones);

    for (auto& guard : roadHelper.m_guards) {
        m_guards.push_back(Guard{
            .m_value        = guard.m_value,
            .m_id           = guard.m_id,
            .m_mirrorFromId = guard.m_mirrorFromId,
            .m_pos          = guard.m_pos,
            .m_zone         = guard.m_zone,
            .m_joinable     = guard.m_joinable,
        });
    }
}

void FHTemplateProcessor::runTownsPlacement()
{
    auto placeTown = [this](FHTown town, FHPos pos, TileZone& tileZone, Core::LibraryPlayerConstPtr player, Core::LibraryFactionConstPtr faction) {
        town.m_factionId = faction;

        pos.m_x += 2;
        town.m_pos    = pos;
        town.m_player = player;
        if (town.m_hasFort)
            town.m_defIndex.variant = "FORT";
        const bool isPlayerControlled = m_playerInfo.contains(player);
        if (town.m_isMain && isPlayerControlled) {
            if (m_playerInfo[player].m_hasMainTown)
                throw std::runtime_error("Multiple main towns is not allowed");
            m_playerInfo[player].m_hasMainTown      = true;
            m_playerInfo[player].m_mainTownMapIndex = m_map.m_towns.size();
        }
        if (!town.m_garisonRmg.empty()) {
            town.m_hasGarison = true;
            for (const auto& [levels, value] : town.m_garisonRmg) {
                Core::LibraryUnitConstPtr unit = nullptr;
                for (auto* funit : faction->units) {
                    if (levels.contains(funit->level))
                        unit = funit;
                }
                assert(unit);
                int count = 0;
                if (isPlayerControlled) {
                    count = value / unit->value;
                } else {
                    count = getPossibleCount(unit, value);
                }
                town.m_garison.push_back({ unit, count });
            }
        }
        m_map.m_towns.push_back(town);
        MapTileArea townArea;
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 3; y++) {
                auto townTilePos = pos;
                townTilePos.m_x -= x;
                townTilePos.m_y -= y;
                if (m_tileContainer.m_tileIndex.contains(townTilePos))
                    townArea.m_innerArea.insert(m_tileContainer.m_tileIndex[townTilePos]);
            }
        }
        townArea.makeEdgeFromInnerArea();
        tileZone.m_blocked.insert(townArea.m_innerArea);
        tileZone.m_innerAreaTowns.insert(townArea.m_innerArea);
        tileZone.m_innerAreaTowns.insert(townArea.m_outsideEdge);
        tileZone.m_innerAreaSegmentsRoads.insert(townArea.m_outsideEdge);
    };

    auto playerNone = m_database->players()->find(std::string(Core::LibraryPlayer::s_none));

    RoadHelper roadHelper(m_map, m_tileContainer, m_rng, m_logOutput);

    for (auto& tileZone : m_tileZones) {
        const auto& towns = tileZone.m_rngZoneSettings.m_towns;
        if (towns.size() == 0)
            continue;

        std::vector<MapTilePtr> townPositions(towns.size());

        auto centroidCell = tileZone.m_centroid;
        for (size_t i = 0; const auto& [_, town] : towns) {
            i++;
            if (town.m_closeToConnection.empty())
                continue;
            auto radius = town.m_tilesToTarget;
            if (radius <= 0)
                continue;

            auto                    connectionTile = tileZone.m_namedTiles.at(town.m_closeToConnection);
            std::vector<MapTilePtr> tilesInRadius;
            for (auto* zoneTile : tileZone.m_innerAreaUsable.m_innerArea) {
                auto distance = posDistance(connectionTile, zoneTile);
                if (distance < radius - 1 || distance > radius + 1)
                    continue;
                tilesInRadius.push_back(zoneTile);
            }

            auto it              = std::min_element(tilesInRadius.cbegin(), tilesInRadius.cend(), [centroidCell](MapTilePtr l, MapTilePtr r) {
                return posDistance(centroidCell, l) < posDistance(centroidCell, r);
            });
            townPositions[i - 1] = *it;
        }

        if (towns.size() == 1) {
            if (!townPositions[0])
                townPositions[0] = tileZone.m_centroid;
        } else {
            KMeansSegmentation seg;
            seg.m_points.reserve(tileZone.m_area.m_innerArea.size());
            for (auto* cell : tileZone.m_area.m_innerArea) {
                seg.m_points.push_back({ cell });
            }

            std::vector<MapTilePtr> townPositionsEst;
            {
                KMeansSegmentation segCopy = seg;
                segCopy.initRandomClusterCentoids(towns.size(), m_rng);
                segCopy.run(m_logOutput);
                for (KMeansSegmentation::Cluster& cluster : segCopy.m_clusters)
                    townPositionsEst.push_back(m_tileContainer.m_tileIndex.at(cluster.m_centroid));
            }

            seg.initRandomClusterCentoids(towns.size(), m_rng);

            std::map<MapTilePtr, size_t> cell2pointindex;
            for (size_t i = 0; i < seg.m_points.size(); ++i) {
                cell2pointindex[seg.m_points[i].m_pos] = i;
            }
            for (size_t i = 0; i < townPositions.size(); ++i) {
                auto* cell = townPositions[i];
                if (!cell)
                    continue;

                const size_t index = cell2pointindex.at(cell);
                seg.initCluster(i, index, true);

                auto it = std::min_element(townPositionsEst.begin(), townPositionsEst.end(), [cell](MapTilePtr l, MapTilePtr r) {
                    return posDistance(cell, l) < posDistance(cell, r);
                });
                townPositionsEst.erase(it);
            }
            for (size_t i = 0; i < townPositions.size(); ++i) {
                if (townPositions[i])
                    continue;
                const size_t index = cell2pointindex.at(townPositionsEst.back());
                townPositionsEst.pop_back();
                seg.initCluster(i, index, false);
            }
            seg.run(m_logOutput);

            for (size_t i = 0; KMeansSegmentation::Cluster & cluster : seg.m_clusters) {
                townPositions[i] = (m_tileContainer.m_tileIndex.at(cluster.m_centroid));
                i++;
            }
        }

        for (size_t i = 0; const auto& [_, town] : towns) {
            auto player  = town.m_playerControlled ? tileZone.m_rngZoneSettings.m_player : playerNone;
            auto faction = town.m_useZoneFaction ? tileZone.m_mainTownFaction : nullptr;
            if (!faction)
                faction = getRandomPlayableFaction(town.m_excludeFactionZones);

            placeTown(town.m_town, townPositions[i]->m_pos, tileZone, player, faction);
            i++;
        }

        for (auto&& pos : townPositions) {
            auto pos2 = pos->m_neighborB;
            tileZone.m_roadNodesHighPriority.insert(pos2);
            roadHelper.placeRoad({ pos, pos2 }, 0);
        }
        tileZone.m_blocked.doSort();
        tileZone.m_innerAreaTowns.doSort();
        {
            MapTileArea areaTowns;
            areaTowns.m_innerArea = tileZone.m_innerAreaTowns;
            areaTowns.makeEdgeFromInnerArea();
            tileZone.m_innerAreaTowns.insert(areaTowns.m_outsideEdge);
            tileZone.m_innerAreaTowns.doSort();
        }
        tileZone.m_innerAreaUsable.m_innerArea.erase(tileZone.m_blocked);
        tileZone.m_innerAreaUsable.makeEdgeFromInnerArea();
    }
}

void FHTemplateProcessor::runRoadsPlacement()
{
    RoadHelper roadHelper(m_map, m_tileContainer, m_rng, m_logOutput);

    for (auto& tileZone : m_tileZones) {
        roadHelper.placeRoads(tileZone);

        //        for (auto* cell : tileZone.m_placedRoads) {
        //            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
        //        }

        //        for (auto* cell : tileZone.m_roadNodes) {
        //            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 2 });
        //        }
        //        for (auto* cell : tileZone.m_roadNodesHighPriority) {
        //            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 1 });
        //        }
        //        for (auto* cell : tileZone.m_roadNodes) {
        //            if (tileZone.m_innerAreaUsable.m_innerEdge.contains(cell))
        //                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 4 });
        //        }
    }
}

void FHTemplateProcessor::runRewards()
{
    m_logOutput << m_indent << "RNG TEST A:" << m_rng->gen(1000000) << "\n";

    const auto&   diffSett    = m_map.m_template.m_userSettings.m_difficulty;
    const int64_t armyPercent = m_rng->genMinMax(diffSett.m_minArmyPercent, diffSett.m_maxArmyPercent);
    const int64_t goldPercent = m_rng->genMinMax(diffSett.m_minGoldPercent, diffSett.m_maxGoldPercent);

    m_logOutput << m_indent << "armyPercent=" << armyPercent << ", goldPercent=" << goldPercent << "\n";

    ObjectBundleSet bundleSet(m_rng, m_tileContainer, m_logOutput);
    for (auto& tileZone : m_tileZones) {
        if (tileZone.m_rngZoneSettings.m_scoreTargets.empty())
            continue;

        //if (tileZone.m_id != "P1")
        //    continue;

        m_logOutput << m_indent << " --- generate : " << tileZone.m_id << " --- \n";
        auto      objects     = m_map.m_objects;
        const int maxAttempts = 3;
        for (int i = 1; i <= maxAttempts; ++i) {
            const bool      lastAttempt = i == maxAttempts;
            ObjectGenerator gen(m_map, m_database, m_rng, m_logOutput);
            gen.generate(tileZone.m_rngZoneSettings,
                         tileZone.m_rewardsFaction,
                         tileZone.m_dwellFaction,
                         tileZone.m_terrain,
                         armyPercent,
                         goldPercent);

            if (!bundleSet.consume(gen, tileZone)) {
                if (lastAttempt)
                    throw std::runtime_error("Failed to fit some objects into zone '" + tileZone.m_id + "'");
                m_map.m_objects = objects; // restore map data and try again.
            } else {
                break;
            }
        }

        //for (auto* cell : bundleSet.m_consumeResult.m_centroidsALL) {
        //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 1 }); // red
        //}
    }

    for (auto& guardBundle : bundleSet.m_guards) {
        Guard guard;
        guard.m_value    = guardBundle.m_value;
        guard.m_pos      = guardBundle.m_pos;
        guard.m_zone     = guardBundle.m_zone;
        guard.m_joinable = true;
        m_guards.push_back(guard);
    }

    m_logOutput << m_indent << "RNG TEST B:" << m_rng->gen(1000000) << "\n";
}

void FHTemplateProcessor::runCorrectObjectTerrains()
{
    auto correctObjIndexPos = [this](const std::string& id, Core::ObjectDefIndex& defIndex, const Core::ObjectDefMappings& defMapping, FHPos pos) {
        Core::LibraryTerrainConstPtr requiredTerrain = m_tileContainer.m_tileIndex.at(pos)->m_zone->m_terrain;
        auto                         old             = defIndex;
        defIndex.substitution                        = "!!";
        defIndex.variant                             = "!!";
        auto result                                  = ObjectGenerator::correctObjIndex(defIndex, defMapping, requiredTerrain);
        if (defIndex.substitution == "!!" || defIndex.variant == "!!")
            throw std::runtime_error("Failed to replace '" + id + "' def ('" + old.variant + "','" + old.substitution + "') for '" + requiredTerrain->id + "'");

        return result;
    };

    for (auto& obj : m_map.m_objects.m_banks)
        correctObjIndexPos(obj.m_id->id, obj.m_defIndex, obj.m_id->objectDefs, obj.m_pos);
    for (auto& obj : m_map.m_objects.m_dwellings)
        correctObjIndexPos(obj.m_id->id, obj.m_defIndex, obj.m_id->objectDefs, obj.m_pos);
    for (auto& obj : m_map.m_objects.m_visitables)
        correctObjIndexPos(obj.m_visitableId->id, obj.m_defIndex, obj.m_visitableId->objectDefs, obj.m_pos);
    for (auto& obj : m_map.m_objects.m_mines)
        correctObjIndexPos(obj.m_id->id, obj.m_defIndex, obj.m_id->minesDefs, obj.m_pos);
    for (auto& obj : m_map.m_objects.m_shrines)
        correctObjIndexPos(obj.m_visitableId->id, obj.m_defIndex, obj.m_visitableId->objectDefs, obj.m_pos);
}

void FHTemplateProcessor::runObstacles()
{
    ObstacleHelper obstacleHelper(m_map, m_tileZones, m_tileContainer, m_rng, m_database, m_logOutput);
    obstacleHelper.placeObstacles(3);

    for (auto& tileZone : m_tileZones) {
        for (auto* tile : tileZone.m_needBeBlocked) {
            m_logOutput << m_indent << "still require to be blocked: " << tile->toPrintableString() << "\n";
        }
        tileZone.m_tentativeBlocked.clear();
        if (!tileZone.m_needBeBlocked.empty()) {
            throw std::runtime_error("Some block tiles are not set.");
        }
    }
}

void FHTemplateProcessor::runGuards()
{
    const auto& diffSett = m_map.m_template.m_userSettings.m_difficulty;
    m_userMultiplyGuard  = m_rng->genMinMax(diffSett.m_minGuardsPercent, diffSett.m_maxGuardsPercent);

    auto makeCandidates = [this](int64_t value) {
        std::map<int, int> unitLevelScore;

        for (Core::LibraryUnitConstPtr unit : m_guardUnits) {
            auto possibleCount = getPossibleCount(unit, value);
            if (possibleCount < 5)
                continue;
            if (possibleCount < 10 && unit->level < 50)
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
            if (possibleCount < 10 && unit->level < 50)
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
        int64_t value = guard.m_value;
        if (value == 0)
            continue;

        value = value * m_userMultiplyGuard / 100;

        if (guard.m_zone) {
            const int64_t dispersionPercent = m_rng->genDispersed(0, guard.m_zone->m_rngZoneSettings.m_zoneGuardDispersion);

            value = value * guard.m_zone->m_rngZoneSettings.m_zoneGuardPercent / 100;
            value += value * dispersionPercent / 100;
        }

        std::vector<Core::LibraryUnitConstPtr> candidates = makeCandidates(value);

        if (candidates.empty()) { // value is so low, nobody can guard so low value at least in group of 5
            continue;
        }
        Core::LibraryUnitConstPtr unit = candidates[m_rng->gen(candidates.size() - 1)];

        const bool upgraded = unit->upgrades.empty() ? false : m_rng->genSmall(3) == 0;

        FHMonster fhMonster;
        fhMonster.m_pos        = guard.m_pos->m_pos;
        fhMonster.m_count      = getPossibleCount(unit, value);
        fhMonster.m_id         = unit;
        fhMonster.m_guardValue = value;

        //fhMonster.m_score[FHScoreAttr::Support] = guard.m_zone ? guard.m_zone->m_rngZoneSettings.m_zoneGuardPercent : 666;

        if (upgraded) {
            auto upCount = getPossibleCount(unit->upgrades[0], value);
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
            fhMonster.m_pos      = guard.m_pos->m_pos;
            m_map.m_objects.m_monsters.push_back(std::move(fhMonster));
        }
    }
}

void FHTemplateProcessor::runPlayerInfo()
{
    auto addHero = [this](FHHero fhhero, Core::LibraryHeroConstPtr heroId, bool stdStats) {
        FHHeroData& destHero = fhhero.m_data;
        destHero.m_army.hero = Core::AdventureHero(heroId);
        if (stdStats) {
            destHero.m_hasSecSkills              = true;
            destHero.m_army.hero.secondarySkills = heroId->isWarrior ? m_map.m_template.m_stdSkillsWarrior : m_map.m_template.m_stdSkillsMage;
        }

        m_map.m_wanderingHeroes.push_back(std::move(fhhero));
    };

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

            addHero(std::move(fhhero), info.m_startingHero, info.m_stdStats);
        }
        if (info.m_hasMainTown && info.m_extraHero) {
            const FHTown& mainTown = m_map.m_towns.at(info.m_mainTownMapIndex);

            FHPos pos = mainTown.m_pos;
            pos.m_x -= 2;
            pos.m_y += 1;

            FHHero fhhero;
            fhhero.m_player = playerId;
            fhhero.m_pos    = pos;

            addHero(std::move(fhhero), info.m_extraHero, info.m_stdStats);
        }
    }
}

void FHTemplateProcessor::placeTerrainZones()
{
    if (m_terrainPlaced)
        return;
    m_terrainPlaced = true;

    for (auto& tileZone : m_tileZones) {
        FHZone fhZone;
        fhZone.m_tiles.reserve(tileZone.m_area.m_innerArea.size());
        for (auto* cell : tileZone.m_area.m_innerArea)
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
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_startTile->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 1 }); // red
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_centroid->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
        }

        if (m_stopAfter <= Stage::ZoneTilesRefinement) {
            for (auto* cell : tileZone.m_area.m_innerEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 2 });
            }
            for (auto* cell : tileZone.m_area.m_outsideEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
            }
        }

        if (m_stopAfter <= Stage::BorderRoads) {
            for (auto* cell : tileZone.m_blocked) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = 0, .m_valueB = 4 });
            }
        }
        if (m_stopAfter <= Stage::RoadsPlacement) {
            for (auto* cell : tileZone.m_roadNodes) {
                const int roadLevel  = tileZone.getRoadLevel(cell);
                const int debugValue = roadLevel == 0 ? 1 : (roadLevel == 2 ? 4 : 2);
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = debugValue });
            }
            for (auto* cell : tileZone.m_innerAreaSegmentsRoads) {
                if (!tileZone.m_roadNodes.contains(cell))
                    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
            }
        }
    }
}

Core::LibraryFactionConstPtr FHTemplateProcessor::getRandomFaction(bool rewardOnly)
{
    auto& factions = rewardOnly ? m_rewardFactions : m_playableFactions;
    auto  result   = factions[m_rng->genSmall(factions.size() - 1)];
    return result;
}

Core::LibraryFactionConstPtr FHTemplateProcessor::getRandomPlayableFaction(const std::set<std::string>& excludedZoneIds)
{
    std::vector<Core::LibraryFactionConstPtr> factions;
    auto                                      excluded = getExcludedFactions(excludedZoneIds);
    for (auto* faction : m_playableFactions)
        if (!excluded.contains(faction))
            factions.push_back(faction);

    auto faction = factions[m_rng->genSmall(factions.size() - 1)];
    return faction;
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

    std::sort(heroes.begin(), heroes.end(), [](Core::LibraryHeroConstPtr l, Core::LibraryHeroConstPtr r) {
        return l->sortLess(*r);
    });

    auto* hero = heroes[m_rng->gen(heroes.size() - 1)];
    m_heroPool.erase(hero);
    return hero;
}

int FHTemplateProcessor::getPossibleCount(Core::LibraryUnitConstPtr unit, int64_t value) const
{
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
}

std::set<Core::LibraryFactionConstPtr> FHTemplateProcessor::getExcludedFactions(const std::set<std::string>& zoneIds) const
{
    std::set<Core::LibraryFactionConstPtr> result;
    for (auto& tileZone : m_tileZones) {
        if (zoneIds.contains(tileZone.m_id) && tileZone.m_mainTownFaction)
            result.insert(tileZone.m_mainTownFaction);
    }
    return result;
}

bool FHTemplateProcessor::CmpPlayers::operator()(Core::LibraryPlayerConstPtr a, Core::LibraryPlayerConstPtr b) const
{
    return a->id < b->id;
}

}
