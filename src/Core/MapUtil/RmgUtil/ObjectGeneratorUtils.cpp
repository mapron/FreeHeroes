/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ObjectGeneratorUtils.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "../ScoreUtil.hpp"

namespace FreeHeroes {

bool ArtifactPool::okFilter(Core::LibraryArtifactConstPtr art, bool enableFilter, const FHScoreSettings& scoreSettings)
{
    if (!enableFilter)
        return true;

    Core::MapScore score;
    estimateArtScore(art, score);

    bool isValid = scoreSettings.isValidScore(score);
    return isValid;
}

ArtifactPool::ArtifactPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
    : m_rng(rng)
{
    for (auto* art : database->artifacts()->records()) {
        if (map.m_disabledArtifacts.isDisabled(map.m_isWaterMap, art))
            continue;

        m_artifacts.push_back(art);
    }
}

AcceptableArtifact ArtifactPool::make(const Core::ArtifactFilter& pool, const Core::ArtifactFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings)
{
    ArtList artList = pool.filterPossible(m_artifacts);
    if (artList.empty())
        return {};

    ArtifactSet artSet(artList.cbegin(), artList.cend());
    m_pools[artSet].m_artList = artList;
    return m_pools[artSet].make(filter, m_rng, enableFilter, scoreSettings);
}

bool ArtifactPool::isEmpty(const Core::ArtifactFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings) const
{
    ArtList artList = filter.filterPossible(m_artifacts);
    if (artList.empty())
        return true;

    ArtList artListFiltered;
    for (auto art : artList) {
        if (okFilter(art, enableFilter, scoreSettings))
            artListFiltered.push_back(art);
    }

    return artListFiltered.empty();
}

AcceptableArtifact ArtifactPool::SubPool::make(const Core::ArtifactFilter& filter, Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
{
    bool hasReset = false;
    if (m_current.empty()) {
        m_current = m_artList;
        hasReset  = true;
    }
    auto art = makeOne(filter, rng, enableFilter, scoreSettings);

    while (!art.m_art) {
        if (m_current.empty() || art.m_forceReset) {
            if (hasReset)
                return {};
            m_current = m_artList;
            hasReset  = true;
        }
        art = makeOne(filter, rng, enableFilter, scoreSettings);
    }
    if (!art.m_art) {
        return {};
    }
    return art;
}

AcceptableArtifact ArtifactPool::SubPool::makeOne(const Core::ArtifactFilter& filter, Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
{
    if (!m_currentHigh.empty()) {
        auto art = makeOne(m_currentHigh, filter, rng, enableFilter, scoreSettings);
        if (art.m_art) {
            art.m_forceReset = false;
            return art;
        }
    }
    return makeOne(m_current, filter, rng, enableFilter, scoreSettings);
}

AcceptableArtifact ArtifactPool::SubPool::makeOne(ArtList& current, const Core::ArtifactFilter& filter, Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
{
    ArtList currentFiltered = filter.filterPossible(current);
    if (currentFiltered.empty())
        return { .m_forceReset = true };

    Core::LibraryArtifactConstPtr art = nullptr;

    auto index = rng->gen(currentFiltered.size() - 1);
    art        = currentFiltered[index];
    auto it    = std::find(current.begin(), current.end(), art);
    current.erase(it);

    if (!okFilter(art, enableFilter, scoreSettings))
        return {};

    return { .m_art = art, .m_onDiscard = [this, art] { m_currentHigh.push_back(art); } };
}

bool SpellPool::okFilter(Core::LibrarySpellConstPtr spell, bool asAnySpell, const FHScoreSettings& scoreSettings)
{
    Core::MapScore score;
    estimateSpellScore(spell, score, asAnySpell);

    bool isValid = scoreSettings.isValidScore(score);
    return isValid;
}

SpellPool::SpellPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
    : m_rng(rng)
{
    for (auto* spell : database->spells()->records()) {
        if (map.m_disabledSpells.isDisabled(map.m_isWaterMap, spell))
            continue;
        if (!spell->isTeachable)
            continue;

        m_spells.push_back(spell);
    }
}

SpellPool::SpellPool(const std::set<std::string>& ids, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
    : m_rng(rng)
{
    for (auto* spell : database->spells()->records()) {
        if (!ids.contains(spell->id))
            continue;
        if (!spell->isTeachable)
            continue;

        m_spells.push_back(spell);
    }
}

Core::LibrarySpellConstPtr SpellPool::make(const Core::SpellFilter& filter, bool asAnySpell, const FHScoreSettings& scoreSettings)
{
    SpellList spellList = filter.filterPossible(m_spells);
    if (spellList.empty())
        return nullptr;

    SpellSet artSet(spellList.cbegin(), spellList.cend());
    m_pools[artSet].m_spellList = spellList;
    return m_pools[artSet].make(m_rng, asAnySpell, scoreSettings);
}

bool SpellPool::isEmpty(const Core::SpellFilter& filter, bool asAnySpell, const FHScoreSettings& scoreSettings) const
{
    SpellList spellList = filter.filterPossible(m_spells);
    if (spellList.empty())
        return true;

    SpellList spellListFiltered;
    for (auto spell : spellList) {
        if (okFilter(spell, asAnySpell, scoreSettings))
            spellListFiltered.push_back(spell);
    }

    return spellListFiltered.empty();
}

Core::LibrarySpellConstPtr SpellPool::SubPool::make(Core::IRandomGenerator* rng, bool asAnySpell, const FHScoreSettings& scoreSettings)
{
    bool hasReset = false;
    if (m_current.empty()) {
        m_current = m_spellList;
        hasReset  = true;
    }
    auto art = makeOne(rng, asAnySpell, scoreSettings);

    while (!art) {
        if (m_current.empty()) {
            if (hasReset)
                return nullptr;
            m_current = m_spellList;
            hasReset  = true;
        }
        art = makeOne(rng, asAnySpell, scoreSettings);
    }

    return art;
}

Core::LibrarySpellConstPtr SpellPool::SubPool::makeOne(Core::IRandomGenerator* rng, bool asAnySpell, const FHScoreSettings& scoreSettings)
{
    auto index = rng->gen(m_current.size() - 1);
    auto art   = m_current[index];
    m_current.erase(m_current.begin() + index);
    if (!okFilter(art, asAnySpell, scoreSettings))
        return nullptr;

    return art;
}

}
