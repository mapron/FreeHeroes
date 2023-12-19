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

    const bool           doLog = false;
    ZoneObjectGeneration result;

    Core::MapScore targetScoreRemainingPrev;
    if (zoneSettings.m_scoreTargets.empty())
        return {};

    const FHScoreSettings* lastSettings = &(zoneSettings.m_scoreTargets.rbegin()->second);

    for (const auto& [scoreId, scoreSettings] : zoneSettings.m_scoreTargets) {
        if (!scoreSettings.m_isEnabled) {
            m_logOutput << indentBase << scoreId << " is disabled;\n";
            continue;
        }
        [[maybe_unused]] const bool lastIteration = lastSettings == &scoreSettings;
        Core::MapScore              targetScore;

        ZoneObjectList objectList;

        //group.scale(armyPercent, goldPercent);

        for (const auto& [key, val] : scoreSettings.m_score)
            targetScore[key] = val.m_target;

        //auto targetSum = totalScoreValue(targetScore);

        if (doLog)
            m_logOutput << indentBase << scoreId << " start\n";
        Core::MapScore currentScore, groupLimits;
        for (const auto& [attr, sscope] : scoreSettings.m_score)
            if (sscope.m_maxGroup != -1)
                groupLimits[attr] = sscope.m_maxGroup;

        auto isFilteredOut = [&scoreSettings](const std::string& key) {
            if (!scoreSettings.m_generatorsExclude.empty() && scoreSettings.m_generatorsExclude.contains(key))
                return true;
            if (!scoreSettings.m_generatorsInclude.empty() && !scoreSettings.m_generatorsInclude.contains(key))
                return true;
            return false;
        };

        std::vector<IObjectFactoryPtr> objectFactories;
        if (!isFilteredOut("banks"))
            objectFactories.push_back(std::make_shared<ObjectFactoryBank>(*m_map, zoneSettings.m_generators.m_banks, scoreSettings, scoreId, m_database, m_rng, &artifactPool, terrain));
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
        for (; i < iterLimit; i++) {
            if (!generateOneObject(targetScore, groupLimits, currentScore, objectFactories, objectList)) {
                if (doLog)
                    m_logOutput << indentBase << scoreId << " finished on [" << i << "] iteration\n";
                break;
            }
        }
        if (i >= iterLimit - 1)
            throw std::runtime_error("Iteration limit reached.");

        //auto deficitScore = (targetScore - currentScore);
        //auto deficitSum   = totalScoreValue(deficitScore);
        // auto allowedDeficit      = targetSum * scoreSettings.m_tolerancePercent / 100;
        targetScoreRemainingPrev = currentScore;

        if (doLog) {
            m_logOutput << indentBase << scoreId << " target score:" << targetScore << "\n";
            m_logOutput << indentBase << scoreId << " end score:" << currentScore << "\n";
            //m_logOutput << indentBase << scoreId << " deficit score:" << deficitScore << "\n";
            //m_logOutput << indentBase << scoreId << " checking deficit tolerance: " << deficitSum << " <= " << allowedDeficit << "...\n";
        }
        //if (deficitSum > allowedDeficit && 0) // @todo: redo deficit
        //    throw std::runtime_error("Deficit score for '" + scoreId + "' exceed tolerance!");

        // group pickables together.
        if (scoreSettings.m_useGuards) {
            ZoneObjectList objectListPickables;
            ZoneObjectList objectListNotPickables;

            for (auto& obj : objectList) {
                if (obj.m_object->getType() == IZoneObject::Type::Pickable || obj.m_object->getType() == IZoneObject::Type::Joinable)
                    objectListPickables.push_back(obj);
                else
                    objectListNotPickables.push_back(obj);
            }
            makeGroups(zoneSettings, objectListPickables);

            objectList = objectListNotPickables;
            objectList.insert(objectList.end(), objectListPickables.cbegin(), objectListPickables.cend());
        }
        std::vector<ZoneObjectItem*> needDistributeByRadius;
        std::vector<ZoneObjectItem*> needDistributeEqual;
        for (size_t index = 0; auto& item : objectList) {
            item.m_generatedIndex = index++;
            item.m_generatedCount = objectList.size();
            item.m_objectType     = scoreSettings.m_objectType;
            item.m_useGuards      = scoreSettings.m_useGuards;
            item.m_strongRepulse  = scoreSettings.m_strongRepulse;
            item.m_preferredHeat  = 0;
            if (!scoreSettings.m_preferredHeats.empty())
                item.m_preferredHeat = scoreSettings.m_preferredHeats[m_rng->genMinMax(0, scoreSettings.m_preferredHeats.size() - 1)];

            item.m_pickable = item.m_object->getType() == IZoneObject::Type::Pickable || item.m_object->getType() == IZoneObject::Type::Joinable;
            if (item.m_objectType == ZoneObjectType::Road) {
                if (item.m_useGuards)
                    throw std::runtime_error(scoreId + ": using guards on roads unsupported");
                if (!item.m_pickable)
                    throw std::runtime_error(scoreId + ": all items must be pickable, " + item.m_object->getId() + " is not.");
            }
            if (!item.m_pickable || item.m_useGuards)
                needDistributeByRadius.push_back(&item);
            else
                needDistributeEqual.push_back(&item);
            result.m_allIds.push_back(item.m_object->getId());
        }
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

        m_logOutput << indentBase << scoreId << " generated:" << objectList.size() << "\n";

        result.m_objects.insert(result.m_objects.end(), objectList.cbegin(), objectList.cend());
    }
    std::sort(result.m_allIds.begin(), result.m_allIds.end());
    return result;
}

