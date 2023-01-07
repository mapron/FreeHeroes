/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "RandomGenerator.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4293)
#pragma warning(disable : 4554)

#include <intrin.h>

#endif

inline int __hacked_clz(uint64_t n)
{
#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
    // MSVC does not have __buitin_clzll. Use _BitScanReverse64.
    unsigned long result = 0; // NOLINT(runtime/int)
    if (_BitScanReverse64(&result, n)) {
        return 63 - result;
    }
    return 64;
#elif defined(_MSC_VER) && !defined(__clang__)
    // MSVC does not have __buitin_clzll. Compose two calls to _BitScanReverse
    unsigned long result = 0; // NOLINT(runtime/int)
    if ((n >> 32) && _BitScanReverse(&result, n >> 32)) {
        return 31 - result;
    }
    if (_BitScanReverse(&result, n)) {
        return 63 - result;
    }
    return 64;
#elif defined(__GNUC__) || defined(__clang__)
    // Use __builtin_clzll, which uses the following instructions:
    //  x86: bsr
    //  ARM64: clz
    //  PPC: cntlzd
    static_assert(sizeof(unsigned long long) == sizeof(n), // NOLINT(runtime/int)
                  "__builtin_clzll does not take 64-bit arg");

    // Handle 0 as a special case because __builtin_clzll(0) is undefined.
    if (n == 0) {
        return 64;
    }
    return __builtin_clzll(n);
#else
#error "Unsupported compiler, need fallback."
#endif
}

int __hacked_clz(uint32_t n)
{
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long result = 0; // NOLINT(runtime/int)
    if (_BitScanReverse(&result, n)) {
        return 31 - result;
    }
    return 32;
#elif defined(__GNUC__) || defined(__clang__)
    // Use __builtin_clz, which uses the following instructions:
    //  x86: bsr
    //  ARM64: clz
    //  PPC: cntlzd
    static_assert(sizeof(int) == sizeof(n), "__builtin_clz does not take 32-bit arg");

    // Handle 0 as a special case because __builtin_clz(0) is undefined.
    if (n == 0) {
        return 32;
    }
    return __builtin_clz(n);
#else
#error "Unsupported compiler, need fallback."
#endif
}

#include "hacked_libcxx.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <random>
#include <iostream>

namespace FreeHeroes::Core {

using Distribution64 = hacked_libcxx::uniform_int_distribution<uint64_t>;
using Distribution8  = hacked_libcxx::uniform_int_distribution<uint8_t>;

class RandomGenerator : public IRandomGenerator {
public:
    RandomGenerator();
    ~RandomGenerator();

    void setSeed(uint64_t seedValue) override
    {
        seed = seedValue;
        engine.seed(seedValue);
    }

    uint64_t getSeed() const override
    {
        return seed;
    }

    void makeGoodSeed() override
    {
        std::random_device rd;
        seed = static_cast<uint64_t>(rd()) | (static_cast<uint64_t>(rd()) << 32);
        engine.seed(seed);
    }

    std::vector<uint8_t> serialize() const override
    {
        std::vector<uint8_t> result;

        return result;
    }

    void deserialize(const std::vector<uint8_t>& state) override
    {
        (void) state; // @todo: when we start writing networking code that become very handy.
    }

    uint64_t gen(uint64_t max) override
    {
        if (max == 0)
            return 0;
        return Distribution64(0, max)(engine);
    }

    uint64_t genSumN(size_t n, uint64_t max) override
    {
        uint64_t       result = 0;
        Distribution64 dist(0, max);
        for (size_t i = 0; i < n; ++i)
            result += engine();
        return result;
    }

    std::vector<uint64_t> genSequence(size_t size, uint64_t max) override
    {
        std::vector<uint64_t> result(size);
        Distribution64        dist(0, max);
        for (size_t i = 0; i < size; ++i)
            result[i] = dist(engine);

        return result;
    }

    uint8_t genSmall(uint8_t max) override
    {
        if (max == 0)
            return 0;
        return Distribution8(0, max)(engine);
    }
    uint64_t genSumSmallN(size_t n, uint8_t max) override
    {
        uint64_t      result = 0;
        Distribution8 dist(0, max);
        for (size_t i = 0; i < n; ++i) {
            result += dist(engine);
        }

        return result;
    }

    std::vector<uint8_t> genSmallSequence(size_t size, uint8_t max) override
    {
        std::vector<uint8_t> result(size);
        Distribution8        dist(0, max);
        for (size_t i = 0; i < size; ++i)
            result[i] = dist(engine);

        return result;
    }

private:
    hacked_libcxx::mt19937_64 engine;
    uint64_t                  seed = hacked_libcxx::mt19937_64::default_seed;
};

RandomGenerator::RandomGenerator()
{
}

RandomGenerator::~RandomGenerator() = default;

IRandomGeneratorPtr RandomGeneratorFactory::create() const
{
    return std::make_shared<RandomGenerator>();
}

}
