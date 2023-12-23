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

#include "RmgUtil/ZoneObjectDistributor.hpp"
#include "RmgUtil/ObjectGenerator.hpp"
#include "RmgUtil/ObstacleHelper.hpp"
#include "RmgUtil/RoadHelper.hpp"
#include "RmgUtil/SegmentHelper.hpp"
#include "RmgUtil/TemplateUtils.hpp"
#include "RmgUtil/MapTileRegionSegmentation.hpp"

#include <functional>
#include <stdexcept>
#include <iostream>

namespace Mernel::Reflection {

ENUM_REFLECTION_STRINGIFY(
    FreeHeroes::FHTemplateProcessor::Stage,
    Invalid,
    Invalid,
    ZoneCenterPlacement,
    ZoneTilesInitial,
    BorderRoads,
    TownsPlacement,
    CellSegmentation,
    RoadsPlacement,
    SegmentationRefinement,
    HeatMap,
    Rewards,
    CorrectObjectTerrains,
    Obstacles,
    Guards,
    PlayerInfo)

}

namespace FreeHeroes {

namespace {

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
                                         std::ostream&              logOutput,
                                         const std::string&         stopAfterStage,
                                         const std::string&         debugStage,
                                         const std::string&         tileZoneFilter,
                                         int                        stopAfterHeat,
                                         bool                       extraLogs)
    : m_map(map)
    , m_database(database)
    , m_rng(rng)
    , m_logOutput(logOutput)
    , m_stopAfter(stringToStage(stopAfterStage))
    , m_showDebug(stringToStage(debugStage))
    , m_tileZoneFilter(tileZoneFilter)
    , m_stopAfterHeat(stopAfterHeat)
    , m_extraLogging(extraLogs)
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

std::string FHTemplateProcessor::stageToString(Stage stage)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(stage);
    return std::string(str.begin(), str.end());
}

void FHTemplateProcessor::run()
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

    for (const auto& [key, rngZone] : m_map.m_template.m_zones) {
        if (rngZone.m_player == nullptr || !rngZone.m_player->isPlayable)
            continue;

        if (!m_map.m_template.m_userSettings.m_players[rngZone.m_player].m_enabled)
            continue;

        m_playerInfo[rngZone.m_player] = {};
    }
    for (auto& [playerId, info] : m_playerInfo) {
        auto& pl               = m_map.m_template.m_userSettings.m_players[playerId];
        info.m_faction         = pl.m_faction;
        info.m_startingHero    = pl.m_startingHero;
        info.m_extraHero       = pl.m_extraHero;
        info.m_startingHeroGen = pl.m_startingHeroGen;
        info.m_extraHeroGen    = pl.m_extraHeroGen;
        info.m_team            = pl.m_team;

        m_logOutput << baseIndent << "add player: " << playerId->id << "\n";
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
        auto& tileZone    = m_tileZones[i];
        tileZone.m_player = rngZone.m_player;
        if (!m_playerInfo.contains(tileZone.m_player))
            tileZone.m_player = m_database->players()->find(std::string(Core::LibraryPlayer::s_none));
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
        tileZone.m_startTile = m_tileContainer.m_all.findClosestPoint(startTile);

        tileZone.m_relativeArea = m_rng->genDispersed(rngZone.m_relativeSizeAvg, rngZone.m_relativeSizeDispersion);

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

    Mernel::ProfilerContext                profileContext;
    Mernel::ProfilerDefaultContextSwitcher switcher(profileContext);

    for (Stage stage : { Stage::ZoneCenterPlacement,
                         Stage::ZoneTilesInitial,
                         Stage::BorderRoads,
                         Stage::TownsPlacement,
                         Stage::CellSegmentation,
                         Stage::RoadsPlacement,
                         Stage::SegmentationRefinement,
                         Stage::HeatMap,
                         Stage::Rewards,
                         Stage::CorrectObjectTerrains,
                         Stage::Obstacles,
                         Stage::Guards,
                         Stage::PlayerInfo }) {
        m_currentStage = stage;

        Mernel::ScopeTimer timer;
        m_logOutput << baseIndent << "Start stage: " << stageToString(m_currentStage) << "\n";
        runCurrentStage();
        {
            auto profilerStr = profileContext.printToStr();
            profileContext.clearAll();
            std::string profilerStr2;
            for (size_t i = 0; i < profilerStr.size(); ++i) {
                char c = profilerStr[i];
                profilerStr2 += c;
                if (c == '\n' && i < profilerStr.size() - 1)
                    profilerStr2 += m_indent;
            }
            if (!profilerStr.empty())
                m_logOutput << baseIndent << "Profiler data:\n"
                            << m_indent << profilerStr2;
        }
        m_logOutput << baseIndent << "End stage: " << stageToString(m_currentStage) << " (" << timer.elapsedUS() << " us.)\n";

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
        case Stage::BorderRoads:
            return runBorderRoads();
        case Stage::TownsPlacement:
            return runTownsPlacement();
        case Stage::CellSegmentation:
            return runCellSegmentation();
        case Stage::RoadsPlacement:
            return runRoadsPlacement();
        case Stage::SegmentationRefinement:
            return runSegmentationRefinement();
        case Stage::HeatMap:
            return runHeatMap();
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
}

void FHTemplateProcessor::runZoneTilesInitial()
{
    SegmentHelper helper(m_map, m_tileContainer, m_rng, m_logOutput, m_extraLogging);
    helper.makeInitialZones(m_tileZones);

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
        for (auto* cell : m_tileContainer.m_all) {
            if (!cell->m_zone) {
                m_logOutput << m_indent << "Unzoned cell:" << cell->toPrintableString() << "\n";
                result = false;
            }
        }
        if (!result) {
            throw std::runtime_error("All tiles must be zoned!");
        }
    };

    checkUnzoned();

    MapTileRegion placed;
    for (auto& tileZone : m_tileZones) {
        placed.insert(tileZone.m_area.m_innerArea);
    }

    for (auto* cell : m_tileContainer.m_all) {
        if (!placed.contains(cell)) {
            throw std::runtime_error("I forget to place tile: " + cell->m_pos.toPrintableString());
        }
    }

    for (auto& tileZone : m_tileZones) {
        tileZone.m_centroid = tileZone.m_area.m_innerArea.makeCentroid(true);
    }

    m_map.m_tileMapUpdateRequired = true;
    placeTerrainZones();
}

