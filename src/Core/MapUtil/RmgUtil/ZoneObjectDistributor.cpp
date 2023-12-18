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
                          ShiftLoopDetected,
                          Retry);

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
        } };
        wrap.estimateOccupied(m_tileContainer.m_centerTile);
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
    ZoneObjectWrapPtrList segmentsNormalUnfit;
    for (ZoneObjectWrap* wrap : segmentsNormal) {
        int minHeatAvailableInAllSegments = distribution.m_maxHeat;
        for (ZoneSegment& seg : distribution.m_segments) {
            if (seg.m_freeAreaByHeatTotal < wrap->m_estimatedArea)
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
        wrap->m_placedHeat    = std::max(wrap->m_preferredHeat, currentHeat);
        //m_logOutput << m_indent << "wrap " << wrap->toPrintableString() << "\n";

        std::vector<ZoneSegment*> segCandidates;
        for (ZoneSegment& seg : distribution.m_segments) {
            auto it = seg.m_freeAreaByHeat.find(wrap->m_placedHeat);
            if (it == seg.m_freeAreaByHeat.cend() || it->second <= 0)
                continue;
            if (seg.m_freeAreaByHeatTotal < wrap->m_estimatedArea)
                continue;
            segCandidates.push_back(&seg);
        }
        if (segCandidates.empty()) {
            segmentsNormalUnfit.push_back(wrap);
            //m_logOutput << m_indent << "    failed to find place\n";
            continue;
        }
        ZoneSegment* fitSeg = segCandidates[0];
        if (wrap->m_radiusVector != g_invalidPos) {
            const FHPos closest = fitSeg->m_tileZone->m_centroid->m_pos + wrap->m_radiusVector;
            auto        it      = std::min_element(segCandidates.cbegin(), segCandidates.cend(), [closest](ZoneSegment* l, ZoneSegment* r) {
                const auto lDistance = posDistance(l->m_originalAreaCentroid->m_pos, closest);
                const auto rDistance = posDistance(r->m_originalAreaCentroid->m_pos, closest);
                return std::tuple{ lDistance, l->m_segmentIndex } < std::tuple{ rDistance, r->m_segmentIndex };
            });
            fitSeg              = *it;
        }
        fitSeg->removeHeatSize(wrap->m_estimatedArea, wrap->m_placedHeat);
        //m_logOutput << m_indent << "    fit into " << fitSeg->toPrintableString() << "\n";
        fitSeg->m_candidateObjectsNormal.push_back(wrap);
    }
    for (auto& wrap : segmentsNormalUnfit) {
        std::vector<ZoneSegment*> segCandidates;
        for (ZoneSegment& seg : distribution.m_segments) {
            if (seg.m_freeAreaByHeatTotal < wrap->m_estimatedArea)
                continue;
            segCandidates.push_back(&seg);
        }
        m_logOutput << m_indent << "re-attempt wrap " << wrap->toPrintableString() << "\n";
        if (segCandidates.empty()) {
            // can happen if we need to place 10-tile object and we have remaining two segment 5-tile each.
            m_logOutput << m_indent << "Failed to find free segment to fit object!\n";
            return false;
        }
        ZoneSegment* fitSeg = segCandidates[0]; //@todo: smarter?
        fitSeg->removeHeatSize(wrap->m_estimatedArea, wrap->m_placedHeat);
        //m_logOutput << m_indent << "    fit into " << fitSeg->toPrintableString() << "\n";
        fitSeg->m_candidateObjectsNormal.push_back(wrap);
    }

    return true;
}

