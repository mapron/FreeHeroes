/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "AdventureHero.hpp"
#include "AdventureStack.hpp"
#include "AdventureArmy.hpp"

#include "CoreLogicExport.hpp"

namespace sol {
class state;
}

namespace FreeHeroes::Core {

class IRandomGenerator;
class IGameDatabase;

class CORELOGIC_EXPORT AdventureEstimation {
public:
    AdventureEstimation(const IGameDatabase* gameDatabase);

    struct LevelUpResult {
        bool                                       valid    = false;
        int                                        newLevel = 0;
        std::vector<LibrarySecondarySkillConstPtr> choices;
        std::vector<int>                           choicesLevels;
        HeroPrimaryParamType                       primarySkillUpdated = HeroPrimaryParamType::Attack;

        bool isValid() const noexcept { return valid; }
    };

    void calculateArmy(AdventureArmy& army, LibraryTerrainConstPtr terrain);
    void calculateDayStart(AdventureHero& hero);
    void calculateArmySummon(const AdventureArmy& army, LibraryTerrainConstPtr terrain, AdventureStackMutablePtr stack);

    void          calculateHeroLevelUp(AdventureHero& hero);
    LevelUpResult calculateHeroLevelUp(AdventureHero& hero, IRandomGenerator& rng);
    void          applyLevelUpChoice(AdventureHero& hero, LibrarySecondarySkillConstPtr skill);

private:
    void bindTypes(sol::state& lua);
    void calculateHeroStats(AdventureHero& hero);
    void calculateHeroStatsAfterSquad(AdventureHero& hero, const AdventureSquad& squad);
    void calculateSquad(AdventureSquad& squad, bool reduceExtraFactionsPenalty, LibraryTerrainConstPtr terrain);
    void calculateSquadHeroRng(AdventureSquad& squad, const AdventureHero& hero);
    void calculateSquadSpeed(AdventureSquad& squad);

private:
    const IGameDatabase* const m_gameDatabase;
    LibraryGameRulesConstPtr   m_rules;
};

}
