/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHPos.hpp"
#include "../FHTemplate.hpp"

#include "LibraryFwd.hpp"
#include "MapScore.hpp"
#include "ZoneObject.hpp"

#include <memory>
#include <vector>

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHMap;
struct FHRngZone;

class ObjectGenerator {
public:
    struct IObjectFactory {
        virtual ~IObjectFactory() = default;

        virtual IZoneObjectPtr makeChecked(uint64_t rngFreq, Core::MapScore& currentScore, const Core::MapScore& targetScore) = 0; // return null on fail

        virtual uint64_t totalFreq() const          = 0;
        virtual size_t   totalActiveRecords() const = 0;
    };
    using IObjectFactoryPtr = std::shared_ptr<IObjectFactory>;

public:
    ObjectGenerator(FHMap&                        map,
                    const Core::IGameDatabase*    database,
                    Core::IRandomGenerator* const rng,
                    std::ostream&                 logOutput)
        : m_map(&map)
        , m_database(database)
        , m_rng(rng)
        , m_logOutput(logOutput)
    {}

    ZoneObjectGeneration generate(const FHRngZone&             zoneSettings,
                                  Core::LibraryFactionConstPtr rewardsFaction,
                                  Core::LibraryFactionConstPtr dwellFaction,
                                  Core::LibraryTerrainConstPtr terrain,
                                  int64_t                      armyPercent,
                                  int64_t                      goldPercent) const;

    bool generateOneObject(const Core::MapScore&           targetScore,
                           Core::MapScore&                 currentScore,
                           std::vector<IObjectFactoryPtr>& objectFactories,
                           ZoneObjectList&                 objectList) const;

    void makeGroups(int64_t guardGroupLimit, int64_t guardMinToGroup, ZoneObjectList& objectList) const;
    void shuffle(std::vector<ZoneObjectItem*>& list) const;

    static bool correctObjIndex(Core::ObjectDefIndex& defIndex, const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain);
    static bool terrainViable(const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain);

    template<class T>
    struct AbstractObject;
    template<class T>
    struct AbstractObjectWithId;
    struct GroupObject;

private:
    template<class Record>
    struct AbstractFactory;
    struct ObjectFactoryBank;
    struct ObjectFactoryArtifact;
    struct ObjectFactoryResourcePile;
    struct ObjectFactoryPandora;
    struct ObjectFactoryScroll;
    struct ObjectFactoryShrine;
    struct ObjectFactoryDwelling;
    struct ObjectFactoryVisitable;
    struct ObjectFactoryMine;
    struct ObjectFactorySkillHut;

private:
    FHMap*                           m_map;
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::ostream&                    m_logOutput;
};

}
