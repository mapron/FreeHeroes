/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IAI.hpp"

#include <memory>

namespace FreeHeroes::Core {
class IBattleControl;
class IAIFactory {
public:
    virtual ~IAIFactory() = default;

    virtual std::unique_ptr<IAI> makeAI(const IAI::AIParams & params, IBattleControl & battleControl) = 0;
};

}
