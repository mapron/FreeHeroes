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
        const bool     unguarded = scoreSettings.m_guardPercent == 0;

        //group.scale(armyPercent, goldPercent);

        for (const auto& [key, val] : scoreSettings.m_score)
            targetScore[key] = val.m_target;

        auto targetSum = totalScoreValue(targetScore);

        if (doLog)
            m_logOutput << indentBase << scoreId << " start\n";
        Core::MapScore currentScore, groupLimits;
        for (const auto& [attr, sscope] : scoreSettings.m_score)
            if (sscope.m_maxGroup != -1)
                groupLimits[attr] = sscope.m_maxGroup;

        std::vector<IObjectFactoryPtr> objectFactories;
        objectFactories.push_back(std::make_shared<ObjectFactoryBank>(*m_map, zoneSettings.m_generators.m_banks, scoreSettings, m_database, m_rng, &artifactPool, terrain));
        objectFactories.push_back(std::make_shared<ObjectFactoryArtifact>(*m_map, zoneSettings.m_generators.m_artifacts, scoreSettings, m_database, m_rng, &artifactPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryResourcePile>(*m_map, zoneSettings.m_generators.m_resources, scoreSettings, m_database, m_rng));
        objectFactories.push_back(std::make_shared<ObjectFactoryPandora>(*m_map, zoneSettings.m_generators.m_pandoras, scoreSettings, m_database, m_rng, rewardsFaction));
        objectFactories.push_back(std::make_shared<ObjectFactoryShrine>(*m_map, zoneSettings.m_generators.m_shrines, scoreSettings, m_database, m_rng, &spellPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryScroll>(*m_map, zoneSettings.m_generators.m_scrolls, scoreSettings, m_database, m_rng, &spellPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryDwelling>(*m_map, zoneSettings.m_generators.m_dwellings, scoreSettings, m_database, m_rng, dwellFaction));
        objectFactories.push_back(std::make_shared<ObjectFactoryVisitable>(*m_map, zoneSettings.m_generators.m_visitables, scoreSettings, m_database, m_rng, terrain));
        objectFactories.push_back(std::make_shared<ObjectFactoryMine>(*m_map, zoneSettings.m_generators.m_mines, scoreSettings, m_database, m_rng, terrain));
        objectFactories.push_back(std::make_shared<ObjectFactorySkillHut>(*m_map, zoneSettings.m_generators.m_skillHuts, scoreSettings, m_database, m_rng));

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

        auto deficitScore        = (targetScore - currentScore);
        auto deficitSum          = totalScoreValue(deficitScore);
        auto allowedDeficit      = targetSum * scoreSettings.m_tolerancePercent / 100;
        targetScoreRemainingPrev = currentScore;

        if (doLog) {
            m_logOutput << indentBase << scoreId << " target score:" << targetScore << "\n";
            m_logOutput << indentBase << scoreId << " end score:" << currentScore << "\n";
            m_logOutput << indentBase << scoreId << " deficit score:" << deficitScore << "\n";
            m_logOutput << indentBase << scoreId << " checking deficit tolerance: " << deficitSum << " <= " << allowedDeficit << "...\n";
        }
        if (deficitSum > allowedDeficit)
            throw std::runtime_error("Deficit score for '" + scoreId + "' exceed tolerance!");

        ZoneObjectList objectListPickables;
        ZoneObjectList objectListNotPickables;
        for (auto& obj : objectList) {
            result.m_allIds.push_back(obj->getId());
            if (obj->getType() == IZoneObject::Type::Pickable)
                objectListPickables.push_back(obj);
            else
                objectListNotPickables.push_back(obj);
        }

        if (unguarded) {
            const size_t pickSize = objectListPickables.size();
            const size_t roadSize = pickSize / 3; // @todo: customize in template
            m_logOutput << indentBase << " unguarded roadPickables:" << roadSize << "\n";
            m_logOutput << indentBase << " unguarded objectListPickables:" << (pickSize - roadSize) << "\n";
            m_logOutput << indentBase << " unguarded objectListNotPickables:" << objectListNotPickables.size() << "\n";
            result.m_roadUnguardedPickables.insert(result.m_roadUnguardedPickables.end(), objectListPickables.cbegin(), objectListPickables.cbegin() + roadSize);
            result.m_segmentsUnguardedPickables.insert(result.m_segmentsUnguardedPickables.end(), objectListPickables.cbegin() + roadSize, objectListPickables.cend());
            result.m_segmentsUnguardedNonPickables.insert(result.m_segmentsUnguardedNonPickables.end(), objectListNotPickables.cbegin(), objectListNotPickables.cend());
        } else {
            makeGroups(zoneSettings, objectListPickables);

            m_logOutput << indentBase << " guarded objectListPickables:" << objectListPickables.size() << "\n";
            m_logOutput << indentBase << " guarded objectListNotPickables:" << objectListNotPickables.size() << "\n";
            result.m_segmentsGuarded.insert(result.m_segmentsGuarded.end(), objectListPickables.cbegin(), objectListPickables.cend());
            result.m_segmentsGuarded.insert(result.m_segmentsGuarded.end(), objectListNotPickables.cbegin(), objectListNotPickables.cend());
        }
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
                // m_logOutput << indent << "overflow '" << obj->getId() << "' score=" << obj->getScore() << ", current=" << currentScore << "\n";
                obj->setAccepted(false);
                return true;
            }
            currentScore = currentScoreTmp;

            // m_logOutput << indent << "add '" << obj->getId() << "' score=" << obj->getScore() << " guard=" << obj->getGuard() << "; current factory freq=" << fac->totalFreq() << ", active=" << fac->totalActiveRecords() << "\n";
            obj->setAccepted(true);
            objectList.push_back(obj);
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

    auto pushIfNeeded = [&bundleGuarded, &groupsList, &makeNewObjectBundle, &bundleGuardedEmpty]() {
        if (bundleGuardedEmpty)
            return;
        bundleGuarded->updateMask();
        groupsList.push_back(bundleGuarded);
        bundleGuarded      = makeNewObjectBundle();
        bundleGuardedEmpty = true;
    };

    for (const auto& item : objectList) {
        if (bundleGuarded->tryPush(item)) {
            bundleGuardedEmpty = false;
            continue;
        }
        if (bundleGuardedEmpty)
            throw std::runtime_error("sanity check failed: failed push to empty bundle.");

        pushIfNeeded();
    }
    pushIfNeeded();

    if (0) {
        for (auto& obj : groupsList)
            m_logOutput << "grouped: " << obj->getId() << "\n";
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
