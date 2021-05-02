/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <memory>

namespace FreeHeroes::Sound {

class ISoundResource {
public:
    virtual ~ISoundResource() = default;

    virtual void play() const                          = 0;
    virtual void playFor(int expectedDurationMS) const = 0;
};

using ISoundResourcePtr = std::shared_ptr<const ISoundResource>;

}
