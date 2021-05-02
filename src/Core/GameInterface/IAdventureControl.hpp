/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureFwd.hpp"
#include "LibraryArtifact.hpp"

#include <cstddef>

namespace FreeHeroes::Core {

class IAdventureSquadControl {
public:
    virtual ~IAdventureSquadControl() = default;

    enum class StackActionType
    {
        Delete,
        Swap,
        EqualSplit,
        SplitOne,
        GroupTogether
    };

    struct StackAction {
        StackActionType        type;
        AdventureStackConstPtr from  = nullptr;
        AdventureStackConstPtr to    = nullptr;
        size_t                 count = 0;
    };

    virtual void heroStackAction(StackAction action) = 0;

    virtual void setCompactFormation(bool enabled) = 0;
};

class IAdventureHeroControl {
public:
    virtual ~IAdventureHeroControl() = default;

    struct ArtifactPutOn {
        ArtifactSlotType        slot;
        LibraryArtifactConstPtr bagItem   = nullptr;
        bool                    onlyEmpty = false;
    };
    virtual bool heroArtifactPutOn(ArtifactPutOn putOnParams) = 0;

    struct ArtifactTakeOff {
        ArtifactSlotType slot;
    };
    virtual void heroArtifactTakeOff(ArtifactTakeOff takeOffParams) = 0;

    struct ArtifactSwap {
        ArtifactSlotType slotFrom;
        ArtifactSlotType slotTo;
    };
    virtual void heroArtifactSwap(ArtifactSwap swapParams) = 0;

    struct ArtifactAssembleSet {
        ArtifactSlotType slot;
    };
    virtual void heroArtifactAssembleSet(ArtifactAssembleSet assembleSetParams)    = 0;
    virtual void heroArtifactDisassembleSet(ArtifactAssembleSet assembleSetParams) = 0;
};

}
