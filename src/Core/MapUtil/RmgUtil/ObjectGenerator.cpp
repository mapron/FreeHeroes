/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "ObjectGenerator.hpp"
#include "ObjectGeneratorUtils.hpp"
#include "ObjectGeneratorFactories.hpp"
#include "TemplateUtils.hpp"

#include "MernelPlatform/Profiler.hpp"

#include "LibraryMapBank.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryMapVisitable.hpp"

#include <iostream>
#include <functional>

namespace FreeHeroes {

ZoneObjectGeneration ObjectGenerator::generate(const FHRngZone&             zoneSettings,
                                               Core::LibraryFactionConstPtr rewardsFaction,
                                               Core::LibraryFactionConstPtr dwellFaction,
                                               Core::LibraryTerrainConstPtr terrain,
                                               int64_t                      armyPercent,
                                               int64_t                      goldPercent) const
{
    static const std::string indentBase("      ");
    Mernel::ProfilerScope    scope("generate");

    ArtifactPool artifactPool(*m_map, m_database, m_rng);
    SpellPool    spellPool(*m_map, m_database, m_rng);
    FactionPool  factionRewardsPool;
    factionRewardsPool.m_limit = zoneSettings.m_generators.m_banks.m_maxUniqueFactions;

    const bool           doLog = false;
    ZoneObjectGeneration result;

    Core::MapScore targetScoreRemainingPrev;
    if (zoneSettings.m_scoreTargets.empty())
        return {};

    for (const auto& [scoreId, scoreSettings] : zoneSettings.m_scoreTargets) {
        if (!scoreSettings.m_isEnabled) {
            m_logOutput << indentBase << scoreId << " is disabled;\n";
            continue;
        }

        Core::MapScore targetScore = scoreSettings.makeTargetScore();

        {
            auto targetScoreRemainingPrevCopy = targetScoreRemainingPrev;
            for (auto& [key, val] : targetScoreRemainingPrevCopy) {
                if (targetScore.contains(key)) {
                    targetScore[key] += val;
                    targetScoreRemainingPrev.erase(key);
                }
            }
        }

        if (0) {
            m_logOutput << indentBase << scoreId << " prev: " << targetScoreRemainingPrev << ", target=" << targetScore << "\n";
        }

        //auto targetSum = totalScoreValue(targetScore);

        if (doLog)
            m_logOutput << indentBase << scoreId << " start\n";
        Core::MapScore currentScore;

        auto isFilteredOut = [&scoreSettings](const std::string& key) {
            if (!scoreSettings.m_generatorsExclude.empty() && scoreSettings.m_generatorsExclude.contains(key))
                return true;
            if (!scoreSettings.m_generatorsInclude.empty() && !scoreSettings.m_generatorsInclude.contains(key))
                return true;
            return false;
        };

        std::vector<IObjectFactoryPtr> objectFactories;
        if (!isFilteredOut("banks"))
            objectFactories.push_back(std::make_shared<ObjectFactoryBank>(*m_map, zoneSettings.m_generators.m_banks, scoreSettings, scoreId, m_database, m_rng, &artifactPool, &factionRewardsPool, terrain));
        if (!isFilteredOut("artifacts"))
            objectFactories.push_back(std::make_shared<ObjectFactoryArtifact>(*m_map, zoneSettings.m_generators.m_artifacts, scoreSettings, scoreId, m_database, m_rng, &artifactPool));
        if (!isFilteredOut("resources"))
            objectFactories.push_back(std::make_shared<ObjectFactoryResourcePile>(*m_map, zoneSettings.m_generators.m_resources, scoreSettings, scoreId, m_database, m_rng));
        if (!isFilteredOut("pandoras"))
            objectFactories.push_back(std::make_shared<ObjectFactoryPandora>(*m_map, zoneSettings.m_generators.m_pandoras, scoreSettings, scoreId, m_database, m_rng, rewardsFaction));
        if (!isFilteredOut("shrines"))
            objectFactories.push_back(std::make_shared<ObjectFactoryShrine>(*m_map, zoneSettings.m_generators.m_shrines, scoreSettings, scoreId, m_database, m_rng, &spellPool));
        if (!isFilteredOut("scrolls"))
            objectFactories.push_back(std::make_shared<ObjectFactoryScroll>(*m_map, zoneSettings.m_generators.m_scrolls, scoreSettings, scoreId, m_database, m_rng, &spellPool));
        if (!isFilteredOut("dwellings"))
            objectFactories.push_back(std::make_shared<ObjectFactoryDwelling>(*m_map, zoneSettings.m_generators.m_dwellings, scoreSettings, scoreId, m_database, m_rng, dwellFaction));
        if (!isFilteredOut("visitables"))
            objectFactories.push_back(std::make_shared<ObjectFactoryVisitable>(*m_map, zoneSettings.m_generators.m_visitables, scoreSettings, scoreId, m_database, m_rng, terrain));
        if (!isFilteredOut("mines"))
            objectFactories.push_back(std::make_shared<ObjectFactoryMine>(*m_map, zoneSettings.m_generators.m_mines, scoreSettings, scoreId, m_database, m_rng, terrain));
        if (!isFilteredOut("skillHuts"))
            objectFactories.push_back(std::make_shared<ObjectFactorySkillHut>(*m_map, zoneSettings.m_generators.m_skillHuts, scoreSettings, scoreId, m_database, m_rng));

        const int iterLimit = 100000;
        int       i         = 0;

        ZoneObjectList objectList;
        for (; i < iterLimit; i++) {
            if (!generateOneObject(targetScore, currentScore, objectFactories, objectList)) {
                if (doLog)
                    m_logOutput << indentBase << scoreId << " finished on [" << i << "] iteration\n";
                break;
            }
        }
        if (i >= iterLimit - 1)
            throw std::runtime_error("Iteration limit reached.");

        //if (deficitSum > allowedDeficit && 0) // @todo: redo deficit
        //    throw std::runtime_error("Deficit score for '" + scoreId + "' exceed tolerance!");

        const int64_t guardGroupLimit = scoreSettings.m_guardGroupLimit >= 0 ? scoreSettings.m_guardGroupLimit : zoneSettings.m_guardGroupLimit;
        const int64_t guardThreshold  = scoreSettings.m_guardThreshold >= 0 ? scoreSettings.m_guardThreshold : zoneSettings.m_guardThreshold;
        const int64_t guardMinToGroup = scoreSettings.m_guardMinToGroup >= 0 ? scoreSettings.m_guardMinToGroup : zoneSettings.m_guardMinToGroup;

        for (auto& item : objectList) {
            item.m_generationId = scoreId;
            item.m_objectType   = scoreSettings.m_objectType;
            item.m_scatterType  = item.m_objectType == ZoneObjectType::SegmentScatter || item.m_objectType == ZoneObjectType::RoadScatter;
            item.m_useGuards    = !item.m_scatterType && item.m_object->getGuard() >= guardThreshold;

            item.m_preferredHeat = 0;
            if (!scoreSettings.m_preferredHeats.empty())
                item.m_preferredHeat = scoreSettings.m_preferredHeats[m_rng->genMinMax(0, scoreSettings.m_preferredHeats.size() - 1)];
            item.m_placementOrder = item.m_preferredHeat;
            if (scoreSettings.m_placementOrder >= -1)
                item.m_placementOrder = scoreSettings.m_placementOrder;
            //m_logOutput << "item " << item.m_object->getId() << " item.m_preferredHeat= " << item.m_preferredHeat << " item.m_placementOrder=" << item.m_placementOrder << "\n";

            item.m_pickable = item.m_object->getType() == IZoneObject::Type::Pickable || item.m_object->getType() == IZoneObject::Type::Joinable;

            if (item.m_scatterType) {
                if (!item.m_pickable)
                    throw std::runtime_error(scoreId + ": all items must be pickable, " + item.m_object->getId() + " is not.");
            }
        }
        makeGroups(guardGroupLimit, guardMinToGroup, objectList);

        std::vector<ZoneObjectItem*> needDistributeByRadius;
        std::vector<ZoneObjectItem*> needDistributeEqual;
        for (auto& item : objectList) {
            if (item.m_scatterType)
                needDistributeEqual.push_back(&item);
            else
                needDistributeByRadius.push_back(&item);

            result.m_allIds.push_back(item.m_object->getId());
        }
        shuffle(needDistributeByRadius);
        shuffle(needDistributeEqual);

        int angleStart = m_rng->genMinMax(0, 359);
        for (size_t d = 0; d < needDistributeByRadius.size(); ++d) {
            needDistributeByRadius[d]->m_randomAngleOffset = angleStart;
            needDistributeByRadius[d]->m_generatedIndex    = d;
            needDistributeByRadius[d]->m_generatedCount    = needDistributeByRadius.size();
        }
        for (size_t d = 0; d < needDistributeEqual.size(); ++d) {
            needDistributeEqual[d]->m_generatedIndex = d;
            needDistributeEqual[d]->m_generatedCount = needDistributeEqual.size();
        }

        Core::MapScore remainScoreNext;
        auto           remainScore = (targetScore - currentScore);
        for (const auto& [key, val] : scoreSettings.m_score) {
            if (!remainScore.contains(key))
                continue;
            auto remainValue = remainScore[key];
            if (remainValue > val.m_maxRemain) {
                m_logOutput << indentBase << scoreId << " remainScore=" << remainScore << ": for attr " << FHScoreSettings::attrToString(key) << " max remain is " << val.m_maxRemain << ", but " << remainValue << " is generated.\n";
                throw std::runtime_error("Incorrect target scores. Make sure to create consume chains.");
            }
            if (!val.m_consumeRemain)
                remainScoreNext[key] = remainValue;
        }
        targetScoreRemainingPrev = remainScoreNext + targetScoreRemainingPrev;

        m_logOutput << indentBase << scoreId << " generated: " << objectList.size() << ", (score=" << currentScore << ") unconsumed remain for next target: " << remainScoreNext << "\n";
        //m_logOutput << "targetScoreRemainingPrev=" << targetScoreRemainingPrev << "\n";
        result.m_objects.insert(result.m_objects.end(), objectList.cbegin(), objectList.cend());
    }
    std::sort(result.m_objects.begin(), result.m_objects.end(), [](const auto& r, const auto& l) {
        return std::tuple{ r.m_placementOrder, r.m_generatedIndex, &r } < std::tuple{ l.m_placementOrder, l.m_generatedIndex, &l };
    });
    auto totalScoreRemain = totalScoreValue(targetScoreRemainingPrev);
    std::sort(result.m_allIds.begin(), result.m_allIds.end());
    m_logOutput << indentBase << "Total generated:" << result.m_allIds.size() << ", remainScore=" << targetScoreRemainingPrev << "\n";
    if (totalScoreRemain > 0)
        throw std::runtime_error("Total remainder must be 0");
    return result;
}

bool ObjectGenerator::generateOneObject(const Core::MapScore&           targetScore,
                                        Core::MapScore&                 currentScore,
                                        std::vector<IObjectFactoryPtr>& objectFactories,
                                        ZoneObjectList&                 objectList) const
{
    Mernel::ProfilerScope    scope("oneObject");
    static const std::string indent("        ");
    uint64_t                 totalWeight = 0;
    for (IObjectFactoryPtr& fac : objectFactories) {
        totalWeight += fac->totalFreq();
    }
    if (!totalWeight)
        return false;

    const uint64_t rngFreq = m_rng->gen(totalWeight - 1);

    uint64_t indexWeight = 0, baseWeight = 0;
    for (IObjectFactoryPtr& fac : objectFactories) {
        indexWeight += fac->totalFreq();
        if (indexWeight > rngFreq && fac->totalFreq()) {
            const uint64_t rngFreqForFactory = rngFreq - baseWeight;
            auto           obj               = fac->makeChecked(rngFreqForFactory, currentScore, targetScore);
            if (!obj) {
                return true;
            }
            objectList.push_back(ZoneObjectItem{ .m_object = obj });
            return true;
        }
        baseWeight = indexWeight;
    }

    assert(false);
    return false;
}

void ObjectGenerator::makeGroups(int64_t guardGroupLimit, int64_t guardMinToGroup, ZoneObjectList& objectList) const
{
    if (guardMinToGroup <= 0)
        return;

    ZoneObjectList objectListForGrouping;
    ZoneObjectList objectListGrouped;
    ZoneObjectList objectListRemain;
    {
        for (auto& obj : objectList) {
            if (obj.m_objectType == ZoneObjectType::Segment && obj.m_pickable && obj.m_useGuards) {
                if (obj.m_object->getGuard() >= guardMinToGroup) {
                    objectListForGrouping.push_back(obj);
                    continue;
                }
            }
            objectListRemain.push_back(obj);
        }
        if (objectListForGrouping.empty())
            return;
    }

    auto makeNewObjectBundle = [this, guardGroupLimit]() -> IZoneObjectGroupPtr {
        auto obj = makeNewZoneObjectGroup(guardGroupLimit, m_rng->genMinMax(2, 4), m_rng->genSmall(101));
        return obj;
    };

    auto           bundleGuarded      = makeNewObjectBundle();
    bool           bundleGuardedEmpty = true;
    ZoneObjectItem lastItem;

    auto flushBundle = [&bundleGuarded, &objectListGrouped, &makeNewObjectBundle, &bundleGuardedEmpty, &lastItem]() {
        if (bundleGuardedEmpty)
            return;
        bundleGuarded->updateMask();
        auto item     = lastItem;
        item.m_object = bundleGuarded;
        objectListGrouped.push_back(item);
        bundleGuarded      = makeNewObjectBundle();
        bundleGuardedEmpty = true;
    };

    for (const auto& item : objectListForGrouping) {
        if (!bundleGuardedEmpty) {
            if (bundleGuarded->tryPush(item.m_object)) {
                lastItem = item;
                continue;
            }
            // can not push more
            flushBundle();
        }
        if (bundleGuarded->tryPush(item.m_object)) {
            bundleGuardedEmpty = false;
            lastItem           = item;
            continue;
        }
        throw std::runtime_error("sanity check failed: failed push to empty bundle.");
    }
    flushBundle();

    if (0) {
        for (auto& obj : objectListGrouped)
            m_logOutput << "grouped: " << obj.m_object->getId() << "\n";
    }

    objectList = std::move(objectListRemain);
    objectList.insert(objectList.end(), objectListGrouped.cbegin(), objectListGrouped.cend());
}

void ObjectGenerator::shuffle(std::vector<ZoneObjectItem*>& list) const
{
    std::vector<ZoneObjectItem*> copy;
    while (!list.empty()) {
        size_t remain = list.size();
        if (remain == 1) {
            copy.push_back(list[0]);
            break;
        }
        auto i   = m_rng->genMinMax(0, list.size() - 1);
        auto obj = list[i];
        copy.push_back(obj);
        list.erase(list.begin() + i);
    }
    std::swap(copy, list);
}

bool ObjectGenerator::correctObjIndex(Core::ObjectDefIndex& defIndex, const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain)
{
    for (const auto& [variant, defSource] : defMapping.variants) {
        if (defSource->terrainsSoftCache.contains(requiredTerrain)) {
            defIndex.variant      = variant;
            defIndex.substitution = "";
            return true;
        }

        for (const auto& [subsitutionId, def] : defSource->substitutions) {
            if (def->terrainsSoftCache.contains(requiredTerrain)) {
                defIndex.variant      = variant;
                defIndex.substitution = subsitutionId;
                return true;
            }
        }
    }
    return false;
}

bool ObjectGenerator::terrainViable(const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain)
{
    Core::ObjectDefIndex index;
    return correctObjIndex(index, defMapping, requiredTerrain);
}

}
