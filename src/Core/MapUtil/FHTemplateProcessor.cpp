/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTemplateProcessor.hpp"
#include "FHTemplateUtils.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "KMeans.hpp"

#include <functional>
#include <stdexcept>
#include <iostream>

namespace Mernel::Reflection {
using namespace FreeHeroes;
ENUM_REFLECTION_STRINGIY(FHTemplateProcessor::Stage,
                         Invalid,
                         Invalid,
                         ZoneCenterPlacement,
                         ZoneTilesInitial,
                         ZoneTilesExpand,
                         ZoneTilesRefinement,
                         TownsPlacement,
                         BorderRoads,
                         RoadsPlacement,
                         Borders,
                         InnerObstacles,
                         Rewards,
                         Guards)

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
        if (faction->alignment == Core::LibraryFaction::Alignment::Special
            || faction->alignment == Core::LibraryFaction::Alignment::Independent)
            continue;
        m_playableFactions.push_back(faction);
    }
    assert(!m_playableFactions.empty());
    auto& units = m_database->units()->records();
    for (auto* unit : units) {
        if (unit->faction->alignment == Core::LibraryFaction::Alignment::Special)
            continue;
        m_guardUnits.push_back({ unit, unit->value });
    }
    std::sort(m_guardUnits.begin(), m_guardUnits.end(), [](const Unit& l, const Unit& r) {
        return l.m_value > r.m_value;
    });
}

