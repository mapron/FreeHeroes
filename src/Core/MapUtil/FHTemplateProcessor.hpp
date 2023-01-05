/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "FHMap.hpp"

namespace FreeHeroes {

class FHTemplateProcessor {
public:
    FHTemplateProcessor(const Core::IGameDatabase* database, Core::IRandomGenerator* rng, std::ostream& logOutput);

    void run(FHMap& map) const;

private:
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
    std::ostream&                    m_logOutput;
};

}