void FHTemplateProcessor::runBorderRoads()
{
    SegmentHelper segmentHelper(m_map, m_tileContainer, m_rng, m_logOutput, m_extraLogging);
    auto          guards = segmentHelper.makeBorders(m_tileZones);

    for (auto& guard : guards) {
        m_guards.push_back(std::move(guard));
    }
}

void FHTemplateProcessor::runTownsPlacement()
{
    auto placeTown = [this](FHTown& town, FHPos pos, TileZone& tileZone, Core::LibraryPlayerConstPtr player, Core::LibraryFactionConstPtr faction) {
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
                town.m_garison.stacks.push_back({ unit, count });
            }
        }
        m_map.m_towns.push_back(town);
        MapTileRegionWithEdge townArea;
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
        tileZone.m_unpassableArea.insert(townArea.m_innerArea);
        tileZone.m_innerAreaTownsBorders.insert(townArea.m_innerArea);
        tileZone.m_innerAreaTownsBorders.insert(townArea.m_outsideEdge);
        tileZone.m_roadPotentialArea.insert(townArea.m_outsideEdge);
    };

    auto playerNone = m_database->players()->find(std::string(Core::LibraryPlayer::s_none));

    //RoadHelper roadHelper(m_map, m_tileContainer, m_rng, m_logOutput);

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
            auto&                      area = tileZone.m_innerAreaUsable.m_innerArea;
            KMeansSegmentationSettings settings;
            const size_t               K = towns.size();
            {
                // random cluster init
                settings.m_items.resize(K);
                MapTileRegion used;
                for (size_t i = 0; i < K; i++) {
                    while (true) {
                        size_t index = m_rng->gen(area.size() - 1);
                        auto*  tile  = area[index];
                        if (!used.contains(tile)) {
                            used.insert(tile);
                            break;
                        }
                    }
                }
                for (size_t i = 0; i < K; i++)
                    settings.m_items[i].m_initialCentroid = used[i];
            }
            const auto regionsEst = area.splitByKExt(settings);

            std::vector<MapTilePtr> townPositionsEst;

            for (const auto& cluster : regionsEst)
                townPositionsEst.push_back(cluster.makeCentroid(true));

            for (size_t i = 0; i < townPositions.size(); ++i) {
                auto* cell = townPositions[i];
                if (!cell)
                    continue;
                // if we have fix town, give it high weight and erase closest random position
                settings.m_items[i].m_initialCentroid = cell;
                settings.m_items[i].m_extraMassPoint  = cell;
                settings.m_items[i].m_extraMassWeight = area.size() * 2;

                auto it = std::min_element(townPositionsEst.begin(), townPositionsEst.end(), [cell](MapTilePtr l, MapTilePtr r) {
                    return posDistance(cell, l) < posDistance(cell, r);
                });
                townPositionsEst.erase(it);
            }
            const auto regions = area.splitByKExt(settings);

            for (size_t i = 0; i < townPositions.size(); ++i) {
                auto* cell = townPositions[i];
                if (cell)
                    continue;
                townPositions[i] = regions[i].makeCentroid(true);
            }
        }

        for (size_t i = 0; const auto& [_, town] : towns) {
            auto player  = town.m_playerControlled ? tileZone.m_player : playerNone;
            auto faction = town.m_useZoneFaction ? tileZone.m_mainTownFaction : nullptr;
            if (!faction)
                faction = getRandomPlayableFaction(town.m_excludeFactionZones);

            assert(townPositions[i]);
            FHTown townCopy = town.m_town;
            placeTown(townCopy, townPositions[i]->m_pos, tileZone, player, faction);
            i++;
        }

        for (auto&& pos : townPositions) {
            auto pos2 = pos->m_neighborB;
            tileZone.m_nodes.add(pos2, RoadLevel::Towns);
            tileZone.m_roads.add(pos, RoadLevel::Towns);
            tileZone.m_roads.add(pos2, RoadLevel::Towns);
        }

        {
            MapTileRegionWithEdge areaTowns;
            areaTowns.m_innerArea = tileZone.m_innerAreaTownsBorders;
            areaTowns.makeEdgeFromInnerArea();
            tileZone.m_innerAreaTownsBorders.insert(areaTowns.m_outsideEdge);
        }
        tileZone.m_innerAreaUsable.m_innerArea.erase(tileZone.m_unpassableArea);
        tileZone.m_innerAreaUsable.makeEdgeFromInnerArea();
    }
}

