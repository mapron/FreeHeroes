/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "ZoneObjectDistributor.hpp"
#include "TileZone.hpp"
#include "MapTileRegionSegmentation.hpp"

#include "IRandomGenerator.hpp"

#include "MernelPlatform/Profiler.hpp"
#include "MernelReflection/EnumTraitsMacro.hpp"

#include <iostream>
#include <sstream>

namespace Mernel::Reflection {

ENUM_REFLECTION_STRINGIFY(FreeHeroes::ZoneObjectDistributor::PlacementResult,
                          Success,
                          Success,
                          InsufficientSpaceInSource,
                          EstimateOccupiedFailure,
                          InvalidShiftValue,
                          InvalidCollisionInputs,
                          CollisionImpossibleShift,
                          CollisionHasShift,
                          RunOutOfShiftRetries,
                          ShiftLoopDetected);

}

namespace FreeHeroes {

namespace {

std::string placementResultToString(FreeHeroes::ZoneObjectDistributor::PlacementResult value)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(value);
    return std::string(str.begin(), str.end());
}

// clang-format off
const std::vector<FHPos> g_deltasToTry{ 
    FHPos{ -1,  0 }, FHPos{ +1,  0 }, FHPos{  0, -1 }, FHPos{  0, +1 },
    FHPos{ -1, -1 }, FHPos{ +1, -1 }, FHPos{ +1, +1 }, FHPos{ -1, +1 },
};
// clang-format on

MapTileRegion blurSet(const MapTileRegion& source, bool diag, bool excludeOriginal = true)
{
    if (source.empty())
        return {};

    MapTileRegion result;
    result.reserve(source.size() * 3);
    for (auto pos : source) {
        result.insert(pos);
        result.insert(pos->neighboursList(diag));
    }
    if (excludeOriginal) {
        result.erase(source);
    }
    return result;
}

}

