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
            if (isJoinable)
                m_extraObstacles.erase(m_guardAbsPos);

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

    //m_sizeInCells = m_occupiedWithDangerZone.splitByGrid(3, 3, 2).size();

    if (m_centerOffset == g_invalidPos) {
        m_centerOffset = m_occupiedArea.makeCentroid(true)->m_pos - m_absPos->m_pos;

        m_estimatedArea = m_occupiedWithDangerZone.size();
        if (m_estimatedArea <= 2)
            m_estimatedArea += m_passAroundEdge.size() / 2;
        else
            m_estimatedArea += m_passAroundEdge.size() * 2 / 3;
    }

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
    //std::cerr << "place " << m_generationId << " " << m_object->getId() << "\n";
    m_object->place(m_absPos->m_pos);
}

bool ZoneObjectDistributor::makeInitialDistribution(DistributionResult& distribution, const ZoneObjectGeneration& generated) const
{
    Mernel::ProfilerScope scope("InitialDistribution");
    distribution.m_allOriginalIds = generated.m_allIds;
    size_t totalSize              = 0;
    //size_t                totalSizeCells = 0;
    for (ZoneSegment& seg : distribution.m_segments) {
        totalSize += seg.m_freeArea.size();
        //totalSizeCells += seg.m_originalAreaCells.size();
    }
    if (generated.m_objects.empty())
        return true;
    size_t totalSizeObjects = 0;
    //size_t             totalSizeObjectsCells = 0;
    ZoneObjectWrapPtrList segmentsNormal;
    for (auto& obj : generated.m_objects) {
        ZoneObjectWrap wrap(obj);
        wrap.estimateOccupied(m_tileContainer.m_centerTile);

        makePreferredPoint(distribution, &wrap, wrap.m_randomAngleOffset, wrap.m_generatedIndex, wrap.m_generatedCount);

        //if (obj.m_preferredHeat >= 8 && wrap.m_randomAngleOffset > -1)
        //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = wrap.m_preferredPos->m_pos, .m_brushColor = 1, .m_shapeRadius = 4 });

        distribution.m_allObjects.push_back(wrap);
    }

    for (ZoneObjectWrap& wrap : distribution.m_allObjects) {
        if (wrap.m_scatterType) {
            if (wrap.m_objectType == ZoneObjectType::RoadScatter) {
                distribution.m_roadPickables.push_back(&wrap);
                //m_logOutput << "r id=" << wrap.m_object->getId() << "\n";
            } else if (wrap.m_objectType == ZoneObjectType::SegmentScatter) {
                distribution.m_segFreePickables.push_back(&wrap);
                //m_logOutput << "f id=" << wrap.m_object->getId() << "\n";
            }
        } else {
            segmentsNormal.push_back(&wrap);
            totalSizeObjects += wrap.m_estimatedArea;
        }
    }

    m_logOutput << m_indent << "estimation tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    if (totalSizeObjects > totalSize) {
        m_logOutput << m_indent << "too many generated objects to fit in area!\n";
        return false;
    }
    std::sort(segmentsNormal.begin(), segmentsNormal.end(), [](ZoneObjectWrap* r, ZoneObjectWrap* l) {
        return std::tuple{ r->m_placementOrder, r } < std::tuple{ l->m_placementOrder, l };
    });

    ZoneObjectWrapPtrList segmentsNormalUnfit;
    for (ZoneObjectWrap* object : segmentsNormal) {
        int minHeatAvailableInAllSegments = distribution.m_maxHeat;

        std::vector<ZoneSegment*> segCandidatesWithEnoughSpace;
        for (ZoneSegment& seg : distribution.m_segments) {
            if (seg.m_freeArea.size() < object->m_estimatedArea)
                continue;
            for (const auto& [heat, data] : seg.m_heatMap) {
                if (data.m_free.size() > 0) {
                    minHeatAvailableInAllSegments = std::min(minHeatAvailableInAllSegments, heat);
                    break;
                }
            }
            segCandidatesWithEnoughSpace.push_back(&seg);
        }
        // can happen if we need to place 10-tile object and we have remaining two segment 5-tile each.
        // totalSizeObjects > totalSize check still pass.
        if (segCandidatesWithEnoughSpace.empty()) {
            m_logOutput << m_indent << "Failed to find free segment to fit object= " << object->toPrintableString() << "\n";
            return false;
        }

        if (minHeatAvailableInAllSegments == distribution.m_maxHeat)
            throw std::runtime_error("sanity check failed: no heat");

        object->m_placedHeat = std::max(object->m_preferredHeat, minHeatAvailableInAllSegments);

        if (!placeWrapIntoSegments(distribution, object, segCandidatesWithEnoughSpace)) {
            m_logOutput << m_indent << "Failed to actually place the object= " << object->toPrintableString() << "\n";
            if (object->m_absPos) {
                auto* tz = object->m_absPos->m_zone;
                tz->m_rewardTilesFailure.insert(object->m_occupiedWithDangerZone);
            }
            return false;
        }
    }

    totalSizeObjects = 0;
    for (ZoneSegment& seg : distribution.m_segments) {
        if (seg.m_successNormal.empty())
            continue;
        totalSizeObjects += seg.m_originalArea.size() - seg.m_freeArea.size();
        //m_logOutput << "getFreePercent=" << seg.getFreePercent() << "\n";
    }

    m_logOutput << m_indent << "placed tiles= " << totalSizeObjects << " / " << totalSize << " \n";

    // place inner mountains
    {
        for (auto& seg : distribution.m_segments) {
            auto free = seg.m_freeArea;
            free.erase(free.makeInnerEdge(true));
            auto          parts = free.splitByFloodFill(false);
            MapTileRegion needBlock;
            for (auto& part : parts) {
                if (part.size() < 3)
                    continue;
                const size_t maxArea = 20;

                auto segments  = part.splitByMaxArea(maxArea, 30);
                auto borderNet = MapTileRegionWithEdge::getInnerBorderNet(MapTileRegionWithEdge::makeEdgeList(segments));

                for (const auto& seg2 : segments) {
                    for (auto* tile : seg2) {
                        if (borderNet.contains(tile))
                            continue;

                        needBlock.insert(tile);
                    }
                }
            }

            seg.m_freeArea.erase(needBlock);
            distribution.m_needBlock.insert(needBlock);
        }
    }

    // place roads and free pickables
    {
        const auto& roadRegion = distribution.m_tileZone->m_roads.m_all;
        auto&       freeCells  = distribution.m_allFreeCells;
        if (roadRegion.size() < distribution.m_roadPickables.size()) {
            m_logOutput << m_indent << "Roads size " << roadRegion.size() << " < " << distribution.m_roadPickables.size() << "\n";
            return false;
        }
        for (auto& seg : distribution.m_segments) {
            freeCells.insert(seg.m_spacingArea);
            freeCells.insert(seg.m_freeArea);
        }
        if (freeCells.size() < distribution.m_segFreePickables.size()) {
            m_logOutput << m_indent << "Segment spacing size " << freeCells.size() << " < " << distribution.m_segFreePickables.size() << "\n";
            return false;
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
        for (auto* object : seg.m_successNormal)
            commitPlacement(distribution, object, &seg);
    }

    {
        const auto& roadRegion = distribution.m_tileZone->m_roads.m_all;
        const auto& freeCells  = distribution.m_allFreeCells;
        for (size_t i = 0; auto* obj : distribution.m_roadPickables) {
            obj->m_absPos = roadRegion[i++ * roadRegion.size() / distribution.m_roadPickables.size()];
            obj->place();
            distribution.m_placedIds.push_back(obj->m_object->getId());
        }
        for (size_t i = 0; auto* obj : distribution.m_segFreePickables) {
            obj->m_absPos = freeCells[i++ * freeCells.size() / distribution.m_segFreePickables.size()];
            obj->place();
            distribution.m_placedIds.push_back(obj->m_object->getId());
        }
    }

    {
        std::sort(distribution.m_placedIds.begin(), distribution.m_placedIds.end());

        size_t                   maxSize = std::max(distribution.m_placedIds.size(), distribution.m_allOriginalIds.size());
        std::vector<std::string> missingIds(maxSize);
        std::vector<std::string> extraIds(maxSize);
        {
            auto resIt = std::set_difference(distribution.m_placedIds.cbegin(),
                                             distribution.m_placedIds.cend(),
                                             distribution.m_allOriginalIds.cbegin(),
                                             distribution.m_allOriginalIds.cend(),
                                             extraIds.begin());

            auto newSize = std::distance(extraIds.begin(), resIt);
            extraIds.resize(newSize);
        }
        {
            auto resIt = std::set_difference(distribution.m_allOriginalIds.cbegin(),
                                             distribution.m_allOriginalIds.cend(),
                                             distribution.m_placedIds.cbegin(),
                                             distribution.m_placedIds.cend(),
                                             missingIds.begin());

            auto newSize = std::distance(missingIds.begin(), resIt);
            missingIds.resize(newSize);
        }
        m_logOutput << m_indent << "Total generated: " << distribution.m_allOriginalIds.size() << ", placed: " << distribution.m_placedIds.size() << "\n";
        if (!extraIds.empty()) {
            m_logOutput << m_indent << "More items were placed than generated:\n";
            for (auto& id : extraIds)
                m_logOutput << m_indent << "  " << id << "\n";
        }
        if (!missingIds.empty()) {
            m_logOutput << m_indent << "Some items were generated, but never placed:\n";
            for (auto& id : missingIds)
                m_logOutput << m_indent << "  " << id << "\n";
        }
        if (!extraIds.empty() || !missingIds.empty())
            throw std::runtime_error("Placement logic is corrupted!");
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

    assert(object->m_preferredPos);

    std::set<std::tuple<int64_t, ZoneSegment*>> segCandidatesSorted;
    for (ZoneSegment* seg : segCandidates) {
        auto* bestHeat = seg->findBestHeatData(object->m_placedHeat, 1);
        if (!bestHeat)
            throw std::runtime_error("WTF");

        auto distance    = posDistance(bestHeat->m_centroid, object->m_preferredPos, 100);
        auto freePercent = seg->getFreePercent();
        if (freePercent < 60)
            distance = distance * 150 / 100;
        if (bestHeat->m_heat != object->m_placedHeat)
            distance = distance * 150 / 100;

        segCandidatesSorted.insert(std::tuple{ distance, seg });
    }
    for (auto& [_, seg] : segCandidatesSorted) {
        auto tiles = object->m_objectType == ZoneObjectType::Segment ? seg->getTilesByDistance() : seg->getTilesByDistanceFrom(object->m_preferredPos);
        for (auto* tile : tiles) {
            if (object->estimateOccupied(tile)) {
                if (seg->m_freeArea.intersectWith(object->m_occupiedWithDangerZone) == object->m_occupiedWithDangerZone) {
                    seg->m_freeArea.erase(object->m_allArea);

                    seg->m_freeArea.eraseExclaves(false);
                    seg->m_spacingArea.insert(object->m_passAroundEdge);

                    seg->m_successNormal.push_back(object);
                    object->m_placedHeat = seg->m_tileZone->m_heatForAll.getLevel(object->m_absPos);
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
        tz->m_rewardTilesSpacing.insert(object->m_passAroundEdge);
    }

    distribution.m_needBlock.insert(object->m_extraObstacles);

    distribution.m_placedIds.push_back(object->m_object->getId());

    if (object->m_useGuards) {
        MapGuard guard;
        guard.m_value = object->m_object->getGuard();
        guard.m_score = object->m_object->getScore();
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
    MapTileRegion wholeRegion;
    for (auto& [heat, reg] : tileZone.m_heatForSegments.m_byLevel) {
        auto& rect      = m_heatRegionRects[heat];
        rect.m_region   = reg;
        rect.m_centroid = reg.makeCentroid(false);
        wholeRegion.insert(tileZone.m_heatForAll.m_byLevel[heat]);

        auto outline   = MapTileRegionSegmentation::makeOutline(wholeRegion);
        rect.m_outline = MapTileRegionSegmentation::iterateOutline(outline);
    }

    m_segments.clear();
    for (size_t index = 0; auto& seg : tileZone.m_innerAreaSegments) {
        ZoneSegment zs;
        zs.m_originalArea = seg.m_innerArea;
        zs.m_originalArea.erase(safePadding);
        zs.m_originalArea.eraseExclaves(false);

        zs.m_freeArea             = zs.m_originalArea;
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

ZoneObjectDistributor::ZoneSegment::HeatDataItem* ZoneObjectDistributor::ZoneSegment::findBestHeatData(int heat, size_t estimatedArea)
{
    if (m_heatMap.empty())
        return nullptr;

    for (auto& [key, data] : m_heatMap) {
        if (key < heat)
            continue;
        if (data.m_free.size() >= estimatedArea)
            return &data;
    }
    for (auto& [key, data] : m_heatMap) {
        if (key == heat)
            break;
        if (data.m_free.size() >= estimatedArea)
            return &data;
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

void ZoneObjectDistributor::ZoneSegment::recalcHeat()
{
    for (auto& [heat, data] : m_heatMap) {
        data.m_free.clear();
        data.m_centroid = nullptr;
    }
    for (auto* tile : m_freeArea) {
        int   heat = m_tileZone->m_heatForAll.getLevel(tile);
        auto& m    = m_heatMap[heat];
        m.m_free.insert(tile);
    }
    for (auto& [heat, data] : m_heatMap) {
        data.m_centroid = data.m_free.makeCentroid(true);
        data.m_heat     = heat;
    }
}

MapTilePtrList ZoneObjectDistributor::ZoneSegment::getTilesByDistance() const
{
    std::vector<std::pair<int, MapTilePtr>> tileCandidatesSorted;

    for (auto& [heat, data] : m_heatMap) {
        for (auto* tile : data.m_free) {
            int distance = m_distances.at(tile);
            tileCandidatesSorted.push_back(std::pair{ distance, tile });
        }
    }

    std::sort(tileCandidatesSorted.begin(), tileCandidatesSorted.end());
    MapTilePtrList result;
    result.reserve(tileCandidatesSorted.size());
    for (auto [_, tile] : tileCandidatesSorted) {
        result.push_back(tile);
    }
    return result;
}

MapTilePtrList ZoneObjectDistributor::ZoneSegment::getTilesByDistanceFrom(MapTilePtr tile) const
{
    std::vector<std::pair<int, MapTilePtr>> tileCandidatesSorted;

    for (auto* freetile : m_freeArea) {
        int distance = posDistance(freetile, tile);
        tileCandidatesSorted.push_back(std::pair{ distance, freetile });
    }

    std::sort(tileCandidatesSorted.begin(), tileCandidatesSorted.end());
    MapTilePtrList result;
    result.reserve(tileCandidatesSorted.size());
    for (auto [_, stile] : tileCandidatesSorted) {
        result.push_back(stile);
    }
    return result;
}

void ZoneObjectDistributor::makePreferredPoint(DistributionResult& distribution, ZoneObjectWrap* object, int angleStartOffset, size_t index, size_t count) const
{
    auto&  rect    = distribution.m_heatRegionRects[object->m_preferredHeat];
    size_t regSize = rect.m_region.size();

    auto handleSpecialPoint = [&object](ZoneObjectType type, const MapTileRegion& reg, size_t index) -> bool {
        if (object->m_objectType == type) {
            if (index >= reg.size())
                throw std::runtime_error("Special point is invalid, not enough points in this zone (" + std::to_string(int(type)) + ")");
            object->m_preferredPos = reg[index];
            return true;
        }
        return false;
    };
    if (handleSpecialPoint(ZoneObjectType::TownMid1, distribution.m_tileZone->m_midTownNodes, 0)
        || handleSpecialPoint(ZoneObjectType::TownMid2, distribution.m_tileZone->m_midTownNodes, 1)
        || handleSpecialPoint(ZoneObjectType::TownMid3, distribution.m_tileZone->m_midTownNodes, 2)
        || handleSpecialPoint(ZoneObjectType::ExitMid1, distribution.m_tileZone->m_midExitNodes, 0))
        return;

    if (angleStartOffset == -1) {
        object->m_preferredPos = rect.m_region[index * regSize / count];
        //if (object->m_generationId == "111_ControlArts")
        //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = object->m_preferredPos->m_pos, .m_brushColor = 330, .m_shapeRadius = 4 });

        return;
    }
    size_t anglePos = index * 360 / count;
    int    angle    = angleStartOffset + anglePos;
    if (angle >= 360)
        angle -= 360;
    // angle is [0; 360)

    size_t outlineSize  = rect.m_outline.size();
    size_t outlineIndex = outlineSize * angle / 360;

    object->m_preferredPos = rect.m_outline[outlineIndex];
    rect.m_region.findClosestPoint(object->m_preferredPos->m_pos);

    //if (object->m_generationId == "111_ControlArts")
    //    m_map.m_debugTiles.push_back(FHDebugTile{ .m_pos = object->m_preferredPos->m_pos, .m_brushColor = 330, .m_shapeRadius = 4 });
}

}
