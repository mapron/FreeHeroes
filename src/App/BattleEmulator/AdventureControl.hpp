/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IAdventureControl.hpp"

#include <QObject>

namespace FreeHeroes {

namespace Gui {
class GuiAdventureArmy;
}

class AdventureControl
    : public Core::IAdventureHeroControl
    , public Core::IAdventureSquadControl {
public:
    AdventureControl(Gui::GuiAdventureArmy& army);

    void heroStackAction(StackAction action) override;

    bool heroArtifactPutOn(ArtifactPutOn putOnParams) override;
    void heroArtifactTakeOff(ArtifactTakeOff takeOffParams) override;
    void heroArtifactSwap(ArtifactSwap swapParams) override;
    void heroArtifactAssembleSet(ArtifactAssembleSet assembleSetParams) override;
    void heroArtifactDisassembleSet(ArtifactAssembleSet assembleSetParams) override;

    void setCompactFormation(bool enabled) override;

private:
    Gui::GuiAdventureArmy& m_army;
};

}
