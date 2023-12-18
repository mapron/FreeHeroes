/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ZoneObjectDistributor.hpp"
#include "TileZone.hpp"
#include "MapTileRegionSegmentation.hpp"
#include "FHMap.hpp"

#include "IRandomGenerator.hpp"

#include "MernelPlatform/Profiler.hpp"
#include "MernelReflection/EnumTraitsMacro.hpp"

#include <iostream>
#include <sstream>

namespace FreeHeroes {

bool ZoneObjectWrap::estimateOccupied(MapTilePtr absPosCenter)
{
    if (!absPosCenter)
        return false;

    //Mernel::ProfilerScope scope("estimateOccupied");
    m_rewardArea.clear();
    m_extraObstacles.clear();
    m_unpassableArea.clear();
    m_occupiedArea.clear();

    m_dangerZone.clear();
    m_occupiedWithDangerZone.clear();
    m_passAroundEdge.clear();
    m_allArea.clear();

    m_absPosIsValid = false;

    m_absPos = absPosCenter;
    if (m_centerOffset != g_invalidPos)
        m_absPos = m_absPos->neighbourByOffset(FHPos{} - m_centerOffset);
    if (!m_absPos) {
        return false;
    }

    const auto visitMask     = m_object->getVisitableMask();
    const auto blockNotVisit = m_object->getBlockedUnvisitableMask();
    if (visitMask.empty()) {
        assert(!"Non-visitables not supported");
        return false;
    }
    MapTileRegion visitMaskRegion;
    MapTileRegion blockNotVisitRegion;

    {
        for (FHPos offset : visitMask) {
            auto* tile = m_absPos->neighbourByOffset(offset);
            if (!tile)
                return false;
            visitMaskRegion.insert(tile);
        }
        for (FHPos offset : blockNotVisit) {
            auto* tile = m_absPos->neighbourByOffset(offset);
            if (!tile)
                return false;
            blockNotVisitRegion.insert(tile);
        }
        m_rewardArea = visitMaskRegion.unionWith(blockNotVisitRegion);
        assert((visitMaskRegion.size() + blockNotVisitRegion.size()) > 0);
    }
    const MapTilePtr lastVisitTile   = visitMaskRegion[visitMaskRegion.size() - 1];
    const auto       rewardAreaOuter = m_rewardArea.makeOuterEdge(true);

    const bool isVisitable = m_object->getType() == IZoneObject::Type::Visitable;
    const bool isJoinable  = m_object->getType() == IZoneObject::Type::Joinable;

    if (m_useGuards) {
        MapTileRegion guardCandidates;
        if (m_pickable) {
            guardCandidates = rewardAreaOuter;
        } else if (isVisitable) {
            for (auto* tile : { lastVisitTile->m_neighborL, lastVisitTile->m_neighborR, lastVisitTile->m_neighborBL, lastVisitTile->m_neighborB, lastVisitTile->m_neighborBR }) {
                if (tile)
                    guardCandidates.insert(tile);
            }
            guardCandidates.erase(blockNotVisitRegion);
        }
        if (guardCandidates.empty())
            return false;

        auto it = std::min_element(guardCandidates.cbegin(), guardCandidates.cend(), [](MapTilePtr l, MapTilePtr r) {
            auto lHeat = l->m_zone->m_distances.getLevel(l);
            auto rHeat = r->m_zone->m_distances.getLevel(r);
            return std::tuple{ lHeat, l } < std::tuple{ rHeat, r };
        });

        m_guardAbsPos = *it;

        m_dangerZone.insert(m_guardAbsPos->m_allNeighboursWithDiag);
        m_dangerZone.insert(m_guardAbsPos);

        if (m_pickable) {
            m_extraObstacles = rewardAreaOuter;
            if (!isJoinable)
                m_extraObstacles.erase(m_dangerZone);
            m_unpassableArea = m_extraObstacles;
        }
    }

    if (isVisitable) {
        m_unpassableArea = m_rewardArea;
    }

    m_occupiedArea.insert(m_extraObstacles);
    m_occupiedArea.insert(m_rewardArea);
    if (m_guardAbsPos)
        m_occupiedArea.insert(m_guardAbsPos);

    m_dangerZone.erase(m_occupiedArea);

    m_occupiedWithDangerZone = m_occupiedArea;
    m_occupiedWithDangerZone.insert(m_dangerZone);
    for (auto* tile : m_occupiedWithDangerZone) {
        if (tile->m_orthogonalNeighbours.size() != 4) {
            return false;
        }
    }

    // if need 'breathing space' around it.
    const bool unguardedPickable = (m_pickable && !m_useGuards);
    if (!unguardedPickable)
        m_passAroundEdge = m_occupiedWithDangerZone.makeOuterEdge(false);

    m_allArea = m_occupiedWithDangerZone;
    m_allArea.insert(m_passAroundEdge);

    m_estimatedArea = m_occupiedWithDangerZone.size();
    if (m_estimatedArea <= 2)
        m_estimatedArea += m_passAroundEdge.size() / 2;
    else
        m_estimatedArea += m_passAroundEdge.size();
    //m_sizeInCells = m_occupiedWithDangerZone.splitByGrid(3, 3, 2).size();

    if (m_centerOffset == g_invalidPos)
        m_centerOffset = m_occupiedArea.makeCentroid(true)->m_pos - m_absPos->m_pos;

    m_absPosIsValid = true;
    return true;
}

std::string ZoneObjectWrap::toPrintableString() const
{
    std::ostringstream os;
    os << "id=" << m_object->getId() << ", heat=" << m_preferredHeat << "->" << m_placedHeat << ", size=" << m_estimatedArea;
    return os.str();
}

void ZoneObjectWrap::place() const
{
    m_object->place(m_absPos->m_pos);
}

bool ZoneObjectDistributor::makeInitialDistribution(DistributionResult& distribution, const ZoneObjectGeneration& generated) const
{
    Mernel::ProfilerScope scope("InitialDistribution");
    distribution.m_allOriginalIds = generated.m_allIds;
    size_t totalSize              = 0;
    //size_t                totalSizeCells = 0;
    for (ZoneSegment& seg : distribution.m_segments) {
        totalSize += seg.m_originalArea.size();
        //totalSizeCells += seg.m_originalAreaCells.size();
    }
    if (generated.m_objects.empty())
        return true;
    size_t totalSizeObjects = 0;
    //size_t             totalSizeObjectsCells = 0;
    ZoneObjectWrapPtrList segmentsNormal;
    for (auto& obj : generated.m_objects) {
        ZoneObjectWrap wrap{ {
            .m_object        = obj.m_object,
            .m_objectType    = obj.m_objectType,
            .m_preferredHeat = obj.m_preferredHeat,
            .m_useGuards     = obj.m_useGuards,
            .m_pickable      = obj.m_pickable,
            .m_radiusVector  = obj.m_radiusVector,
        } };
        wrap.estimateOccupied(m_tileContainer.m_centerTile);
        if (wrap.m_radiusVector != g_invalidPos)
            wrap.m_radiusVectorAbsPos = distribution.m_tileZone->m_centroid->m_pos + wrap.m_radiusVector;

        //m_logOutput << "m_radiusVectorAbsPos=" << wrap.m_radiusVectorAbsPos.toPrintableString() << "\n";

        totalSizeObjects += wrap.m_estimatedArea;
        distribution.m_allObjects.push_back(wrap);
    }

    for (ZoneObjectWrap& wrap : distribution.m_allObjects) {
        //m_logOutput << "g id=" << obj->getId() << "\n";
        if (wrap.m_objectType == ZoneObjectType::Segment) {
            if (!wrap.m_useGuards && wrap.m_pickable)
                distribution.m_candidateObjectsFreePickables.push_back(&wrap);
            else
                segmentsNormal.push_back(&wrap);
        }
        if (wrap.m_objectType == ZoneObjectType::Segment) {
            distribution.m_roadPickables.push_back(&wrap);
        }
    }

    m_logOutput << m_indent << "total tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    if (totalSizeObjects > totalSize) {
        m_logOutput << m_indent << "too many generated objects to fit in area!\n";
        return false;
    }
    std::sort(segmentsNormal.begin(), segmentsNormal.end(), [](ZoneObjectWrap* r, ZoneObjectWrap* l) {
        return std::tuple{ r->m_preferredHeat, r } < std::tuple{ l->m_preferredHeat, l };
    });