bool ZoneObjectDistributor::doPlaceDistribution(DistributionResult& distribution) const
{
    if (distribution.m_allObjects.empty())
        return true;

    auto logError = [this](PlacementResult lastResult, ZoneObjectWrap* object, ZoneSegment& seg, MapTilePtr posHint) {
        if (object->m_absPos) {
            auto* tz = object->m_absPos->m_zone;
            tz->m_rewardTilesFailure.insert(object->m_occupiedWithDangerZone);
        }
        //tz->m_rewardTilesPos.insert(object.m_absPos);
        //tz->m_rewardTilesCenters.insert(object.m_absPos->neighbourByOffset(object.m_centerOffset));
        //tz->m_rewardTilesHints.insert(posHint);

        //failedPlacement.push_back(object);
        m_logOutput << m_indent << " placement failure (" << placementResultToString(lastResult) << "): pos=" << (object->m_absPos ? object->m_absPos->toPrintableString() : "NULL") << " freeArea=" << seg.m_freeArea.size() << " objectSize=" << object->m_estimatedArea << "; " << object->m_object->getId() << "\n";

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

    ZoneObjectWrapPtrList failedPlacement;
    size_t                totalObjects = 0;

    for (ZoneSegment& seg : distribution.m_segments) {
        if (seg.m_candidateObjectsNormal.empty())
            continue;
        totalObjects += seg.m_candidateObjectsNormal.size();

        //for (auto& obj : seg.m_candidateObjectsNormal) {
        //    m_logOutput << m_indent << "placing " << obj.m_object->getId() << "\n";
        //}
        auto makeObjRegions = [&seg](MapTileRegionList& objectRegions, MapTileRegion& area, size_t count, ZoneObjectWrapPtrList& candidates) {
            KMeansSegmentationSettings settings = MapTileRegionSegmentation::guessKMeansByGrid(area, count);

            std::set<size_t> remainingFragmentIndices;
            for (size_t i = 0; i < count; ++i)
                remainingFragmentIndices.insert(i);
            for (size_t i = 0; i < count; ++i) {
                ZoneObjectWrap* obj                   = candidates[i];
                auto*           preferredHeatCentroid = seg.findBestHeatCentroid(obj->m_preferredHeat);

                auto   it        = std::min_element(remainingFragmentIndices.begin(), remainingFragmentIndices.end(), [&settings, preferredHeatCentroid](size_t l, size_t r) {
                    auto lCentroid = settings.m_items[l].m_initialCentroid;
                    auto rCentroid = settings.m_items[r].m_initialCentroid;
                    auto lDistance = posDistance(lCentroid, preferredHeatCentroid, 100);
                    auto rDistance = posDistance(rCentroid, preferredHeatCentroid, 100);
                    return std::tuple{ lDistance, l } < std::tuple{ rDistance, r };
                });
                size_t bestIndex = *it;
                remainingFragmentIndices.erase(it);
                obj->m_segmentFragmentIndex            = bestIndex;
                settings.m_items[bestIndex].m_areaHint = obj->m_estimatedArea;
            }
            objectRegions = area.splitByKExt(settings);
        };

        size_t            count = seg.m_candidateObjectsNormal.size();
        MapTileRegionList objectRegions;
        {
            makeObjRegions(objectRegions, seg.m_originalArea, count, seg.m_candidateObjectsNormal);
        }
        //std::vector<ZoneObjectWrap*> successPlacementSeg;
        std::vector<ZoneObjectWrap*> failedPlacementSeg;

        for (ZoneObjectWrap* object : seg.m_candidateObjectsNormal) {
            MapTilePtr posHint = objectRegions[object->m_segmentFragmentIndex].makeCentroid(true);
            if (seg.placeOnMap(*object, posHint, false) == PlacementResult::Success) {
                seg.m_freeArea.erase(object->m_allArea);
                seg.m_successNormal.push_back(object);
            } else
                failedPlacementSeg.push_back(object);
        }
        if (failedPlacementSeg.empty()) {
            continue;
        }

        seg.compactIfNeeded();

        {
            count = failedPlacementSeg.size();
            makeObjRegions(objectRegions, seg.m_freeArea, count, failedPlacementSeg);

            for (ZoneObjectWrap* object : failedPlacementSeg) {
                MapTilePtr posHint = objectRegions[object->m_segmentFragmentIndex].makeCentroid(true);
                auto       result  = seg.placeOnMap(*object, posHint, false);
                if (result == PlacementResult::Success) {
                    seg.m_freeArea.erase(object->m_allArea);
                    seg.m_successNormal.push_back(object);
                } else {
                    //logError(result, object, seg, posHint);
                    failedPlacement.push_back(object);
                }
            }
        }

        //m_logOutput << m_indent << prefix << " placement failure (" << int(lastResult) << ") [" << i << "]: pos=" << (bundle.m_absPos ? bundle.m_absPos->toPrintableString() : "NULL") << " size=" << bundle.m_estimatedArea << "; " << bundle.toPrintableString() << "\n";
    }

    if (!failedPlacement.empty()) {
        m_logOutput << m_indent << " placement failure=" << failedPlacement.size() << " of " << totalObjects << " items, trying to find alternative segments\n";
        size_t failures = 0;
        for (ZoneObjectWrap* object : failedPlacement) {
            auto         it  = std::max_element(distribution.m_segments.begin(), distribution.m_segments.end(), [](const ZoneSegment& l, const ZoneSegment& r) {
                return std::tuple{ l.m_freeArea.size(), &l } < std::tuple{ r.m_freeArea.size(), &r };
            });
            ZoneSegment& seg = *it;
            if (seg.m_freeArea.size() < object->m_estimatedArea) {
                m_logOutput << m_indent << "Failed to find free segment of size " << object->m_estimatedArea << "\n";
                return false;
            }
            seg.compactIfNeeded();

            MapTilePtr posHint = seg.m_freeArea.makeCentroid(true);
            auto       result  = seg.placeOnMap(*object, posHint, true);
            if (result == PlacementResult::Success) {
                seg.m_freeArea.erase(object->m_allArea);
                seg.m_successNormal.push_back(object);

                seg.m_compact = false;
            } else {
                logError(result, object, seg, posHint);
                failures++;
            }
        }
        m_logOutput << m_indent << " failures after correction=" << failures << "\n";
        if (failures > 0) {
            if (0)
                return false;
        }
    }
    MergedRegion totalFreeTiles;

    for (ZoneSegment& seg : distribution.m_segments) {
        if (seg.m_candidateObjectsNormal.empty())
            continue;
        seg.commitAll(m_map, distribution);
    }

    //for (ZoneObjectWrap* object : distribution.m_candidateObjectsFreePickables) {
    //distribution.m_tileZone->m_rewardTilesCenters;
    //}
    //std::map<int, MapTileRegion> roadsByRegion = distribution.m_tileZone->m_roadHeat.m_byLevel;
    //for (ZoneObjectWrap* object : distribution.m_roadPickables) {
    //    roadsByRegion
    //}

    return true;
}

void ZoneObjectDistributor::DistributionResult::init(TileZone& tileZone)
{
    MapTileRegion safePadding = tileZone.m_nodes.getRegion(RoadLevel::Towns);
    {
        safePadding = blurSet(safePadding, true, false);
    }
    if (tileZone.m_innerAreaSegments.empty())
        throw std::runtime_error("No segments in tile zone!");

    m_maxHeat  = tileZone.m_rngZoneSettings.m_maxHeat;
    m_tileZone = &tileZone;

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
        zs.m_tileZone             = &tileZone;

        std::map<int, MapTileRegion> heatFragments;

        for (auto* tile : zs.m_originalArea) {
            int heat = tileZone.m_segmentHeat.getLevel(tile);
            zs.m_freeAreaByHeat[heat]++;
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

MapTilePtr ZoneObjectDistributor::ZoneSegment::findBestHeatCentroid(int heat) const
{
    auto it = m_heatCentroids.find(heat);
    if (it == m_heatCentroids.cend())
        it = m_heatCentroids.cbegin();
    return it->second;
}

void ZoneObjectDistributor::ZoneSegment::compactIfNeeded()
{
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
    recalcFree();
}

void ZoneObjectDistributor::ZoneSegment::commitAll(FHMap& m_map, DistributionResult& distribution)
{
    for (ZoneObjectWrap* object : m_successNormal) {
        commitPlacement(m_map, distribution, object);
    }
}

void ZoneObjectDistributor::ZoneSegment::commitPlacement(FHMap& m_map, DistributionResult& distribution, ZoneObjectWrap* object)
{
    this->m_freeArea.erase(object->m_allArea);
    object->place();
    //m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = seg.m_originalAreaCentroid->m_pos, .m_text = std::to_string(seg.m_segmentIndex) });

    int paletteSize = distribution.m_maxHeat;
    int heatLevel   = object->m_preferredHeat;
    if (1) {
        m_map.m_debugTiles.push_back(FHDebugTile{
            .m_pos         = object->m_absPos->m_pos,
            .m_penColor    = heatLevel + 1, // heatLevel is 0-based
            .m_penPalette  = paletteSize,
            .m_shape       = 1,
            .m_shapeRadius = 4,
        });
    }

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

void ZoneObjectDistributor::ZoneSegment::recalcFree(ZoneObjectWrap* exclude)
{
    m_freeArea = m_originalArea;
    for (ZoneObjectWrap* nobject : m_successNormal) {
        if (nobject != exclude)
            m_freeArea.erase(nobject->m_allArea);
    }
}

ZoneObjectDistributor::PlacementResult ZoneObjectDistributor::ZoneSegment::placeOnMap(ZoneObjectWrap& bundle,
                                                                                      MapTilePtr      posHint,
                                                                                      bool            packPlacement)
{
    Mernel::ProfilerScope scope("placeOnMap");
    ZoneSegment&          seg = *this;

    if (seg.m_freeArea.size() < bundle.m_estimatedArea)
        return PlacementResult::InsufficientSpaceInSource;

    MapTilePtr pos = posHint;
    assert(pos);

    FHPos newPossibleShift = g_invalidPos;

    auto tryPlaceInner = [&bundle, &seg, &newPossibleShift](MapTilePtr pos) -> PlacementResult {
        Mernel::ProfilerScope scope("placeInner");
        if (!bundle.estimateOccupied(pos))
            return PlacementResult::EstimateOccupiedFailure;

        if (seg.m_freeArea.size() < bundle.m_estimatedArea)
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

    auto tryPlaceOuter = [&pos, &tryPlaceInner, &seg, &newPossibleShift, packPlacement]() -> PlacementResult {
        PlacementResult lastResult;
        // if first attempt is succeed, we'll try to find near place where we don't fit, and then backup to more close fit.
        if ((lastResult = tryPlaceInner(pos)) == PlacementResult::Success && packPlacement) {
            auto originalPos = pos;

            auto findLowerDistanceNeighbour = [&seg](MapTilePtr pos) {
                int currentDistance = seg.m_tileZone->m_distances.getLevel(pos);
                if (currentDistance < 0)
                    return pos;
                MapTilePtr betterHeat = pos;

                for (MapTilePtr neigh : pos->m_allNeighboursWithDiag) {
                    if (!seg.m_freeArea.contains(neigh))
                        continue;

                    int newDistance = seg.m_tileZone->m_distances.getLevel(neigh);
                    if (newDistance >= 0 && newDistance < currentDistance) {
                        currentDistance = newDistance;
                        betterHeat      = neigh;
                    }
                }
                return betterHeat;
            };
            MapTilePtr    betterPos = findLowerDistanceNeighbour(pos);
            MapTileRegion used;
            if (betterPos != pos) {
                used.insert(pos);
                used.insert(betterPos);
                MapTilePtr betterPosPrev = pos;
                //m_logOutput << m_indent << " cadidate? " << pos->toPrintableString() << " -> " << betterPos->toPrintableString() << "\n";

                // if we found that shifting to neighbor stop make us fail...
                if ((lastResult = tryPlaceInner(betterPos)) != PlacementResult::Success) {
                    //std::cout << "          A reposition stop " << pos->toPrintableString() << " -> " << betterPos->toPrintableString() << " to " << betterPosPrev->toPrintableString() << "\n";

                    // ...backup to previous successful spot
                    return tryPlaceInner(betterPosPrev);
                }
                Mernel::ProfilerScope scope1("reposition");
                for (int i = 0; i < 10; ++i) {
                    betterPosPrev = betterPos;

                    MapTilePtr evenBetterPos = findLowerDistanceNeighbour(betterPos);
                    if (evenBetterPos == betterPos || used.contains(evenBetterPos))
                        break;
                    betterPos = evenBetterPos;

                    if ((lastResult = tryPlaceInner(betterPos)) != PlacementResult::Success) {
                        //std::cout << "         B reposition stop " << betterPosPrev->toPrintableString() << " -> " << betterPos->toPrintableString() << "\n";
                        return tryPlaceInner(betterPosPrev);
                    }
                }
                if (lastResult != PlacementResult::Success)
                    lastResult = tryPlaceInner(originalPos);

                //if (betterPos != pos)
                //    m_logOutput << m_indent << " reposition " << pos->toPrintableString() << " -> " << betterPos->toPrintableString() << "\n";
            }
            return lastResult;
        }
        //Mernel::ProfilerScope scope1("failed");
        if (lastResult == PlacementResult::EstimateOccupiedFailure) {
            for (int i = 0; i < 5; ++i) {
                MapTileRegion neigh(pos->m_allNeighboursWithDiag);
                pos = neigh.findClosestPoint(pos->m_container->m_centerTile->m_pos);
                assert(pos);

                if ((lastResult = tryPlaceInner(pos)) == PlacementResult::Success) {
                    return lastResult;
                }
                if (lastResult != PlacementResult::EstimateOccupiedFailure)
                    return PlacementResult::Retry;
            }
            return lastResult;
        }
        if (lastResult == PlacementResult::CollisionImpossibleShift) {
            for (MapTilePtr neigh : pos->m_allNeighboursWithDiag) {
                if ((lastResult = tryPlaceInner(neigh)) == PlacementResult::Success) {
                    return lastResult;
                }
                if (lastResult != PlacementResult::CollisionImpossibleShift)
                    return PlacementResult::Retry;
            }
            return lastResult;
        }
        if (lastResult == PlacementResult::CollisionHasShift) {
            //Mernel::ProfilerScope scope2("hasShift");
            MapTileRegion used;
            used.insert(pos);
            for (int i = 0; i < 5; ++i) {
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
                    return PlacementResult::Retry;
            }
            return PlacementResult::RunOutOfShiftRetries;
        }

        return lastResult;
    };
    PlacementResult lastResult;
    for (int i = 0; i < 3; i++) {
        lastResult = tryPlaceOuter();
        if (lastResult != PlacementResult::Retry) {
            return lastResult;
        }
    }

    return PlacementResult::RunOutOfShiftRetries;
}

}
