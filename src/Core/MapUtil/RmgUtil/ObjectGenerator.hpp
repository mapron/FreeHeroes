/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <memory>

#include "../FHMap.hpp"

namespace FreeHeroes {

class ObjectGenerator {
public:
    struct IObject {
        virtual ~IObject() = default;

        virtual void        setPos(FHPos pos) = 0;
        virtual void        place() const     = 0;
        virtual FHScore     getScore() const  = 0;
        virtual void        disable()         = 0;
        virtual std::string getId() const     = 0;
        virtual int64_t     getGuard() const  = 0;
    };

    using IObjectPtr = std::shared_ptr<IObject>;

    struct IObjectFactory {
        virtual ~IObjectFactory() = default;

        virtual IObjectPtr make(uint64_t rngFreq)     = 0;
        virtual uint64_t   totalFreq() const          = 0;
        virtual size_t     totalActiveRecords() const = 0;
    };
    using IObjectFactoryPtr = std::shared_ptr<IObjectFactory>;

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

    void generate(const FHRngZone& zoneSettings);

    std::vector<IObjectPtr> m_objects;

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

private:
    FHMap&                           m_map;
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::ostream&                    m_logOutput;
};

}
