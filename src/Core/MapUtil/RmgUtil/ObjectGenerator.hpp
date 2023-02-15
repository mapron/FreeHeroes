/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "../FHPos.hpp"

#include "LibraryFwd.hpp"
#include "MapScore.hpp"

#include <memory>
#include <vector>

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHMap;
struct FHRngZone;
struct FHScoreSettings;

class ObjectGenerator {
public:
    struct IObject {
        virtual ~IObject() = default;

        enum class Type
        {
            Visitable,
            Pickable,
            Joinable,  // monster join
            Removable, // Prison
        };

        virtual void           setPos(FHPos pos) = 0;
        virtual void           place() const     = 0;
        virtual Core::MapScore getScore() const  = 0;
        virtual void           disable()         = 0;
        virtual std::string    getId() const     = 0;
        virtual int64_t        getGuard() const  = 0;
        virtual Type           getType() const   = 0;
        virtual FHPos          getOffset() const { return FHPos{}; }

        virtual Core::LibraryObjectDefConstPtr getDef() const { return nullptr; }
    };

    using IObjectPtr = std::shared_ptr<IObject>;

    struct IObjectFactory {
        virtual ~IObjectFactory() = default;

        virtual IObjectPtr make(uint64_t rngFreq)     = 0;
        virtual uint64_t   totalFreq() const          = 0;
        virtual size_t     totalActiveRecords() const = 0;
    };
    using IObjectFactoryPtr = std::shared_ptr<IObjectFactory>;

    struct ObjectGroup {
        std::vector<IObjectPtr> m_objects;
        int                     m_guardPercent = 100;
        std::string             m_id;
        Core::MapScore          m_targetScore;
        int64_t                 m_targetScoreTotal = 0;
        const FHScoreSettings*  m_scoreSettings    = nullptr;
    };

public:
    ObjectGenerator(FHMap&                        map,
                    const Core::IGameDatabase*    database,
                    Core::IRandomGenerator* const rng,
                    std::ostream&                 logOutput)
        : m_map(map)
        , m_database(database)
        , m_rng(rng)
        , m_logOutput(logOutput)
    {}

    void generate(const FHRngZone&             zoneSettings,
                  Core::LibraryFactionConstPtr mainFaction,
                  Core::LibraryFactionConstPtr rewardsFaction,
                  Core::LibraryTerrainConstPtr terrain);

    bool generateOneObject(const Core::MapScore& targetScore, Core::MapScore& currentScore, std::vector<IObjectFactoryPtr>& objectFactories, ObjectGroup& group);

    static bool correctObjIndex(Core::ObjectDefIndex& defIndex, const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain);
    static bool terrainViable(const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain);

    std::vector<ObjectGroup> m_groups;

    template<class T>
    struct AbstractObject;
    template<class T>
    struct AbstractObjectWithId;

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

private:
    FHMap&                           m_map;
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::ostream&                    m_logOutput;
};

}
