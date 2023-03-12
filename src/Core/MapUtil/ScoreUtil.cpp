/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ScoreUtil.hpp"

#include "LibraryArtifact.hpp"
#include "LibrarySpell.hpp"

namespace FreeHeroes {

Core::MapScore estimateReward(const Core::Reward& reward, Core::ScoreAttr armyAttr)
{
    Core::MapScore score;
    for (const auto& [id, count] : reward.resources.data) {
        const int amount = count / id->pileSize;
        const int value  = amount * id->value;
        auto      attr   = (id->rarity == Core::LibraryResource::Rarity::Gold) ? Core::ScoreAttr::Gold : Core::ScoreAttr::Resource;
        score[attr] += value;
    }

    int64_t armyValue = 0;
    for (const auto& unit : reward.units) {
        armyValue += unit.count * unit.unit->value;
    }
    for (const auto& unit : reward.randomUnits) {
        armyValue += unit.m_value;
    }
    if (armyValue)
        score[armyAttr] = armyValue;

    if (reward.gainedExp)
        score[Core::ScoreAttr::Experience] = reward.gainedExp * 5 / 4;

    return score;
}

void estimateArtScore(Core::LibraryArtifactConstPtr art, Core::MapScore& score)
{
    auto attr = Core::ScoreAttr::ArtSupport;
    if (std::find(art->tags.cbegin(), art->tags.cend(), Core::LibraryArtifact::Tag::Stats) != art->tags.cend())
        attr = Core::ScoreAttr::ArtStat;
    if (std::find(art->tags.cbegin(), art->tags.cend(), Core::LibraryArtifact::Tag::Control) != art->tags.cend())
        attr = Core::ScoreAttr::Control;

    score[attr] += art->value;
}

void estimateSpellScore(Core::LibrarySpellConstPtr spell, Core::MapScore& score, bool asAnySpell)
{
    auto attr = Core::ScoreAttr::SpellCommon;
    if (std::find(spell->tags.cbegin(), spell->tags.cend(), Core::LibrarySpell::Tag::Control) != spell->tags.cend())
        attr = Core::ScoreAttr::Control;
    if (std::find(spell->tags.cbegin(), spell->tags.cend(), Core::LibrarySpell::Tag::OffensiveSummon) != spell->tags.cend())
        attr = Core::ScoreAttr::SpellOffensive;

    if (asAnySpell)
        score[Core::ScoreAttr::SpellAny] += spell->value;
    else
        score[attr] += spell->value;
}

void estimateSpellListScore(const std::vector<Core::LibrarySpellConstPtr>& spells, Core::MapScore& score, bool asAnySpell)
{
    for (Core::LibrarySpellConstPtr spell : spells) {
        Core::MapScore one;
        estimateSpellScore(spell, one, asAnySpell);
        for (const auto& [attr, value] : one) {
            score[attr] = std::max(score[attr], value);
        }
    }
    // make sure spell list is worth 150% of maximum value
    for (auto& [attr, value] : score) {
        value = value * 3 / 2;
    }
}

}
