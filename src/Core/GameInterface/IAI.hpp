/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>

namespace FreeHeroes::Core {

class IAI {
public:
    virtual ~IAI() = default;
    struct AIParams {
        bool    useSpells               = false;
        int64_t fullKillsMultiply       = 3;
        int64_t mainKillsWeight         = 10;
        int64_t mainDamageWeight        = 3;
        int64_t retaliationKillsWeight  = -10;
        int64_t retaliationDamageWeight = -1;
        int64_t extraKillsMultiply      = 2;
        int64_t blockShooterMultiply    = 3;
    };

    virtual int  run(int stepLimit) = 0;
    virtual void runStep()          = 0;

    virtual std::string getProfiling() const = 0;
    virtual void        clearProfiling()     = 0;
};

}