bool ZoneObjectWrap::estimateOccupied(MapTilePtr absPos, MapTilePtr centroid)
{
    if (!absPos)
        return false;

    auto guardPosition = posDirectionTo(centroid->m_pos, absPos->m_pos);
    if (guardPosition == FHPosDirection::Invalid)
        guardPosition = FHPosDirection::B;

    if (m_absPosIsValid && m_guardPosition == guardPosition) {
        return estimateShift(absPos);
    }

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

    m_absPos        = absPos;
    m_guardPosition = guardPosition;

    const auto visitMask     = m_zoneObject->getVisitableMask();
    const auto blockNotVisit = m_zoneObject->getBlockedUnvisitableMask();
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
    const MapTilePtr lastVisitTile      = visitMaskRegion[visitMaskRegion.size() - 1];
    const auto       rewardAreaCentroid = m_rewardArea.makeCentroid(true);
    const auto       rewardAreaOuter    = m_rewardArea.makeOuterEdge(true);

    const bool isVisitable = m_zoneObject->getType() == IZoneObject::Type::Visitable;
    const bool isPickable  = m_zoneObject->getType() == IZoneObject::Type::Pickable;

    if (m_isGuarded) {
        MapTileRegion guardCandidates;
        if (isPickable) {
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

        m_guardAbsPos = guardCandidates.findClosestPoint(rewardAreaCentroid->m_pos + radiusVector(guardPosition));

        m_dangerZone.insert(m_guardAbsPos->m_allNeighboursWithDiag);
        m_dangerZone.insert(m_guardAbsPos);

        if (isPickable) {
            m_extraObstacles = rewardAreaOuter;
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
    const bool unguardedPickable = (isPickable && !m_isGuarded);
    if (!unguardedPickable)
        m_passAroundEdge = m_occupiedWithDangerZone.makeOuterEdge(false);

    m_allArea = m_occupiedWithDangerZone;
    m_allArea.insert(m_passAroundEdge);

    m_sizeInCells = m_occupiedWithDangerZone.splitByGrid(3, 3, 2).size();

    m_centerOffset = m_occupiedWithDangerZone.makeCentroid(true)->m_pos - m_absPos->m_pos;

    m_absPosIsValid = true;
    return true;
}

bool ZoneObjectWrap::estimateShift(MapTilePtr absPos)
{
    //Mernel::ProfilerScope scope("estimate shift");
    auto delta      = absPos->m_pos - m_absPos->m_pos;
    m_absPos        = absPos;
    m_absPosIsValid = false;

    auto shiftArea = [delta](MapTileRegion& area) {
        bool result = true;
        area.updateAllValues([delta, &result](MapTilePtr tile) {
            tile = tile->neighbourByOffset(delta);
            if (!tile) {
                result = false;
            }
            return tile;
        });
        return result;
    };

    if (!shiftArea(m_rewardArea)
        || !shiftArea(m_extraObstacles)
        || !shiftArea(m_unpassableArea)
        || !shiftArea(m_occupiedArea)

        || !shiftArea(m_dangerZone)
        || !shiftArea(m_occupiedWithDangerZone)
        || !shiftArea(m_passAroundEdge)
        || !shiftArea(m_allArea))
        return false;

    if (m_guardAbsPos) {
        m_guardAbsPos = m_guardAbsPos->neighbourByOffset(delta);
        if (!m_guardAbsPos)
            return false;
    }

    m_absPosIsValid = true;
    return true;
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
    size_t totalSizeObjects = 0;
    //size_t             totalSizeObjectsCells = 0;
    ZoneObjectWrapList segmentsNormal;
    for (auto& obj : generated.m_segmentsGuarded) {
        ZoneObjectWrap wrap;
        wrap.m_isGuarded  = true;
        wrap.m_zoneObject = obj;
        wrap.estimateOccupied(m_tileContainer.m_centerTile, m_tileContainer.m_centerTile);
        totalSizeObjects += wrap.getEstimatedArea();
        //totalSizeObjectsCells += wrap.m_sizeInCells;
        //m_logOutput << "g id=" << obj->getId() << "\n";
        segmentsNormal.push_back(wrap);
    }
    m_logOutput << m_indent << "total tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    for (auto& obj : generated.m_segmentsUnguardedNonPickables) {
        ZoneObjectWrap wrap;
        wrap.m_isGuarded  = false;
        wrap.m_zoneObject = obj;
        wrap.estimateOccupied(m_tileContainer.m_centerTile, m_tileContainer.m_centerTile);
        totalSizeObjects += wrap.getEstimatedArea();
        //totalSizeObjectsCells += wrap.m_sizeInCells;
        //m_logOutput << "u id=" << obj->getId() << "\n";
        segmentsNormal.push_back(wrap);
    }
    m_logOutput << m_indent << "total tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    // m_segmentsUnguardedPickables is too much rn, split to roads later

    for (auto& obj : generated.m_segmentsUnguardedPickables) {
        ZoneObjectWrap wrap;
        wrap.m_isGuarded  = false;
        wrap.m_zoneObject = obj;
        wrap.estimateOccupied(m_tileContainer.m_centerTile, m_tileContainer.m_centerTile);
        totalSizeObjects += wrap.getEstimatedArea();
        distribution.m_candidateObjectsFreePickables.push_back(wrap);
    }

    m_logOutput << m_indent << "total tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    if (totalSizeObjects > totalSize) {
        m_logOutput << m_indent << "too many generated objects to fit in area!\n";
        return false;
    }
    for (ZoneObjectWrap& wrap : segmentsNormal) {
        const size_t remainingSizeToFit            = wrap.getEstimatedArea();
        int          minHeatAvailableInAllSegments = 10;
        for (ZoneSegment& seg : distribution.m_segments) {
            if (seg.m_freeAreaByHeatTotal < remainingSizeToFit)
                continue;
            for (const auto& [h, cnt] : seg.m_freeAreaByHeat) {
                if (cnt > 0) {
                    minHeatAvailableInAllSegments = std::min(minHeatAvailableInAllSegments, h);
                    break;
                }
            }
        }
        if (minHeatAvailableInAllSegments == 10)
            throw std::runtime_error("sanity check failed: no heat");

        const int currentHeat = minHeatAvailableInAllSegments;
        wrap.m_preferredHeat  = currentHeat;

        bool fitInSomeSeg = false;
        for (ZoneSegment& seg : distribution.m_segments) {
            auto it = seg.m_freeAreaByHeat.find(wrap.m_preferredHeat);
            if (it == seg.m_freeAreaByHeat.cend() || it->second <= 0)
                continue;

            if (seg.removeHeatSize(remainingSizeToFit, wrap.m_preferredHeat)) {
                fitInSomeSeg = true;
                seg.m_candidateObjectsNormal.push_back(wrap);
                break;
            }
        }
        if (!fitInSomeSeg) {
            // can happen if we need to place 10-tile object and we have remaining two segment 5-tile each.
            m_logOutput << m_indent << "Failed to find free segment to fit object\n";
            return false;
        }
    }

    return true;
}

bool ZoneObjectDistributor::doPlaceDistribution(DistributionResult& distribution) const
{
    ZoneObjectWrapList failedPlacement;
    size_t             totalObjects = 0;

    auto placeOnMapWrap = [this, &failedPlacement, &totalObjects, &distribution](ZoneObjectWrap& object, ZoneSegment& seg, MapTilePtr posHint) {
        totalObjects++;
        PlacementResult lastResult;
        if ((lastResult = placeOnMap(object, seg.m_freeArea, seg.m_originalAreaCentroid, posHint)) == PlacementResult::Success) {
            object.m_zoneObject->place(object.m_absPos->m_pos);
            seg.m_freeArea.erase(object.m_allArea);

            distribution.m_placedIds.push_back(object.m_zoneObject->getId());

            if (object.m_isGuarded) {
                Guard guard;
                guard.m_value = object.m_zoneObject->getGuard();
                guard.m_pos   = object.m_guardAbsPos;
                distribution.m_guards.push_back(guard);
            }
            return;
        }

        failedPlacement.push_back(object);
        m_logOutput << m_indent << " placement failure (" << placementResultToString(lastResult) << "): pos=" << (object.m_absPos ? object.m_absPos->toPrintableString() : "NULL") << " freeArea=" << seg.m_freeArea.size() << " objectSize=" << object.getEstimatedArea() << "; " << object.m_zoneObject->getId() << "\n";

        /*
        MergedRegion reg;
        seg.m_freeArea.erase(posHint);
        
        reg.initFromTile(seg.m_freeArea[0]);
        reg.m_regions['O'] = seg.m_freeArea;
        std::cout << reg.dump();*/

        /*
       
        if (0) {
            const auto [collisionResult, newPossibleShift] = MapTileRegionWithEdge::getCollisionShiftForObject(bundle.m_occupiedWithDangerZone, bundle.m_lastCellSource, true);
            m_logOutput << m_indent << prefix << " collisionResult=" << int(collisionResult) << " \n";
            std::string debug;
            MapTileRegion::compose(&m_tileContainer, bundle.m_occupiedWithDangerZone, bundle.m_lastCellSource, debug, true, true);
            m_logOutput << debug;
        }*/
    };

    for (ZoneSegment& seg : distribution.m_segments) {
        if (seg.m_candidateObjectsNormal.empty())
            continue;

        //for (auto& obj : seg.m_candidateObjectsNormal) {
        //    m_logOutput << m_indent << "placing " << obj.m_zoneObject->getId() << "\n";
        //}

        size_t count    = seg.m_candidateObjectsNormal.size();
        auto   settings = MapTileRegionSegmentation::guessKMeansByGrid(seg.m_originalArea, count);
        for (size_t i = 0; i < count; ++i) {
            settings.m_items[i].m_areaHint = seg.m_candidateObjectsNormal[i].getEstimatedArea();
        }
        auto objectRegions = seg.m_originalArea.splitByKExt(settings);
        for (size_t i = 0; i < count; ++i) {
            ZoneObjectWrap& object         = seg.m_candidateObjectsNormal[i];
            MapTilePtr      posHint        = objectRegions[i].makeCentroid(true);
            MapTilePtr      posHintShifted = posHint->neighbourByOffset(FHPos{} - object.m_centerOffset);
            if (!posHintShifted) {
                posHintShifted = m_tileContainer.m_all.findClosestPoint(posHint->m_pos - object.m_centerOffset);
            }
            placeOnMapWrap(object, seg, posHintShifted);
        }

        //m_logOutput << m_indent << prefix << " placement failure (" << int(lastResult) << ") [" << i << "]: pos=" << (bundle.m_absPos ? bundle.m_absPos->toPrintableString() : "NULL") << " size=" << bundle.getEstimatedArea() << "; " << bundle.toPrintableString() << "\n";
    }

    if (!failedPlacement.empty()) {
        m_logOutput << m_indent << " placement failure=" << failedPlacement.size() << " of " << totalObjects << " items\n";
        return true;
    }

    return true;
}

ZoneObjectDistributor::PlacementResult ZoneObjectDistributor::placeOnMap(ZoneObjectWrap& bundle,
                                                                         MapTileRegion&  region,
                                                                         MapTilePtr      centroid,
                                                                         MapTilePtr      posHint) const
{
    Mernel::ProfilerScope scope("placeOnMap");

    if (region.size() < bundle.getEstimatedArea())
        return PlacementResult::InsufficientSpaceInSource;

    MapTilePtr pos = posHint;
    assert(pos);

    FHPos newPossibleShift = g_invalidPos;

    auto tryPlaceInner = [&pos, &bundle, &region, centroid, &newPossibleShift]() -> PlacementResult {
        Mernel::ProfilerScope scope("placeInner");
        if (!bundle.estimateOccupied(pos, centroid))
            return PlacementResult::EstimateOccupiedFailure;

        if (region.size() < bundle.getEstimatedArea())
            return PlacementResult::InsufficientSpaceInSource;

        MapTileRegionWithEdge::CollisionResult collisionResult = MapTileRegionWithEdge::CollisionResult::InvalidInputs;

        std::tie(collisionResult, newPossibleShift) = MapTileRegionWithEdge::getCollisionShiftForObject(bundle.m_occupiedWithDangerZone, region, true);
        if (collisionResult == MapTileRegionWithEdge::CollisionResult::NoCollision)
            return PlacementResult::Success;

        if (collisionResult == MapTileRegionWithEdge::CollisionResult::InvalidInputs)
            return PlacementResult::InvalidCollisionInputs;

        if (collisionResult == MapTileRegionWithEdge::CollisionResult::ImpossibleShift)
            return PlacementResult::CollisionImpossibleShift;

        return PlacementResult::CollisionHasShift;
    };

    PlacementResult lastResult;
    // if first attempt is succeed, we'll try to find near place where we don't fit, and then backup to more close fit.
    if ((lastResult = tryPlaceInner()) == PlacementResult::Success && 0) {
        auto originalPos = pos;
        for (FHPos delta : g_deltasToTry) {
            pos = originalPos;
            for (int i = 0; i < 3; ++i) {
                Mernel::ProfilerScope scope1("reposition");
                pos = pos->neighbourByOffset(delta);
                if (!pos)
                    break;

                // if we found that shifting to neighbor stop make us fail...
                if (tryPlaceInner() != PlacementResult::Success) {
                    // ...backup to previous successful spot
                    pos = pos->neighbourByOffset(FHPos{} - delta);
                    assert(pos);
                    return tryPlaceInner();
                }
            }
        }
        pos = originalPos;
        assert(pos);
        return tryPlaceInner();
    }
    //Mernel::ProfilerScope scope1("failed");
    if (lastResult == PlacementResult::EstimateOccupiedFailure) {
        for (int i = 0; i < 5; ++i) {
            MapTileRegion neigh(pos->m_allNeighboursWithDiag);
            pos = neigh.findClosestPoint(m_tileContainer.m_centerTile->m_pos);
            assert(pos);

            if ((lastResult = tryPlaceInner()) == PlacementResult::Success) {
                return lastResult;
            }
            if (lastResult != PlacementResult::EstimateOccupiedFailure)
                break;
        }
    }
    if (lastResult == PlacementResult::CollisionHasShift) {
        //Mernel::ProfilerScope scope2("hasShift");
        MapTileRegion used;
        used.insert(pos);
        for (int i = 0; i < 3; ++i) {
            auto newPos = pos->neighbourByOffset(newPossibleShift);
            if (!newPos)
                return PlacementResult::InvalidShiftValue;
            if (used.contains(newPos))
                return PlacementResult::ShiftLoopDetected;

            pos = newPos;
            used.insert(pos);

            if ((lastResult = tryPlaceInner()) == PlacementResult::Success) {
                //Mernel::ProfilerScope scope3("deltashelp");
                return lastResult;
            }
            //Mernel::ProfilerScope scope3("deltasNOThelp");
            if (lastResult != PlacementResult::CollisionHasShift)
                return lastResult;
        }
        return PlacementResult::RunOutOfShiftRetries;
    }

    return lastResult;
}

void ZoneObjectDistributor::DistributionResult::init(TileZone& tileZone)
{
    MapTileRegion safePadding = tileZone.m_nodes.getRegion(RoadLevel::Towns);
    {
        safePadding = blurSet(safePadding, true, false);
    }
    if (tileZone.m_innerAreaSegments.empty())
        throw std::runtime_error("No segments in tile zone!");

    m_segments.clear();
    for (auto& seg : tileZone.m_innerAreaSegments) {
        ZoneSegment zs;
        zs.m_originalArea = seg.m_innerArea;
        zs.m_originalArea.erase(safePadding); // @todo: move this to segmentation phase
        zs.m_freeArea             = zs.m_originalArea;
        zs.m_originalAreaCells    = zs.m_originalArea.splitByGrid(3, 3, 2);
        zs.m_freeAreaByHeatTotal  = zs.m_originalArea.size();
        zs.m_originalAreaCentroid = zs.m_originalArea.makeCentroid(true);

        for (auto* tile : zs.m_originalArea) {
            int heat = tileZone.m_segmentHeat.getLevel(tile);
            zs.m_freeAreaByHeat[heat]++;
        }

        m_segments.push_back(std::move(zs));
    }
}

bool ZoneObjectDistributor::ZoneSegment::removeHeatSize(size_t size, int startingHeat)
{
    if (m_freeAreaByHeatTotal < size)
        return false;
    m_freeAreaByHeatTotal -= size;

    for (auto& [heat, value] : m_freeAreaByHeat) {
        if (heat < startingHeat)
            continue;

        if (size <= value) {
            value -= size;
            break;
        }
        size -= value;
        value = 0;
    }
    return true;
}

}