    ZoneObjectWrapPtrList segmentsNormalUnfit;
    for (ZoneObjectWrap* object : segmentsNormal) {
        int minHeatAvailableInAllSegments = distribution.m_maxHeat;

        std::set<ZoneSegment*> segCandidates;
        for (ZoneSegment& seg : distribution.m_segments) {
            if (seg.m_freeArea.size() < object->m_estimatedArea)
                continue;
            for (const auto& [heat, data] : seg.m_heatMap) {
                if (data.m_free.size() > 0) {
                    minHeatAvailableInAllSegments = std::min(minHeatAvailableInAllSegments, heat);
                    break;
                }
            }
            segCandidates.insert(&seg);
        }
        if (minHeatAvailableInAllSegments == distribution.m_maxHeat)
            throw std::runtime_error("sanity check failed: no heat");

        object->m_placedHeat = std::max(object->m_preferredHeat, minHeatAvailableInAllSegments);

        std::map<int, std::vector<ZoneSegment*>> segCandidatesByHeat;
        for (ZoneSegment* seg : segCandidates) {
            auto* heatData = seg->findBestHeatData(object->m_placedHeat);
            if (!heatData)
                throw std::runtime_error("bad logic. we already determined that segCandidates is no empty");
            segCandidatesByHeat[heatData->m_heat].push_back(seg);
        }
        bool placeSucceed = false;
        for (auto& [heat, segCandidates2] : segCandidatesByHeat) {
            if (placeWrapIntoSegments(distribution, object, segCandidates2)) {
                placeSucceed = true;
                break;
            }
        }

        if (!placeSucceed) {
            // can happen if we need to place 10-tile object and we have remaining two segment 5-tile each.
            // totalSizeObjects > totalSize check still pass.
            m_logOutput << m_indent << "Failed to find free segment to fit object= " << object->toPrintableString() << "\n";
            if (object->m_absPos) {
                auto* tz = object->m_absPos->m_zone;
                tz->m_rewardTilesFailure.insert(object->m_occupiedWithDangerZone);
            }
            //return false;
        }
    }