bool ObjectGenerator::generateOneObject(const Core::MapScore&           targetScore,
                                        const Core::MapScore&           groupLimits,
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

    auto isScoreOverflow = [&targetScore](const Core::MapScore& current) {
        for (const auto& [key, val] : current) {
            if (!targetScore.contains(key))
                return true;
            const auto targetVal = targetScore.at(key);
            if (val > targetVal)
                return true;
        }
        return false;
    };

    auto isGroupOverflow = [&groupLimits](const Core::MapScore& current) {
        for (const auto& [key, val] : current) {
            if (!groupLimits.contains(key))
                continue;
            const auto limitVal = groupLimits.at(key);
            if (val > limitVal)
                return true;
        }
        return false;
    };

    uint64_t indexWeight = 0, baseWeight = 0;
    for (IObjectFactoryPtr& fac : objectFactories) {
        indexWeight += fac->totalFreq();
        if (indexWeight > rngFreq && fac->totalFreq()) {
            const uint64_t rngFreqForFactory = rngFreq - baseWeight;
            auto           obj               = fac->make(rngFreqForFactory);
            if (!obj)
                throw std::runtime_error("Object factory failed to make an object!");
            if (obj->getScore().empty())
                throw std::runtime_error("Object '" + obj->getId() + "' has no score!");

            Core::MapScore currentScoreTmp = currentScore + obj->getScore();
            if (isScoreOverflow(currentScoreTmp) || isGroupOverflow(obj->getScore())) {
                //m_logOutput << indent << "overflow '" << obj->getId() << "' score=" << obj->getScore() << ", current=" << currentScore << "\n";
                obj->setAccepted(false);
                return true;
            }
            currentScore = currentScoreTmp;

            //m_logOutput << indent << "add '" << obj->getId() << "' score=" << obj->getScore() << " guard=" << obj->getGuard() << "; current factory freq=" << fac->totalFreq() << ", active=" << fac->totalActiveRecords() << "\n";
            obj->setAccepted(true);
            objectList.push_back(ZoneObjectItem{ .m_object = obj });
            return true;
        }
        baseWeight = indexWeight;
    }

    assert(false);
    return false;
}

void ObjectGenerator::makeGroups(const FHRngZone& zoneSettings, ZoneObjectList& objectList) const
{
    auto makeNewObjectBundle = [this, &zoneSettings]() -> IZoneObjectGroupPtr {
        int64_t min         = zoneSettings.m_guardMin;
        int64_t max         = zoneSettings.m_guardMax;
        auto    targetGuard = min + m_rng->gen(max - min);

        auto obj = makeNewZoneObjectGroup(targetGuard, 1 + m_rng->genSmall(3), m_rng->genSmall(101));
        return obj;
    };
    ZoneObjectList groupsList;

    auto bundleGuarded      = makeNewObjectBundle();
    bool bundleGuardedEmpty = true;

    auto flushBundle = [&bundleGuarded, &groupsList, &makeNewObjectBundle, &bundleGuardedEmpty]() {
        if (bundleGuardedEmpty)
            return;
        bundleGuarded->updateMask();
        groupsList.push_back(ZoneObjectItem{ .m_object = bundleGuarded });
        bundleGuarded      = makeNewObjectBundle();
        bundleGuardedEmpty = true;
    };

    for (const auto& item : objectList) {
        if (!bundleGuardedEmpty) {
            if (bundleGuarded->tryPush(item.m_object)) {
                continue;
            }
            // can not push more
            flushBundle();
        }
        if (bundleGuarded->tryPush(item.m_object)) {
            bundleGuardedEmpty = false;
            continue;
        }
        throw std::runtime_error("sanity check failed: failed push to empty bundle.");
    }
    flushBundle();

    if (0) {
        for (auto& obj : groupsList)
            m_logOutput << "grouped: " << obj.m_object->getId() << "\n";
    }

    objectList = std::move(groupsList);
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
