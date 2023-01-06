/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTemplateProcessor.hpp"
#include "FHTemplateUtils.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"
#include "MernelPlatform/Profiler.hpp"

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
                         RoadsPlacement,
                         Borders)

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
    int     m_zoneIndex  = 0;
    int64_t m_distance   = 0;
    int64_t m_zoneRadius = 0;

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
                         Stage::RoadsPlacement,
                         Stage::Borders }) {
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
        case Stage::RoadsPlacement:
            return runRoadsPlacement();
        case Stage::Borders:
            return runBorders();
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
        int greedyPercent         = tileZone.m_rngZoneSettings.m_greedy ? 120 : 100;
        tileZone.m_absoluteArea   = tileZone.m_relativeArea * area * greedyPercent / m_totalRelativeArea / 100;
        tileZone.m_absoluteRadius = static_cast<int64_t>(sqrt(tileZone.m_absoluteArea) / M_PI);

        m_logOutput << m_indent << "zone [" << tileZone.m_id << "] area=" << tileZone.m_absoluteArea << ", radius=" << tileZone.m_absoluteRadius << "\n";
    }

    for (auto& [pos, cell] : m_mapCanvas.m_tiles) {
        //cell.m_zoneIndex = 1;
        std::vector<DistanceRecord> distances;

        for (auto& tileZone : m_tileZones) {
            const auto&   zonePos  = tileZone.m_startTile;
            const int64_t distance = posDistance(pos, zonePos);
            distances.push_back({ tileZone.m_index, distance, tileZone.m_absoluteRadius });
        }
        std::sort(distances.begin(), distances.end(), [](const DistanceRecord& l, const DistanceRecord& r) {
            return l.dbr() < r.dbr();
        });
        const DistanceRecord& first  = distances[0];
        const DistanceRecord& second = distances[1];

        const int64_t firstZoneRadius         = first.m_zoneRadius;
        const int64_t secondZoneRadius        = second.m_zoneRadius;
        const int64_t zonesTotalRadius        = firstZoneRadius + secondZoneRadius;
        const int64_t totalDistance           = first.m_distance + second.m_distance;
        const int64_t totalDistanceInRadiuses = totalDistance * 100 / zonesTotalRadius;
        const auto    distanceDiff            = totalDistanceInRadiuses * first.m_zoneRadius / 100 - first.m_distance;

        if (distanceDiff < 2)
            continue;

        cell.m_zoned     = true;
        cell.m_zoneIndex = first.m_zoneIndex;
    }

    for (auto& tileZone : m_tileZones) {
        tileZone.readFromMap();

        m_logOutput << m_indent << "zone [" << tileZone.m_id << "] areaDeficit=" << tileZone.getAreaDeficit() << "\n";
    }
    for (auto& tileZone : m_tileZones) {
        tileZone.writeToMap();
    }
}

void FHTemplateProcessor::runZoneTilesExpand()
{
    m_tileZonesPtrs.resize(m_tileZones.size());
    for (size_t i = 0; i < m_tileZonesPtrs.size(); ++i)
        m_tileZonesPtrs[i] = &m_tileZones[i];

    auto fillDeficitIteraction = [this](int thresholdPercent, bool allowConsumingNeighbours) {
        std::sort(m_tileZonesPtrs.begin(), m_tileZonesPtrs.end(), [](TileZone* l, TileZone* r) {
            return l->getAreaDeficit() > r->getAreaDeficit();
        });
        for (TileZone* zone : m_tileZonesPtrs) {
            if (!allowConsumingNeighbours && zone->m_rngZoneSettings.m_greedy)
                continue;

            zone->fillDeficit(thresholdPercent, allowConsumingNeighbours);
            if (allowConsumingNeighbours) {
                for (auto& tileZone : m_tileZones) {
                    tileZone.readFromMapIfDirty();
                }
            }
        }
    };

    for (auto& tileZone : m_tileZones) {
        m_logOutput << m_indent << "(before optimize) zone [" << tileZone.m_id << "] areaDeficit=" << tileZone.getAreaDeficit() << "\n";
    }

    //fillDeficitIteraction(25, false);
    // fillDeficitIteraction(15, false);
    fillDeficitIteraction(20, true);
    fillDeficitIteraction(10, true);
    //fillDeficitIteraction(5, false);
    fillDeficitIteraction(0, true);

    for (auto& tileZone : m_tileZones) {
        m_logOutput << m_indent << "(after optimize) zone [" << tileZone.m_id << "] areaDeficit=" << tileZone.getAreaDeficit() << "\n";
    }
}

void FHTemplateProcessor::runZoneTilesRefinement()
{
    for (TileZone* zone : m_tileZonesPtrs) {
        zone->fillTheRest();
    }
    for (auto& tileZone : m_tileZones) {
        tileZone.readFromMapIfDirty();
    }
    m_mapCanvas.checkUnzoned();

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
    for (TileZone* zone : m_tileZonesPtrs) {
        zone->fillTheRest();
    }

    m_mapCanvas.checkUnzoned();

    // debug exclives
    //for (auto& [pos, cell] : mapDraft.m_tiles) {
    //    map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = cell.m_zoned ? cell.m_zoneIndex : 11, .m_valueB = cell.m_exFix ? 1 : 0 });
    // }

    std::set<FHPos> placed;
    for (auto& tileZone : m_tileZones) {
        placed.insert(tileZone.m_innerArea.cbegin(), tileZone.m_innerArea.cend());
    }

    m_mapCanvas.checkAllTerrains(placed);

    m_map.m_tileMapUpdateRequired = true;
}

void FHTemplateProcessor::runTownsPlacement()
{
}

void FHTemplateProcessor::runRoadsPlacement()
{
}

void FHTemplateProcessor::runBorders()
{
}

void FHTemplateProcessor::placeTerrainZones()
{
    for (auto& tileZone : m_tileZones) {
        tileZone.readFromMap();

        FHZone fhZone;
        fhZone.m_tiles     = { tileZone.m_innerArea.cbegin(), tileZone.m_innerArea.cend() };
        fhZone.m_terrainId = tileZone.m_rngZoneSettings.m_terrain;
        assert(fhZone.m_terrainId);
        m_map.m_zones.push_back(std::move(fhZone));
    }
}

void FHTemplateProcessor::placeDebugInfo()
{
    for (auto& tileZone : m_tileZones) {
        m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = tileZone.m_startTile, .m_valueA = tileZone.m_index, .m_valueB = 1 }); // red
        if (m_stopAfter <= Stage::ZoneTilesRefinement) {
            for (auto& pos : tileZone.m_innerEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = tileZone.m_index, .m_valueB = 2 });
            }
            for (auto& pos : tileZone.m_outsideEdge) {
                m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = pos, .m_valueA = tileZone.m_index, .m_valueB = 3 });
            }
        }
    }
}

}