    return true;
}

void ZoneObjectDistributor::doPlaceDistribution(DistributionResult& distribution) const
{
    if (distribution.m_allObjects.empty())
        return;

    MergedRegion totalFreeTiles;

    for (ZoneSegment& seg : distribution.m_segments) {
        if (seg.m_successNormal.empty())
            continue;
        for (auto* object : seg.m_successNormal)
            commitPlacement(distribution, object, &seg);
    }
    //for (ZoneObjectWrap* object : distribution.m_candidateObjectsFreePickables) {
    //distribution.m_tileZone->m_rewardTilesCenters;
    //}
    //std::map<int, MapTileRegion> roadsByRegion = distribution.m_tileZone->m_roadHeat.m_byLevel;
    //for (ZoneObjectWrap* object : distribution.m_roadPickables) {
    //    roadsByRegion
    //}
}

bool ZoneObjectDistributor::placeWrapIntoSegments(DistributionResult& distribution, ZoneObjectWrap* object, std::vector<ZoneSegment*>& segCandidates) const
{
    if (segCandidates.empty()) {
        return false;
    }

    //ZoneSegment* fitSeg = *segCandidates.begin();
    std::vector<std::pair<int64_t, ZoneSegment*>> segCandidatesSorted;

    if (object->m_radiusVectorAbsPos != g_invalidPos) {
        const FHPos outOfMapRVectorPos = object->m_radiusVectorAbsPos;

        for (ZoneSegment* seg : segCandidates) {
            const auto distance = posDistance(seg->m_originalAreaCentroid->m_pos, outOfMapRVectorPos);
            //m_logOutput << "distance " << seg->m_originalAreaCentroid->m_pos.toPrintableString() << "<-> " << outOfMapRVectorPos.toPrintableString() << " = " << distance << "\n";
            segCandidatesSorted.push_back(std::pair{ distance, seg });
        }
    } else {
        for (ZoneSegment* seg : segCandidates) {
            segCandidatesSorted.push_back(std::pair{ int64_t(0), seg });
        }
    }
    std::sort(segCandidatesSorted.begin(), segCandidatesSorted.end());

    for (auto& [_, seg] : segCandidatesSorted) {
        MapTilePtrList placementTileCandidates; // unsorted! important! we do not want address order.

        std::vector<std::pair<int, MapTilePtr>> tileCandidatesSorted;

        for (auto& [heat, data] : seg->m_heatMap) {
            for (auto* tile : data.m_free) {
                int distance = seg->m_distances.at(tile);
                tileCandidatesSorted.push_back(std::pair{ distance, tile });
            }
        }

        std::sort(tileCandidatesSorted.begin(), tileCandidatesSorted.end());
        for (auto& [__, tile] : tileCandidatesSorted) {
            if (object->estimateOccupied(tile)) {
                if (seg->m_freeArea.intersectWith(object->m_occupiedWithDangerZone) == object->m_occupiedWithDangerZone) {
                    seg->m_freeArea.erase(object->m_allArea);

                    seg->m_freeArea.eraseExclaves(false);

                    seg->m_successNormal.push_back(object);
                    object->m_placedHeat = seg->m_tileZone->m_segmentHeat.getLevel(object->m_absPos);
                    seg->recalcHeat();
                    return true;
                }
            }
        }
    }

    return false;
}

