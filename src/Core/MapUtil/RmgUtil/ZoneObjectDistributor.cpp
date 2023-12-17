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

bool ZoneObjectWrap::estimateOccupied(MapTilePtr absPos)
{
    if (!absPos)
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

    m_absPos = absPos;

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
            auto lHeat = l->m_zone->m_segmentHeat.getLevel(l);
            auto rHeat = r->m_zone->m_segmentHeat.getLevel(r);
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

    m_sizeInCells = m_occupiedWithDangerZone.splitByGrid(3, 3, 2).size();

    m_centerOffset = m_occupiedWithDangerZone.makeCentroid(true)->m_pos - m_absPos->m_pos;

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
    for (auto& obj : generated.m_objects) {
        ZoneObjectWrap wrap{ {
            .m_object        = obj.m_object,
            .m_objectType    = obj.m_objectType,
            .m_preferredHeat = obj.m_preferredHeat,
            .m_useGuards     = obj.m_useGuards,
            .m_pickable      = obj.m_pickable,
        } };
        wrap.estimateOccupied(m_tileContainer.m_centerTile);
        totalSizeObjects += wrap.getEstimatedArea();
        //m_logOutput << "g id=" << obj->getId() << "\n";
        if (wrap.m_objectType == ZoneObjectType::Segment) {
            if (!wrap.m_useGuards && wrap.m_pickable)
                distribution.m_candidateObjectsFreePickables.push_back(wrap);
            else
                segmentsNormal.push_back(wrap);
        }
    }

    m_logOutput << m_indent << "total tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    if (totalSizeObjects > totalSize) {
        m_logOutput << m_indent << "too many generated objects to fit in area!\n";
        return false;
    }
    for (ZoneObjectWrap& wrap : segmentsNormal) {
        const size_t remainingSizeToFit = wrap.getEstimatedArea();

        int minHeatAvailableInAllSegments = distribution.m_maxHeat;
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
        if (minHeatAvailableInAllSegments == distribution.m_maxHeat)
            throw std::runtime_error("sanity check failed: no heat");

        const int currentHeat = minHeatAvailableInAllSegments;
        wrap.m_preferredHeat  = std::max(wrap.m_preferredHeat, currentHeat);
        m_logOutput << m_indent << "wrap id=" << wrap.m_object->getId() << ", heat=" << wrap.m_preferredHeat << ", size=" << remainingSizeToFit << "\n";

        std::vector<std::pair<ZoneSegment*, size_t>> segCandidates;
        for (ZoneSegment& seg : distribution.m_segments) {
            auto it = seg.m_freeAreaByHeat.find(wrap.m_preferredHeat);
            if (it == seg.m_freeAreaByHeat.cend() || it->second <= 0)
                continue;
            if (seg.m_freeAreaByHeatTotal < remainingSizeToFit)
                continue;
            segCandidates.push_back(std::pair{ &seg, it->second });

            //m_logOutput << m_indent << "skipped " << seg.toPrintableString() << "\n";
        }
        if (segCandidates.empty()) {
            // can happen if we need to place 10-tile object and we have remaining two segment 5-tile each.
            m_logOutput << m_indent << "Failed to find free segment to fit object\n";
            return false;
        }
        auto         it     = std::min_element(segCandidates.cbegin(), segCandidates.cend(), [](const auto& l, const auto& r) {
            return std::tuple{ l.second, l.first->m_segmentIndex } < std::tuple{ r.second, r.first->m_segmentIndex };
        });
        ZoneSegment* fitSeg = it->first;
        fitSeg->removeHeatSize(remainingSizeToFit, wrap.m_preferredHeat);
        m_logOutput << m_indent << "    fit into " << fitSeg->toPrintableString() << "\n";
        fitSeg->m_candidateObjectsNormal.push_back(wrap);
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
        if ((lastResult = placeOnMap(object, seg, posHint)) == PlacementResult::Success) {
            object.m_object->place(object.m_absPos->m_pos);
            seg.m_freeArea.erase(object.m_allArea);

            //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = seg.m_originalAreaCentroid->m_pos, .m_text = std::to_string(seg.m_segmentIndex) });

            int paletteSize = distribution.m_maxHeat;
            int heatLevel   = object.m_preferredHeat;
            if (0) {
                m_map.m_debugTiles.push_back(FHDebugTile{
                    .m_pos         = object.m_absPos->m_pos,
                    .m_penColor    = heatLevel + 1, // heatLevel is 0-based
                    .m_penPalette  = paletteSize,
                    .m_shape       = 1,
                    .m_shapeRadius = 4,
                });
            }

            auto* tz = object.m_absPos->m_zone;
            tz->m_rewardTilesDanger.insert(object.m_dangerZone);
            if (0) {
                tz->m_rewardTilesMain.insert(object.m_rewardArea);
                tz->m_rewardTilesSpacing.insert(object.m_passAroundEdge);
                tz->m_rewardTilesPos.insert(object.m_absPos);
                tz->m_rewardTilesCenters.insert(object.m_absPos->neighbourByOffset(object.m_centerOffset));
            }

            tz->m_rewardTilesHints.insert(posHint);
            distribution.m_needBlock.insert(object.m_extraObstacles);

            distribution.m_placedIds.push_back(object.m_object->getId());

            if (object.m_useGuards) {
                Guard guard;
                guard.m_value = object.m_object->getGuard();
                guard.m_pos   = object.m_guardAbsPos;
                distribution.m_guards.push_back(guard);
            }
            return;
        }
        auto* tz = object.m_absPos->m_zone;
        tz->m_rewardTilesFailure.insert(object.m_occupiedWithDangerZone);
        //tz->m_rewardTilesPos.insert(object.m_absPos);
        //tz->m_rewardTilesCenters.insert(object.m_absPos->neighbourByOffset(object.m_centerOffset));
        //tz->m_rewardTilesHints.insert(posHint);

        failedPlacement.push_back(object);
        m_logOutput << m_indent << " placement failure (" << placementResultToString(lastResult) << "): pos=" << (object.m_absPos ? object.m_absPos->toPrintableString() : "NULL") << " freeArea=" << seg.m_freeArea.size() << " objectSize=" << object.getEstimatedArea() << "; " << object.m_object->getId() << "\n";

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
        //    m_logOutput << m_indent << "placing " << obj.m_object->getId() << "\n";
        //}

        size_t count    = seg.m_candidateObjectsNormal.size();
        auto   settings = MapTileRegionSegmentation::guessKMeansByGrid(seg.m_originalArea, count);

        std::set<size_t> remainingFragmentIndices;
        for (size_t i = 0; i < count; ++i)
            remainingFragmentIndices.insert(i);
        for (size_t i = 0; i < count; ++i) {
            auto& obj                   = seg.m_candidateObjectsNormal[i];
            auto* preferredHeatCentroid = seg.m_heatCentroids.at(obj.m_preferredHeat);

            auto   it        = std::min_element(remainingFragmentIndices.begin(), remainingFragmentIndices.end(), [&settings, preferredHeatCentroid](size_t l, size_t r) {
                auto lCentroid = settings.m_items[l].m_initialCentroid;
                auto rCentroid = settings.m_items[r].m_initialCentroid;
                auto lDistance = posDistance(lCentroid, preferredHeatCentroid, 100);
                auto rDistance = posDistance(rCentroid, preferredHeatCentroid, 100);
                return std::tuple{ lDistance, l } < std::tuple{ rDistance, r };
            });
            size_t bestIndex = *it;
            remainingFragmentIndices.erase(it);
            obj.m_segmentFragmentIndex             = bestIndex;
            settings.m_items[bestIndex].m_areaHint = obj.getEstimatedArea();
        }
        auto objectRegions = seg.m_originalArea.splitByKExt(settings);
        for (ZoneObjectWrap& object : seg.m_candidateObjectsNormal) {
            MapTilePtr posHint        = objectRegions[object.m_segmentFragmentIndex].makeCentroid(true);
            MapTilePtr posHintShifted = posHint->neighbourByOffset(FHPos{} - object.m_centerOffset);
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
                                                                         ZoneSegment&    seg,
                                                                         MapTilePtr      posHint) const
{
    Mernel::ProfilerScope scope("placeOnMap");

    if (seg.m_freeArea.size() < bundle.getEstimatedArea())
        return PlacementResult::InsufficientSpaceInSource;

    MapTilePtr pos = posHint;
    assert(pos);

    FHPos newPossibleShift = g_invalidPos;

    auto tryPlaceInner = [&bundle, &seg, &newPossibleShift](MapTilePtr pos) -> PlacementResult {
        Mernel::ProfilerScope scope("placeInner");
        if (!bundle.estimateOccupied(pos))
            return PlacementResult::EstimateOccupiedFailure;

        if (seg.m_freeArea.size() < bundle.getEstimatedArea())
            return PlacementResult::InsufficientSpaceInSource;

        MapTileRegionWithEdge::CollisionResult collisionResult = MapTileRegionWithEdge::CollisionResult::InvalidInputs;

        std::tie(collisionResult, newPossibleShift) = MapTileRegionWithEdge::getCollisionShiftForObject(bundle.m_occupiedWithDangerZone, seg.m_freeArea, true);
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
    if ((lastResult = tryPlaceInner(pos)) == PlacementResult::Success) {
        auto originalPos = pos;
        if (!seg.m_heatMapping.contains(pos))
            return lastResult;
        int        currentHeat = seg.m_heatMapping[pos];
        MapTilePtr betterHeat  = pos;
        for (MapTilePtr neigh : pos->m_allNeighboursWithDiag) {
            auto it = seg.m_heatMapping.find(neigh);
            if (it != seg.m_heatMapping.cend() && it->second < currentHeat) {
                currentHeat = it->second;
                betterHeat  = neigh;
            }
        }
        if (betterHeat != pos) {
            FHPos delta = betterHeat->m_pos - pos->m_pos;
            pos         = originalPos;
            for (int i = 0; i < 3; ++i) {
                Mernel::ProfilerScope scope1("reposition");
                pos = pos->neighbourByOffset(delta);
                if (!pos)
                    break;

                // if we found that shifting to neighbor stop make us fail...
                if (tryPlaceInner(pos) != PlacementResult::Success) {
                    // ...backup to previous successful spot
                    pos = pos->neighbourByOffset(FHPos{} - delta);
                    assert(pos);
                    return tryPlaceInner(pos);
                }
            }
        }
        pos = originalPos;
        assert(pos);
        return tryPlaceInner(pos);
    }
    //Mernel::ProfilerScope scope1("failed");
    if (lastResult == PlacementResult::EstimateOccupiedFailure) {
        for (int i = 0; i < 5; ++i) {
            MapTileRegion neigh(pos->m_allNeighboursWithDiag);
            pos = neigh.findClosestPoint(m_tileContainer.m_centerTile->m_pos);
            assert(pos);

            if ((lastResult = tryPlaceInner(pos)) == PlacementResult::Success) {
                return lastResult;
            }
            if (lastResult != PlacementResult::EstimateOccupiedFailure)
                break;
        }
    }
    if (lastResult == PlacementResult::CollisionImpossibleShift) {
        for (MapTilePtr neigh : pos->m_allNeighboursWithDiag) {
            if ((lastResult = tryPlaceInner(neigh)) == PlacementResult::Success) {
                return lastResult;
            }
            if (lastResult != PlacementResult::CollisionImpossibleShift)
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

            if ((lastResult = tryPlaceInner(pos)) == PlacementResult::Success) {
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

    m_maxHeat = tileZone.m_rngZoneSettings.m_maxHeat;

    m_segments.clear();
    for (size_t index = 0; auto& seg : tileZone.m_innerAreaSegments) {
        ZoneSegment zs;
        zs.m_originalArea = seg.m_innerArea;
        zs.m_originalArea.erase(safePadding); // @todo: move this to segmentation phase
        zs.m_freeArea             = zs.m_originalArea;
        zs.m_originalAreaCells    = zs.m_originalArea.splitByGrid(3, 3, 2);
        zs.m_freeAreaByHeatTotal  = zs.m_originalArea.size();
        zs.m_originalAreaCentroid = zs.m_originalArea.makeCentroid(true);
        zs.m_segmentIndex         = index++;

        std::map<int, MapTileRegion> heatFragments;

        for (auto* tile : zs.m_originalArea) {
            int heat = tileZone.m_segmentHeat.getLevel(tile);
            zs.m_freeAreaByHeat[heat]++;
            zs.m_heatMapping[tile] = heat;
            heatFragments[heat].insert(tile);
        }
        for (auto& [heat, reg] : heatFragments) {
            zs.m_heatCentroids[heat] = reg.makeCentroid(false);
        }

        m_segments.push_back(std::move(zs));
    }
}

void ZoneObjectDistributor::ZoneSegment::removeHeatSize(size_t size, int startingHeat)
{
    if (m_freeAreaByHeatTotal < size)
        throw std::runtime_error("Check total size before using");
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
}

std::string ZoneObjectDistributor::ZoneSegment::toPrintableString() const
{
    std::ostringstream os;
    os << "[" << m_segmentIndex << "] tot: " << m_freeAreaByHeatTotal << ", heats: {";
    size_t check = 0;
    for (auto& [heat, value] : m_freeAreaByHeat) {
        if (!value)
            continue;
        os << heat << ": " << value << ", ";
        check += value;
    }
    os << "}, ck:" << check << ", objs:" << m_candidateObjectsNormal.size();
    return os.str();
}

}
