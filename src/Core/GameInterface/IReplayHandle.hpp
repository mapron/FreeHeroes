/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <cstdint>

namespace FreeHeroes::Core {

class IReplayHandle
{
public:
    virtual ~IReplayHandle() = default;

    virtual void rewindToStart() = 0;
    virtual size_t getSize() const = 0;
    virtual size_t getPos() const = 0;
    virtual bool executeCurrent() = 0;
};

}
