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
    FH2H3MConverter(const Core::IGameDatabase* database);

    void convertMap(const FHMap& src, H3Map& dest) const;

private:
    static H3Pos int3fromPos(FHPos pos, int xoffset = 0)
    {
        return { (uint8_t) (pos.m_x + xoffset), (uint8_t) pos.m_y, (uint8_t) pos.m_z };
    }
    std::vector<uint32_t> convertResources(const Core::ResourceAmount& amount) const;
    void                  convertReward(const Core::Reward& fhReward, MapReward& reward) const;
    void                  convertRewardHut(const Core::Reward& fhReward, MapSeerHut::MapQuestWithReward& questWithReward) const;
    void                  convertQuest(const FHQuest& fhQuest, MapQuest& quest) const;
    void                  convertEvent(const FHGlobalMapEvent& fhEvent, GlobalMapEvent& event) const;
    void                  convertMessage(const FHMessageWithBattle& fhMessage, MapMessage& message) const;
    void                  convertSquad(const Core::AdventureSquad& squad, StackSetFixed& fixedStacks) const;

    void convertHeroArtifacts(const Core::AdventureHero& hero, HeroArtSet& artSet) const;
    void convertVisitableRewards(const FHVisitable& fhVisitable, MapVisitableWithReward& visitable) const;

    std::vector<StackBasicDescriptor> convertStacks(const std::vector<Core::UnitWithCount>& stacks) const;
    std::vector<uint8_t>              convertPrimaryStats(const Core::HeroPrimaryParams& stats) const;

    uint32_t convertArtifact(Core::LibraryArtifactConstPtr id) const;

private:
    class ObjectTemplateCache;

private:
    const Core::IGameDatabase* const m_database;
};

}
