/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <memory>

#include "FHMap.hpp"

namespace FreeHeroes {

class ObjectGenerator {
public:
    using Score = std::map<FHScoreSettings::Attr, int64_t>;

    struct IObject {
        virtual ~IObject() = default;

        virtual void  setPos(FHPos pos) = 0;
        virtual void  place() const     = 0;
        virtual Score getScore() const  = 0;
    };

    using IObjectPtr = std::shared_ptr<IObject>;

    struct IObjectFactory {
        virtual ~IObjectFactory() = default;

        virtual IObjectPtr make() = 0;
    };
    using IObjectFactoryPtr = std::shared_ptr<IObjectFactory>;

public:
    ObjectGenerator(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : m_database(database)
        , m_rng(rng)
        , m_map(map)
    {}

    void generate();

    FHScoreSettings m_score;

    std::vector<IObjectFactoryPtr> m_objectFactories;
    std::vector<IObjectPtr>        m_objects;

private:
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    FHMap&                           m_map;
};

}
