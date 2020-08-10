/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureControl.hpp"

// Gui
#include "AdventureWrappers.hpp"

// Core
#include "AdventureArmy.hpp"
#include "AdventureStack.hpp"

#include <cassert>

namespace FreeHeroes {
using namespace Core;
AdventureControl::AdventureControl(Gui::GuiAdventureArmy & army)
    : m_army(army)
{

}

void AdventureControl::heroStackAction(StackAction action)
{
    auto guiSquad = m_army.getSquad();
    auto stackFrom = guiSquad->getStack(action.from->armyParams.indexInArmy);
    auto fromLibrary = stackFrom->getSource()->library;
    auto fromCount   = stackFrom->getSource()->count;
    if (action.type == StackActionType::Swap) {
        auto stackTo   = guiSquad->getStack(action.to->armyParams.indexInArmy);
        auto toLibrary   = stackTo->getSource()->library;
        auto toCount     = stackTo->getSource()->count;
        if (fromLibrary != toLibrary) {
            stackFrom->setParams(toLibrary, toCount);
            stackTo  ->setParams(fromLibrary, fromCount);
        } else {
            stackFrom->setCount(fromCount + toCount);
            stackTo  ->setCount(0);
        }
    } else if (action.type == StackActionType::EqualSplit) {
        if (fromCount < 2)
            return;
        int currentGroupCount = 0;
        int freeIndex = -1;
        int totalCount = 0;
        for (size_t i = 0; i < guiSquad->getCount(); ++i) {
            if (guiSquad->getStackUnitLibrary(i) == fromLibrary) {
                currentGroupCount++;
                totalCount += guiSquad->getStackCount(i);
            }
            if (freeIndex == -1 && !guiSquad->getStackUnitLibrary(i))
                freeIndex = i;
        }
        if (freeIndex == -1)
            return;
        const int newExistingCount = totalCount / (currentGroupCount + 1);
        const int remainCount = totalCount - newExistingCount * currentGroupCount;
        for (size_t i =0; i < guiSquad->getCount(); ++i) {
            if (guiSquad->getStackUnitLibrary(i) == fromLibrary) {
                guiSquad->getStack(i)->setCount(newExistingCount);
            }
        }
        guiSquad->getStack(freeIndex)->setParams(fromLibrary, remainCount);
    } else if (action.type == StackActionType::SplitOne) {
        if (fromCount < 2)
            return;
        for (size_t i = 0; i < guiSquad->getCount(); ++i) {
            if (!guiSquad->getStackUnitLibrary(i)) {
                guiSquad->getStack(i)->setParams(fromLibrary, 1);
                stackFrom->setCount(fromCount - 1);
                return;
            }
        }
    } else if (action.type == StackActionType::GroupTogether) {
        for (size_t i = 0; i < guiSquad->getCount(); ++i) {
            if ((int)i == action.from->armyParams.indexInArmy)
                continue;
            auto stackTo = guiSquad->getStack(action.from->armyParams.indexInArmy);

            if (stackTo->getSource()->count > 0 && stackTo->getSource()->library == fromLibrary) {
                fromCount += stackTo->getSource()->count;
                stackTo->setCount(0);
            }
        }
        stackFrom->setCount(fromCount);
    } else if (action.type == StackActionType::Delete) {
        stackFrom->setCount(0);
    }

}

void AdventureControl::setCompactFormation(bool enabled)
{
    m_army.getSquad()->setFormation(enabled);
}


bool AdventureControl::heroArtifactPutOn(ArtifactPutOn putOnParams)
{
    auto hero = m_army.getHero()->getSource();

    assert(ArtifactWearingSet::getCompatibleSlotType(putOnParams.slot) == putOnParams.bagItem->slot);
    auto freeSlots = hero->estimated.slotsInfo.freeUsed;

    auto & current = hero->artifactsOn[putOnParams.slot];
    if (current) {
        freeSlots = freeSlots + current->slotReq;
    }

    if (!putOnParams.bagItem->slotReq.canFitInto(freeSlots))
        return false;


    auto prev = current;
    if (prev && putOnParams.onlyEmpty) {
        return false;
    }
    auto & bag = hero->artifactsBag[putOnParams.bagItem];
    if (bag<=0)
        return false;

    current = putOnParams.bagItem;
    if (prev) {
        hero->artifactsBag[prev]++;
    }
    bag--;
    m_army.getHero()->refreshArtifactsModels();
    m_army.getHero()->refreshExternalChange();
    return true;
}

void AdventureControl::heroArtifactTakeOff(ArtifactTakeOff takeOffParams)
{
    auto hero = m_army.getHero()->getSource();

    auto & from = hero->artifactsOn[takeOffParams.slot];
    hero->artifactsBag[from]++;
    from = nullptr;
    m_army.getHero()->refreshArtifactsModels();
    m_army.getHero()->refreshExternalChange();
}

void AdventureControl::heroArtifactSwap( ArtifactSwap swapParams)
{
    auto hero = m_army.getHero()->getSource();

    assert(ArtifactWearingSet::getCompatibleSlotType(swapParams.slotFrom) == ArtifactWearingSet::getCompatibleSlotType(swapParams.slotTo));
    auto & from = hero->artifactsOn[swapParams.slotFrom];
    auto & to = hero->artifactsOn[swapParams.slotTo];
    std::swap(from, to);

    m_army.getHero()->refreshArtifactsModels();
    m_army.getHero()->refreshExternalChange();
}

void AdventureControl::heroArtifactAssembleSet(ArtifactAssembleSet assembleSetParams)
{
    auto hero = m_army.getHero()->getSource();
    auto & from = hero->artifactsOn[assembleSetParams.slot];
    auto setArt = from->partOfSet;
    if (!setArt)
        return;

    std::vector<ArtifactSlotType> setSlots;
    ArtifactSlotType assemblyOnSlot = ArtifactSlotType::Invalid;
    std::set<LibraryArtifactConstPtr> requiredParts(setArt->parts.cbegin(), setArt->parts.cend());
    for (auto & p : hero->artifactsOn) {
        if (requiredParts.contains(p.second)) {
            setSlots.push_back(p.first);
            requiredParts.erase(p.second);
            if (ArtifactWearingSet::getCompatibleSlotType(p.first) == setArt->slot && assemblyOnSlot == ArtifactSlotType::Invalid)
                assemblyOnSlot = p.first;
        }
    }
    if (!requiredParts.empty())
        return;

    assert(setSlots.size() == setArt->parts.size());
    assert(assemblyOnSlot != ArtifactSlotType::Invalid);
    for (auto slot : setSlots) {
        hero->artifactsOn.erase(slot);
    }
    hero->artifactsOn[assemblyOnSlot] = setArt;
    m_army.getHero()->refreshArtifactsModels();
    m_army.getHero()->refreshExternalChange();
}

void AdventureControl::heroArtifactDisassembleSet(ArtifactAssembleSet assembleSetParams)
{
    auto hero = m_army.getHero()->getSource();
    auto & from = hero->artifactsOn[assembleSetParams.slot];
    if (!from->parts.size())
        return;

    AdventureHero::ArtifactsOnMap newArts;

    auto lockedSlots =  hero->estimated.slotsInfo.extraWearing;
    lockedSlots.wearingSlots.insert(assembleSetParams.slot);
    for (auto part : from->parts) {
        auto possibleWearingSlots =  ArtifactWearingSet::getCompatibleWearingSlots(part->slot);
        ArtifactSlotType partWearing = ArtifactSlotType::Invalid;
        for (auto wearing : possibleWearingSlots) {
            if (lockedSlots.wearingSlots.contains(wearing)) {
                partWearing = wearing;
                lockedSlots.wearingSlots.erase(wearing);
                break;
            }
        }
        assert(partWearing != ArtifactSlotType::Invalid);
        newArts[partWearing] = part;
    }
    from = nullptr;
    for (auto & p : newArts) {
        assert(hero->artifactsOn[p.first] == nullptr);
        hero->artifactsOn[p.first] = p.second;
    }

    m_army.getHero()->refreshArtifactsModels();
    m_army.getHero()->refreshExternalChange();
}


}