void ZoneObjectDistributor::commitPlacement(DistributionResult& distribution, ZoneObjectWrap* object, ZoneSegment* seg) const
{
    object->place();
    //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = seg.m_originalAreaCentroid->m_pos, .m_text = std::to_string(seg.m_segmentIndex) });

    auto* tz = object->m_absPos->m_zone;
    if (0) {
        tz->m_rewardTilesDanger.insert(object->m_dangerZone);
        tz->m_rewardTilesMain.insert(object->m_rewardArea);
        tz->m_rewardTilesSpacing.insert(object->m_passAroundEdge);
        tz->m_rewardTilesPos.insert(object->m_absPos);
        tz->m_rewardTilesCenters.insert(object->m_absPos->neighbourByOffset(object->m_centerOffset));
        //tz->m_rewardTilesHints.insert(posHint);
    }

    distribution.m_needBlock.insert(object->m_extraObstacles);

    distribution.m_placedIds.push_back(object->m_object->getId());

    if (object->m_useGuards) {
        Guard guard;
        guard.m_value = object->m_object->getGuard();
        guard.m_pos   = object->m_guardAbsPos;
        distribution.m_guards.push_back(guard);
    }
}

void ZoneObjectDistributor::DistributionResult::init(TileZone& tileZone)
{
    MapTileRegion safePadding = tileZone.m_unpassableArea.makeOuterEdge(false);
    if (tileZone.m_innerAreaSegments.empty())
        throw std::runtime_error("No segments in tile zone!");

    m_maxHeat  = tileZone.m_rngZoneSettings.m_maxHeat;
    m_tileZone = &tileZone;

    m_segments.clear();
    for (size_t index = 0; auto& seg : tileZone.m_innerAreaSegments) {
        ZoneSegment zs;
        zs.m_originalArea = seg.m_innerArea;
        zs.m_freeArea     = zs.m_originalArea;
        zs.m_freeArea.erase(safePadding);
        zs.m_freeArea.eraseExclaves(false);
        zs.m_originalAreaCentroid = zs.m_originalArea.makeCentroid(true);
        zs.m_segmentIndex         = index++;
        zs.m_tileZone             = &tileZone;
        zs.recalcHeat();
        for (auto* tile : zs.m_freeArea) {
            zs.m_distances[tile] = zs.m_tileZone->m_distances.getLevel(tile);
        }

        m_segments.push_back(std::move(zs));
    }
}