void FHTemplateProcessor::runCellSegmentation()
{
    SegmentHelper segmentHelper(m_map, m_tileContainer, m_rng, m_logOutput, m_extraLogging);

    for (auto& tileZone : m_tileZones) {
        if (isFilteredOut(tileZone))
            continue;

        segmentHelper.makeSegments(tileZone);
    }
}

void FHTemplateProcessor::runRoadsPlacement()
{
    RoadHelper roadHelper(m_map, m_tileContainer, m_rng, m_logOutput, m_extraLogging);

    for (auto& tileZone : m_tileZones) {
        if (isFilteredOut(tileZone))
            continue;

        auto prevPlaced = tileZone.m_roads.m_tileLevels;
        roadHelper.placeRoads(tileZone);
        for (auto& [tile, level] : prevPlaced)
            tileZone.m_roads.add(tile, level);

        for (const auto& [level, region] : tileZone.m_roads.m_byLevel) {
            roadHelper.placeRoad(region, level);
        }
    }
}

void FHTemplateProcessor::runSegmentationRefinement()
{
    SegmentHelper segmentHelper(m_map, m_tileContainer, m_rng, m_logOutput, m_extraLogging);

    for (auto& tileZone : m_tileZones) {
        if (isFilteredOut(tileZone))
            continue;

        segmentHelper.refineSegments(tileZone);
    }
}

void FHTemplateProcessor::runHeatMap()
{
    SegmentHelper segmentHelper(m_map, m_tileContainer, m_rng, m_logOutput, m_extraLogging);

    for (auto& tileZone : m_tileZones) {
        if (isFilteredOut(tileZone))
            continue;

        segmentHelper.makeHeatMap(tileZone);
    }
}

