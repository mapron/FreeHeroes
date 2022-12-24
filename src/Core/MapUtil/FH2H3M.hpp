/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "FHMap.hpp"
#include "H3MMap.hpp"

namespace FreeHeroes {

class FH2H3MConverter {
public:
    FH2H3MConverter(const Core::IGameDatabase* database, Core::IRandomGenerator* rng);

    void convertMap(const FHMap& src, H3Map& dest) const;

private:
    static H3Pos int3fromPos(FHPos pos, int xoffset = 0)
    {
        return { (uint8_t) (pos.m_x + xoffset), (uint8_t) pos.m_y, (uint8_t) pos.m_z };
    }
    std::vector<uint32_t> convertResource(const Core::ResourceAmount& amount) const;
    void                  convertReward(const Core::Reward& fhReward, MapReward& reward) const;
    void                  convertRewardHut(const Core::Reward& fhReward, MapSeerHut* hut) const;
    void                  convertQuest(const FHQuest& fhQuest, MapQuest& quest) const;

    std::vector<StackBasicDescriptor> convertStacks(const std::vector<Core::UnitWithCount>& stacks) const;
    std::vector<uint8_t>              convertPrimaryStats(const Core::HeroPrimaryParams& stats) const;

private:
    class ObjectTemplateCache;

private:
    const Core::IGameDatabase* const m_database;
    Core::IRandomGenerator* const    m_rng;
};

}