std::string ZoneObjectDistributor::ZoneSegment::toPrintableString() const
{
    std::ostringstream os;
    os << "[" << m_segmentIndex << "] tot: " << m_freeArea.size() << ", heats: {";

    for (auto& [heat, value] : m_heatMap) {
        if (!value.m_centroid)
            continue;
        os << heat << ": " << value.m_free.size() << ", ";
    }
    os << "}, objs:" << m_successNormal.size();
    return os.str();
}

ZoneObjectDistributor::ZoneSegment::HeatDataItem* ZoneObjectDistributor::ZoneSegment::findBestHeatData(int heat)
{
    if (m_heatMap.empty())
        return nullptr;

    auto it = m_heatMap.find(heat);
    if (it != m_heatMap.end() && it->second.m_centroid)
        return &(it->second);

    for (auto it2 = m_heatMap.begin(); it2 != m_heatMap.end(); ++it2) {
        if (it2->second.m_centroid) {
            return &(it2->second);
        }
    }
    return nullptr;
}

void ZoneObjectDistributor::ZoneSegment::compactIfNeeded()
{
    /*
    if (m_compact)
        return;
    m_compact = true;
    std::sort(m_successNormal.begin(), m_successNormal.end(), [this](ZoneObjectWrap* l, ZoneObjectWrap* r) {
        auto lDistance = m_tileZone->m_distances.getLevel(l->m_absPos);
        auto rDistance = m_tileZone->m_distances.getLevel(r->m_absPos);

        return std::tuple{ l->m_preferredHeat, lDistance, l } < std::tuple{ r->m_preferredHeat, rDistance, r };
    });
    for (ZoneObjectWrap* object : m_successNormal) {
        recalcFree(object);
        auto result = placeOnMap(*object, object->m_absPos->neighbourByOffset(object->m_centerOffset), true);
        if (result != PlacementResult::Success) {
            //m_logOutput << m_indent << " AAAA: " << object->toPrintableString() << "\n";
            throw std::runtime_error("Successful object must be successful again after shift! " + std::to_string(int(result)));
        }
    }
    recalcFree();*/
}

void ZoneObjectDistributor::ZoneSegment::commitPlacement(DistributionResult& distribution, ZoneObjectWrap* object)
{
    this->m_freeArea.erase(object->m_allArea);
}

void ZoneObjectDistributor::ZoneSegment::recalcFree(ZoneObjectWrap* exclude)
{
    m_freeArea = m_originalArea;
    for (ZoneObjectWrap* nobject : m_successNormal) {
        if (nobject != exclude)
            m_freeArea.erase(nobject->m_allArea);
    }
}

void ZoneObjectDistributor::ZoneSegment::recalcHeat()
{
    for (auto& [heat, data] : m_heatMap) {
        data.m_free.clear();
        data.m_centroid = nullptr;
        data.m_heat     = heat;
    }
    for (auto* tile : m_freeArea) {
        int   heat = m_tileZone->m_segmentHeat.getLevel(tile);
        auto& m    = m_heatMap[heat];
        m.m_free.insert(tile);
    }
    for (auto& [heat, data] : m_heatMap) {
        data.m_centroid = data.m_free.makeCentroid(true);
    }
}

}