void FHTemplateProcessor::runRewards()
{
    m_logOutput << m_indent << "RNG TEST A:" << m_rng->gen(1000000) << "\n";

    const auto&   diffSett    = m_map.m_template.m_userSettings.m_difficulty;
    const int64_t armyPercent = m_rng->genMinMax(diffSett.m_minArmyPercent, diffSett.m_maxArmyPercent);
    const int64_t goldPercent = m_rng->genMinMax(diffSett.m_minGoldPercent, diffSett.m_maxGoldPercent);

    m_logOutput << m_indent << "armyPercent=" << armyPercent << ", goldPercent=" << goldPercent << "\n";

    const ObjectGenerator gen(m_map, m_database, m_rng, m_logOutput);

    const ZoneObjectDistributor objectDistributor(m_map, m_rng, m_tileContainer, m_logOutput);

    for (auto& tileZone : m_tileZones) {
        if (tileZone.m_rngZoneSettings.m_scoreTargets.empty())
            continue;

        if (isFilteredOut(tileZone))
            continue;

        ZoneObjectDistributor::DistributionResult distributionResultCopy;
        distributionResultCopy.init(tileZone);
        distributionResultCopy.m_stopAfterHeat = m_stopAfterHeat;

        auto      objects       = m_map.m_objects;
        auto      needBeBlocked = tileZone.m_needPlaceObstacles;
        const int maxAttempts   = 3;
        for (int i = 1; i <= maxAttempts; ++i) {
            m_logOutput << m_indent << " --- generate : " << tileZone.m_id << " [attempt " << i << " / " << maxAttempts << "] --- \n";
            auto zoneObjectGeneration = gen.generate(tileZone.m_rngZoneSettings,
                                                     tileZone.m_rewardsFaction,
                                                     tileZone.m_dwellFaction,
                                                     tileZone.m_terrain,
                                                     armyPercent,
                                                     goldPercent);

            auto distributionResult = distributionResultCopy;

            if (objectDistributor.makeInitialDistribution(distributionResult, zoneObjectGeneration)) {
                objectDistributor.doPlaceDistribution(distributionResult);
                distributionResultCopy = distributionResult;
                if (m_showDebug == Stage::Rewards) {
                    for (auto& seg : distributionResult.m_segments) {
                        for (auto* object : seg.m_successNormal) {
                            int paletteSize = distributionResult.m_maxHeat;

                            m_map.m_debugTiles.push_back(FHDebugTile{
                                .m_pos         = object->m_absPos->m_pos,
                                .m_penColor    = object->m_preferredHeat + 1, // heatLevel is 0-based
                                .m_penAlpha    = 120,
                                .m_penPalette  = paletteSize,
                                .m_shape       = 1,
                                .m_shapeRadius = 4,
                            });
                            m_map.m_debugTiles.push_back(FHDebugTile{
                                .m_pos         = object->m_absPos->m_pos,
                                .m_penColor    = object->m_placedHeat + 1, // heatLevel is 0-based
                                .m_penAlpha    = 120,
                                .m_penPalette  = paletteSize,
                                .m_shape       = 1,
                                .m_shapeRadius = 1,
                            });
                        }
                    }
                }
                break;
            }

            if (i == maxAttempts)
                throw std::runtime_error("Failed to fit some objects into zone '" + tileZone.m_id + "'");
            m_logOutput << m_indent << "Failed to fit some objects into zone '" + tileZone.m_id + "', retry"
                        << "\n";
            m_map.m_objects = objects; // restore map data and try again.
            m_map.m_debugTiles.clear();
            tileZone.m_needPlaceObstacles = needBeBlocked;
        }

        for (auto& guard : distributionResultCopy.m_guards) {
            guard.m_zone     = &tileZone;
            guard.m_joinable = true;
            m_guards.push_back(std::move(guard));
        }

        tileZone.m_needPlaceObstacles.insert(distributionResultCopy.m_needBlock);

        //for (auto* cell : bundleSet.m_consumeResult.m_centroidsALL) {
        //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 1 }); // red
        //}
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
        if (obj.m_id->allowedTerrainsOverride.empty())
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
        for (auto* tile : tileZone.m_needPlaceObstacles) {
            m_logOutput << m_indent << "still require to be blocked: " << tile->toPrintableString() << "\n";
        }
        tileZone.m_needPlaceObstaclesTentative.clear();
        if (!tileZone.m_needPlaceObstacles.empty()) {
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
        fhMonster.m_pos          = guard.m_pos->m_pos;
        fhMonster.m_count        = getPossibleCount(unit, value);
        fhMonster.m_id           = unit;
        fhMonster.m_guardValue   = value;
        fhMonster.m_score        = guard.m_score;
        fhMonster.m_generationId = guard.m_generationId;

        if (upgraded) {
            auto upCount = getPossibleCount(unit->upgrades[0], value);
            // let's say upgraded stack is 1/4 of stacks. Then recalc count taking an account 1/4 of value is upped.
            fhMonster.m_count = fhMonster.m_count * 3 / 4 + upCount * 1 / 4;
        }

        if (guard.m_joinable) {
            fhMonster.m_aggressionMin = 4;
            fhMonster.m_aggressionMax = 10;
        } else {
            fhMonster.m_aggressionMin = 10;
            fhMonster.m_aggressionMax = 10;
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
    auto addHero = [this](FHHero fhhero, Core::LibraryHeroConstPtr heroId) {
        FHHeroData& destHero = fhhero.m_data;
        destHero.m_army.hero = Core::AdventureHero(heroId);
        if (false) { // @todo: beteer hero customization!
            destHero.m_hasSecSkills              = true;
            destHero.m_army.hero.secondarySkills = heroId->isWarrior ? m_map.m_template.m_stdSkillsWarrior : m_map.m_template.m_stdSkillsMage;
        }

        m_map.m_wanderingHeroes.push_back(std::move(fhhero));
    };

    for (const auto& [playerId, info] : m_playerInfo) {
        FHPlayer& player                = m_map.m_players[playerId];
        player.m_startingFactions       = { info.m_faction };
        player.m_generateHeroAtMainTown = false;
        player.m_aiPossible             = true;
        player.m_humanPossible          = true;
        player.m_team                   = info.m_team;

        if (info.m_hasMainTown && info.m_startingHero) {
            const FHTown& mainTown = m_map.m_towns.at(info.m_mainTownMapIndex);
            FHPos         pos      = mainTown.m_pos;
            pos.m_x -= 2;

            FHHero fhhero;
            fhhero.m_player = playerId;
            fhhero.m_pos    = pos;
            fhhero.m_isMain = true;

            addHero(std::move(fhhero), info.m_startingHero);
        }
        if (info.m_hasMainTown && info.m_extraHero) {
            const FHTown& mainTown = m_map.m_towns.at(info.m_mainTownMapIndex);

            FHPos pos = mainTown.m_pos;
            pos.m_x -= 2;
            pos.m_y += 1;

            FHHero fhhero;
            fhhero.m_player = playerId;
            fhhero.m_pos    = pos;

            addHero(std::move(fhhero), info.m_extraHero);
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
    if (m_showDebug == Stage::Invalid)
        return;

    for (auto& tileZone : m_tileZones) {
        if (m_showDebug == Stage::ZoneTilesInitial) {
            if (auto* cell = tileZone.m_startTile) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 1, .m_shapeRadius = 4 });
            }
            if (auto* cell = tileZone.m_centroid) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 150, .m_shapeRadius = 4 });
            }
        }
        /*
        if (m_showDebug == Stage::TownsPlacement) {
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_startTile->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 1 }); // red
            m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_centroid->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
        }

        if (m_showDebug == Stage::ZoneTilesRefinement) {
            for (auto* cell : tileZone.m_area.m_innerEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_valueA = tileZone.m_index, .m_valueB = 2 });
            }
            
        }
        */
        if (m_showDebug == Stage::BorderRoads) {
            for (auto& guard : m_guards) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = guard.m_pos->m_pos, .m_brushColor = 1, .m_shapeRadius = 1 });
            }
            for (auto* cell : tileZone.m_needPlaceObstacles) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_penColor = 1, .m_shape = 1, .m_shapeRadius = 3 });
            }
            for (auto* cell : tileZone.m_needPlaceObstaclesTentative) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_penColor = 43, .m_shape = 1, .m_shapeRadius = 3 });
            }
            //for (auto* cell : tileZone.m_area.m_innerEdge) {
            //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 80, .m_brushAlpha = 200, .m_shape = 1, .m_shapeRadius = 1 });
            //}
            //for (auto* cell : tileZone.m_innerAreaUsable.m_innerEdge) {
            //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 300, .m_brushAlpha = 200, .m_shape = 1, .m_shapeRadius = 1 });
            //}
        }
        if (m_showDebug == Stage::CellSegmentation) {
            for (auto& seg : tileZone.m_innerAreaSegments) {
                for (auto* cell : seg.m_innerEdge) {
                    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 40, .m_shapeRadius = 3 });
                }
            }
            for (auto* cell : tileZone.m_roadPotentialArea) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 330, .m_shapeRadius = 1 });
            }
        }
        if (m_showDebug == Stage::CellSegmentation || m_showDebug == Stage::RoadsPlacement || m_showDebug == Stage::BorderRoads) {
            for (const auto& [roadLevel, area] : tileZone.m_nodes.m_byLevel) {
                for (auto* cell : area) {
                    if (roadLevel == RoadLevel::NoRoad) // error!
                        m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 1, .m_shape = 2, .m_shapeRadius = 3 });
                    else
                        m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_penColor = int(roadLevel) + 1, .m_penPalette = 6, .m_shape = 2, .m_shapeRadius = 3 });
                }
            }
            for (const auto& [roadLevel, area] : tileZone.m_roads.m_byLevel) {
                for (auto* cell : area) {
                    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = int(roadLevel) + 1, .m_brushPalette = 6, .m_shape = 2, .m_shapeRadius = 1 });
                }
            }
        }
        if (m_showDebug == Stage::SegmentationRefinement) {
            for (auto& seg : tileZone.m_innerAreaSegments) {
                for (auto* cell : seg.m_innerEdge) {
                    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 40, .m_shapeRadius = 3, .m_text = std::to_string(seg.m_index) });
                }
            }
        }

        if (m_showDebug == Stage::HeatMap) {
            auto* roadHeat = &tileZone.m_heatForRoads;
            auto* segHeat  = &tileZone.m_heatForSegments;
            for (auto* heatData : { roadHeat, segHeat }) {
                const bool isRoad = roadHeat == heatData;
                for (const auto& [tile, heatLevel] : heatData->m_tileLevels) {
                    int penColor = isRoad ? -2 : 0;   // white/transparent
                    int shape    = 1 + heatLevel % 2; // cirlce/square

                    int paletteSize = tileZone.m_rngZoneSettings.m_maxHeat;
                    m_map.m_debugTiles.push_back(FHDebugTile{
                        .m_pos          = tile->m_pos,
                        .m_brushColor   = heatLevel + 1, // heatLevel is 0-based
                        .m_brushPalette = paletteSize,
                        .m_penColor     = penColor,
                        //.m_textColor    = (heat == 1 ? -1 : -2),
                        .m_shape = shape,
                        //.m_text         = std::to_string(heat),
                    });
                }
            }
        }
        if (m_showDebug == Stage::HeatMap || m_showDebug == Stage::RoadsPlacement) {
            auto* townMids = &tileZone.m_midTownNodes;
            auto* exitMids = &tileZone.m_midExitNodes;
            for (auto* mids : { townMids, exitMids }) {
                const bool isTown = townMids == mids;
                for (auto* cell : *mids) {
                    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = isTown ? 1 : 50, .m_brushAlpha = 200, .m_shapeRadius = 4 });
                }
            }
        }

        if (m_showDebug == Stage::Rewards) {
            for (auto& seg : tileZone.m_innerAreaSegments) {
                for (auto* tile : seg.m_innerEdge) {
                    int paletteSize = tileZone.m_rngZoneSettings.m_maxHeat;
                    int heatLevel   = tileZone.m_heatForSegments.getLevel(tile);
                    m_map.m_debugTiles.push_back(FHDebugTile{
                        .m_pos         = tile->m_pos,
                        .m_penColor    = heatLevel + 1, // heatLevel is 0-based
                        .m_penPalette  = paletteSize,
                        .m_shape       = 2,
                        .m_shapeRadius = 4,
                    });
                }
            }

            for (auto* cell : tileZone.m_rewardTilesSpacing) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 55, .m_shapeRadius = 1 });
            }
            for (auto* cell : tileZone.m_rewardTilesFailure) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = cell->m_pos, .m_brushColor = 1, .m_brushAlpha = 80, .m_shapeRadius = 4 });
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

bool FHTemplateProcessor::isFilteredOut(const TileZone& tileZone) const
{
    if (m_tileZoneFilter.empty())
        return false;
    return m_tileZoneFilter != tileZone.m_id;
}

bool FHTemplateProcessor::CmpPlayers::operator()(Core::LibraryPlayerConstPtr a, Core::LibraryPlayerConstPtr b) const
{
    return a->id < b->id;
}

}
