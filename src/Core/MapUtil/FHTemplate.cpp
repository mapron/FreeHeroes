/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FHTemplate.hpp"

#include "FHTemplateReflection.hpp"

namespace FreeHeroes {
using namespace Mernel;

std::string FHScoreSettings::attrToString(FHScoreAttr attr)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(attr);
    return std::string(str.data(), str.size());
}

void FHTemplate::rescaleToSize(int newMapSize, int oldWidth, int oldHeight)
{
    const int wmult = newMapSize;
    const int wdiv  = oldWidth;

    const int hmult = newMapSize;
    const int hdiv  = oldHeight;

    for (auto& [key, rngZone] : m_zones) {
        rngZone.m_centerAvg.m_x = rngZone.m_centerAvg.m_x * wmult / wdiv;
        rngZone.m_centerAvg.m_y = rngZone.m_centerAvg.m_y * hmult / hdiv;

        rngZone.m_centerDispersion.m_x = rngZone.m_centerDispersion.m_x * wmult / wdiv;
        rngZone.m_centerDispersion.m_y = rngZone.m_centerDispersion.m_y * hmult / hdiv;
    }
}

FHScore operator+(const FHScore& l, const FHScore& r)
{
    FreeHeroes::FHScore result = l;

    for (const auto& [key, val] : r)
        result[key] += val;

    return result;
}

FHScore operator-(const FHScore& l, const FHScore& r)
{
    FreeHeroes::FHScore result = l;

    for (const auto& [key, val] : r) {
        result[key] -= val;
        if (!result[key])
            result.erase(key);
    }

    return result;
}

std::ostream& operator<<(std::ostream& stream, const FHScore& score)
{
    stream << "{";
    for (bool start = false; const auto& [key, val] : score) {
        if (start)
            stream << ", ";
        stream << FreeHeroes::FHScoreSettings::attrToString(key) << "=" << val;
        start = true;
    }
    stream << "} (sum=" << totalScoreValue(score) << ")";
    return stream;
}

int64_t maxScoreValue(const FHScore& score)
{
    int64_t result = 0;
    for (const auto& [attr, value] : score)
        result = std::max(result, value);
    return result;
}

int64_t totalScoreValue(const FHScore& score)
{
    int64_t result = 0;
    for (const auto& [attr, value] : score)
        result += value;
    return result;
}

}
