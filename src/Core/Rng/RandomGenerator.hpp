/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IRandomGenerator.hpp"

#include "CoreRngExport.hpp"

#include <memory>

namespace FreeHeroes::Core {

class CORERNG_EXPORT RandomGeneratorFactory : public IRandomGeneratorFactory {
public:
    IRandomGeneratorPtr create() const override;
};

}
