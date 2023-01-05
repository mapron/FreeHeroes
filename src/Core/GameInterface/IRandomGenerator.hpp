/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>

namespace FreeHeroes::Core {

class IRandomGenerator {
public:
    virtual ~IRandomGenerator() = default;

    virtual void setSeed(uint64_t seedValue) = 0;

    virtual uint64_t getSeed() const = 0;

    virtual void makeGoodSeed() = 0;

    virtual std::vector<uint8_t> serialize() const = 0;

    virtual void deserialize(const std::vector<uint8_t>& state) = 0;

    virtual uint64_t              gen(uint64_t max)                      = 0;
    virtual uint64_t              genSumN(size_t n, uint64_t max)        = 0;
    virtual std::vector<uint64_t> genSequence(size_t size, uint64_t max) = 0;

    virtual uint8_t              genSmall(uint8_t max)                      = 0;
    virtual uint64_t             genSumSmallN(size_t n, uint8_t max)        = 0;
    virtual std::vector<uint8_t> genSmallSequence(size_t size, uint8_t max) = 0;

    virtual int64_t genDispersed(int64_t avg, uint64_t dispersion)
    {
        return avg - dispersion + gen(dispersion * 2);
    }
};

using IRandomGeneratorPtr = std::shared_ptr<IRandomGenerator>;

class IRandomGeneratorFactory {
public:
    virtual ~IRandomGeneratorFactory() = default;

    virtual IRandomGeneratorPtr create() const = 0;
};

}