void FHTemplateProcessor::run(const std::string& stopAfterStage)
{
    std::string baseIndent        = "      ";
    m_indent                      = baseIndent + "  ";
    m_map.m_tileMapUpdateRequired = false;

    const int regionCount = m_map.m_rngZones.size();
    if (regionCount <= 1)
        throw std::runtime_error("need at least two zones");

    m_mapCanvas.init(m_map.m_tileMap.m_width, m_map.m_tileMap.m_height, m_map.m_tileMap.m_depth);

    m_totalRelativeArea = 0;

    m_tileZones.resize(regionCount);
    for (int i = 0; const auto& [key, rngZone] : m_map.m_rngZones) {
        auto& tileZone = m_tileZones[i];
        assert(rngZone.m_terrain);
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
                         Stage::Borders,
                         Stage::InnerObstacles,
                         Stage::Rewards,
                         Stage::Guards }) {
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
        case Stage::Borders:
            return runBorders();
        case Stage::InnerObstacles:
            return runInnerObstacles();
        case Stage::Rewards:
            return runRewards();
        case Stage::Guards:
            return runGuards();
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

        m_logOutput << m_indent << "zone [" << tileZone.m_id << "] area=" << tileZone.m_absoluteArea << ", radius=" << tileZone.m_absoluteRadius << "\n";
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
    auto placeTown = [this](FHPos pos, FHPlayerId player, Core::LibraryFactionConstPtr faction, bool hasFort) {
        FHTown town;
        if (!faction)
            faction = getRandomFaction();
        town.m_factionId = faction;

        pos.m_x += 2;
        town.m_pos     = pos;
        town.m_player  = player;
        town.m_hasFort = hasFort;
        if (hasFort)
            town.m_defIndex.variant = "FORT";
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
        if (tileZone.m_rngZoneSettings.m_towns == 0) {
            continue;
        } else if (tileZone.m_rngZoneSettings.m_towns == 1) {
            townPositions.push_back(tileZone.m_centroid);
        } else {
            KMeansSegmentation seg;
            seg.m_points.reserve(tileZone.m_innerArea.size());
            for (auto* cell : tileZone.m_innerArea)
                seg.m_points.push_back({ cell->m_pos });

            seg.initRandomClusterCentoids(tileZone.m_rngZoneSettings.m_towns, m_rng);
            seg.run(m_logOutput);

            for (KMeansSegmentation::Cluster& cluster : seg.m_clusters) {
                townPositions.push_back(cluster.getCentroid().toPos());
            }
        }
        auto mainTownFaction = tileZone.m_rngZoneSettings.m_faction;
        if (!mainTownFaction)
            mainTownFaction = getRandomFaction();

        placeTown(townPositions[0], tileZone.m_rngZoneSettings.m_player, mainTownFaction, true);
        for (size_t i = 1; i < townPositions.size(); ++i) {
            placeTown(townPositions[i], tileZone.m_rngZoneSettings.m_player, nullptr, false);
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

    for (const auto& [connectionId, connections] : m_map.m_rngConnections) {
        auto&                 tileZoneFrom = findZoneById(connections.m_from);
        auto&                 tileZoneTo   = findZoneById(connections.m_to);
        auto                  key          = makeKey(tileZoneFrom, tileZoneTo);
        TileZone::TileRegion& border       = borderTiles[key];
        if (border.empty()) {
            throw std::runtime_error("No border between '" + connections.m_from + "' and '" + connections.m_to + "'");
        }
        FHPos borderCentroid = TileZone::makeCentroid(border); // switch to k-means when we need more than one connection.

        {
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
            bool nfound         = false;
            auto checkNeighbour = [cell, &nfound](MapCanvas::Tile* ncell) {
                if (!nfound && ncell && ncell->m_zone != cell->m_zone) {
                    ncell->m_zone->m_roadNodes.insert(ncell);
                    nfound = true;
                }
            };
            checkNeighbour(cell->m_neighborB);
            checkNeighbour(cell->m_neighborT);
            checkNeighbour(cell->m_neighborR);
            checkNeighbour(cell->m_neighborL);
            assert(nfound);
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
            for (auto* cell : tileZone.m_innerArea)
                seg.m_points.push_back({ cell->m_pos });

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
                if (prev.m_x != cur.m_x && prev.m_y != cur.m_y) // diagonal
                {
                    FHPos extra = prev;
                    extra.m_y   = cur.m_y;
                    //extra.m_x   = cur.m_x;
                    path.push_back(extra);
                }
            }

            connected.push_back(cell);

            placeRoad(std::move(path));
        }
    }
}

void FHTemplateProcessor::runBorders()
{
}

void FHTemplateProcessor::runInnerObstacles()
{
}

void FHTemplateProcessor::runRewards()
{
}

void FHTemplateProcessor::runGuards()
{
    auto makeCandidates = [this](int64_t value) {
        std::vector<Unit*> candidates; // unit with at least 3 in the stack
        std::vector<Unit*> candidates3to9;
        std::vector<Unit*> candidates10to49;
        std::vector<Unit*> candidates50to99;
        std::vector<Unit*> candidates100plus;
        for (Unit& unit : m_guardUnits) {
            auto possibleCount = value / unit.m_value;
            if (possibleCount < 3)
                continue;
            candidates.push_back(&unit);
            if (possibleCount < 10)
                candidates3to9.push_back(&unit);
            else if (possibleCount < 50)
                candidates10to49.push_back(&unit);
            else if (possibleCount < 100)
                candidates50to99.push_back(&unit);
            else
                candidates100plus.push_back(&unit);
        }

        // @todo: that is just a draft!
        if (candidates10to49.size() > 10) {
            return candidates10to49;
        }
        if (candidates50to99.size() > 10) {
            return candidates50to99;
        }
        if (candidates3to9.size() > 5) {
            return candidates3to9;
        }
        if (candidates100plus.size() > 5) {
            return candidates100plus;
        }

        return candidates;
    };

    std::map<std::string, size_t> nameIndex;

    for (auto& guard : m_guards) {
        if (guard.m_value == 0)
            continue;

        std::vector<Unit*> candidates = makeCandidates(guard.m_value);

        if (candidates.empty()) { // value is so low, nobody can guard so low value at least in group of 3
            continue;
        }
        Unit* unit = candidates.size() == 1 ? candidates[0] : candidates[m_rng->gen(candidates.size() - 1)];

        auto finalValue = m_rng->genDispersed(guard.m_value, guard.m_valueDispersion);

        FHMonster fhMonster;
        fhMonster.m_pos        = guard.m_pos;
        fhMonster.m_count      = finalValue / unit->m_value;
        fhMonster.m_id         = unit->m_id;
        fhMonster.m_guardValue = finalValue;

        if (guard.m_joinable) {
            fhMonster.m_agressionMin = 4;
            fhMonster.m_agressionMax = 10;
        } else {
            fhMonster.m_agressionMin = 10;
            fhMonster.m_agressionMax = 10;
        }

        fhMonster.m_joinOnlyForMoney = true;
        fhMonster.m_joinPercent      = 50;

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

void FHTemplateProcessor::placeTerrainZones()
{
    for (auto& tileZone : m_tileZones) {
        FHZone fhZone;
        fhZone.m_tiles.reserve(tileZone.m_innerArea.size());
        for (auto* cell : tileZone.m_innerArea)
            fhZone.m_tiles.push_back(cell->m_pos);
        fhZone.m_terrainId = tileZone.m_rngZoneSettings.m_terrain;
        assert(fhZone.m_terrainId);
        m_map.m_zones.push_back(std::move(fhZone));
    }
}

void FHTemplateProcessor::placeDebugInfo()
{
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

Core::LibraryFactionConstPtr FHTemplateProcessor::getRandomFaction()
{
    return m_playableFactions[m_rng->genSmall(m_playableFactions.size() - 1)];
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

FHPos operator+(const FHPos& left_, const FHPos& right_)
{
    return { left_.m_x + right_.m_x, left_.m_y + right_.m_y };
}

AstarGenerator::Node::Node(FHPos pos, AstarGenerator::Node* parent)
{
    m_parent = parent;
    m_pos    = pos;
}

uint64_t AstarGenerator::Node::getScore()
{
    return m_G + m_H;
}

AstarGenerator::AstarGenerator()
{
    m_directions = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 }, { -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }
    };
}

void AstarGenerator::setWorldSize(FHPos worldSize_)
{
    m_worldSize = worldSize_;
}

void AstarGenerator::addCollision(FHPos coordinates_)
{
    m_collisions.push_back(coordinates_);
}

void AstarGenerator::removeCollision(FHPos coordinates_)
{
    auto it = std::find(m_collisions.begin(), m_collisions.end(), coordinates_);
    if (it != m_collisions.end()) {
        m_collisions.erase(it);
    }
}

void AstarGenerator::clearCollisions()
{
    m_collisions.clear();
}

AstarGenerator::CoordinateList AstarGenerator::findPath(FHPos source, FHPos target)
{
    std::shared_ptr<Node> current;
    NodeSet               openSet, closedSet;
    openSet.reserve(100);
    closedSet.reserve(100);
    openSet.push_back(std::make_shared<Node>(source));

    while (!openSet.empty()) {
        auto current_it = openSet.begin();
        current         = *current_it;

        for (auto it = openSet.begin(); it != openSet.end(); it++) {
            auto node = *it;
            if (node->getScore() <= current->getScore()) {
                current    = node;
                current_it = it;
            }
        }

        if (current->m_pos == target) {
            break;
        }

        closedSet.push_back(current);
        openSet.erase(current_it);

        for (uint64_t i = 0; i < m_directions.size(); ++i) {
            FHPos newCoordinates(current->m_pos + m_directions[i]);
            if (detectCollision(newCoordinates) || findNodeOnList(closedSet, newCoordinates)) {
                continue;
            }

            uint64_t totalCost = current->m_G + ((i < 4) ? 10 : 14);

            Node* successorRaw = findNodeOnList(openSet, newCoordinates);
            if (successorRaw == nullptr) {
                auto successor = std::make_shared<Node>(newCoordinates, current.get());
                successor->m_G = totalCost;
                successor->m_H = posDistance(successor->m_pos, target) * 10;
                openSet.push_back(successor);
            } else if (totalCost < successorRaw->m_G) {
                successorRaw->m_parent = current.get();
                successorRaw->m_G      = totalCost;
            }
        }
    }

    CoordinateList path;
    Node*          currentRaw = current.get();
    while (currentRaw != nullptr) {
        path.push_back(currentRaw->m_pos);
        currentRaw = currentRaw->m_parent;
    }

    return path;
}

AstarGenerator::Node* AstarGenerator::findNodeOnList(NodeSet& nodes_, FHPos coordinates_)
{
    for (auto&& node : nodes_) {
        if (node->m_pos == coordinates_) {
            return node.get();
        }
    }
    return nullptr;
}

bool AstarGenerator::detectCollision(FHPos coordinates_)
{
    if (coordinates_.m_x < 0 || coordinates_.m_x >= m_worldSize.m_x || coordinates_.m_y < 0 || coordinates_.m_y >= m_worldSize.m_y || std::find(m_collisions.begin(), m_collisions.end(), coordinates_) != m_collisions.end()) {
        return true;
    }
    return false;
}

}
