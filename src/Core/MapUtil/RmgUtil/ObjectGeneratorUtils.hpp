/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ObjectGenerator.hpp"

#include "../FHMap.hpp"

#include <functional>
#include <iostream>

namespace FreeHeroes {

template<class Child>
struct CommonRecord {
    bool     m_enabled   = true;
    uint64_t m_frequency = 0;
    uint64_t m_attempts  = 1;

    int m_generatedCounter = 0;
    int m_minLimit         = -1;
    int m_maxLimit         = -1;

    Child& setFreq(int freq)
    {
        m_frequency = freq;
        return static_cast<Child&>(*this);
    }
};

template<class Record>
struct CommonRecordList {
    std::vector<Record>        m_records;
    size_t                     m_active    = 0;
    uint64_t                   m_frequency = 0;
    std::map<uint64_t, size_t> m_index;

    void updateFrequency()
    {
        m_frequency = 0;
        m_active    = 0;
        m_index.clear();
        for (size_t i = 0; const Record& rec : m_records) {
            auto freq = rec.m_frequency;
            if (rec.m_enabled && freq > 0) {
                if (rec.m_minLimit > 0) {
                    if (rec.m_generatedCounter < rec.m_minLimit)
                        freq = 1000000;
                }
                m_index[m_frequency] = i;
                m_frequency += freq;
                m_active++;
            }
            i++;
        }
    }

    size_t getFreqIndex(uint64_t rngFreq) const
    {
        if (rngFreq == 0)
            return m_index.at(0);
        auto it = m_index.lower_bound(rngFreq);
        it--;
        const size_t lower = it->second;
        return lower;
    }

    void onDisable(Record& record)
    {
        record.m_attempts--;
        if (record.m_attempts == 0) {
            record.m_enabled = false;
            updateFrequency();
        }
    }

    void onAccept(Record& record)
    {
        record.m_generatedCounter++;
        if (record.m_maxLimit != -1) {
            if (record.m_generatedCounter >= record.m_maxLimit) {
                record.m_enabled = false;
                updateFrequency();
            }
        }
    }
};

struct AcceptableArtifact {
    Core::LibraryArtifactConstPtr m_art        = nullptr;
    std::function<void()>         m_onDiscard  = [] {};
    bool                          m_forceReset = false;
};

class ArtifactPool {
public:
    using ArtifactSet = std::set<Core::LibraryArtifactConstPtr>;
    using ArtList     = std::vector<Core::LibraryArtifactConstPtr>;

    static bool okFilter(Core::LibraryArtifactConstPtr art, bool enableFilter, const FHScoreSettings& scoreSettings);

    ArtifactPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng);

    AcceptableArtifact make(const Core::ArtifactFilter& pool, const Core::ArtifactFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings);
    bool               isEmpty(const Core::ArtifactFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings) const;

private:
    struct SubPool {
        ArtList m_artList;
        ArtList m_current;
        ArtList m_currentHigh;

        AcceptableArtifact make(const Core::ArtifactFilter& filter, Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings);

        AcceptableArtifact makeOne(const Core::ArtifactFilter& filter, Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings);

        AcceptableArtifact makeOne(ArtList& current, const Core::ArtifactFilter& filter, Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings);
    };
    std::map<ArtifactSet, SubPool> m_pools;
    Core::IRandomGenerator* const  m_rng;

    ArtList m_artifacts;
};

class SpellPool {
public:
    using SpellSet  = std::set<Core::LibrarySpellConstPtr>;
    using SpellList = std::vector<Core::LibrarySpellConstPtr>;

    static bool okFilter(Core::LibrarySpellConstPtr spell, bool asAnySpell, const FHScoreSettings& scoreSettings);

    SpellPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng);
    SpellPool(const std::set<std::string>& ids, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng);

    Core::LibrarySpellConstPtr make(const Core::SpellFilter& filter, bool asAnySpell, const FHScoreSettings& scoreSettings);
    bool                       isEmpty(const Core::SpellFilter& filter, bool asAnySpell, const FHScoreSettings& scoreSettings) const;

private:
    struct SubPool {
        SpellList m_spellList;
        SpellList m_current;

        Core::LibrarySpellConstPtr make(Core::IRandomGenerator* rng, bool asAnySpell, const FHScoreSettings& scoreSettings);

        Core::LibrarySpellConstPtr makeOne(Core::IRandomGenerator* rng, bool asAnySpell, const FHScoreSettings& scoreSettings);
    };
    std::map<SpellSet, SubPool>   m_pools;
    Core::IRandomGenerator* const m_rng;

    SpellList m_spells;
};

class FactionPool {
public:
    std::set<Core::LibraryFactionConstPtr> m_factions;
    size_t                                 m_limit = 0;

    void addUnit(Core::LibraryUnitConstPtr unit)
    {
        m_factions.insert(unit->faction);
    }
    void addUnits(const Core::Reward& reward)
    {
        for (auto& unit : reward.units) {
            m_factions.insert(unit.unit->faction);
        }
    }
    bool isAllowed(Core::LibraryUnitConstPtr unit) const
    {
        if (m_factions.contains(unit->faction))
            return true;
        return m_factions.size() < m_limit;
    }
    bool isAllowed(const Core::Reward& reward) const
    {
        size_t nonExistent = 0;
        for (auto& unit : reward.units) {
            if (!m_factions.contains(unit.unit->faction))
                nonExistent++;
        }
        return m_factions.size() + nonExistent <= m_limit;
    }
};

template<class T>
struct ObjectGenerator::AbstractObject : public IZoneObject {
    virtual Core::LibraryObjectDefConstPtr getDef() const { return nullptr; }

    Mask getVisitableMask() const override
    {
        Core::CombinedMask combined = getMask();
        Mask               result;
        for (const auto& point : combined.m_visitable)
            result.insert(FHPos{ point.m_x, point.m_y });
        return result;
    }
    Mask getBlockedUnvisitableMask() const override
    {
        Core::CombinedMask combined = getMask();
        Mask               result;
        for (const auto& point : combined.m_blocked)
            if (!combined.m_visitable.contains(point))
                result.insert(FHPos{ point.m_x, point.m_y });
        return result;
    }

    virtual Core::CombinedMask getMask() const
    {
        auto def = getDef();
        assert(def);
        if (!def)
            return {};
        return def->combinedMask;
    }

    void place(FHPos pos) const override
    {
        auto obj  = m_obj;
        obj.m_pos = pos;
        m_map->m_objects.container<T>().push_back(std::move(obj));
    }
    Core::MapScore getScore() const override { return m_obj.m_score; }

    void setAccepted(bool accepted) override
    {
        if (accepted)
            m_onAccept();
        else
            m_onDisable();
    }
    int64_t getGuard() const override { return m_obj.m_guard; }

    T                     m_obj;
    FHMap*                m_map       = nullptr;
    std::function<void()> m_onDisable = [] {};
    std::function<void()> m_onAccept  = [] {};
};

template<class T>
struct ObjectGenerator::AbstractObjectWithId : public ObjectGenerator::AbstractObject<T> {
    std::string getId() const override { return this->m_obj.m_id->id; }
};

template<class Record>
struct ObjectGenerator::AbstractFactory : public IObjectFactory {
    AbstractFactory(FHMap&                        map,
                    const FHScoreSettings&        scoreSettings,
                    const std::string&            scoreId,
                    const Core::IGameDatabase*    database,
                    Core::IRandomGenerator* const rng)
        : m_map(map)
        , m_scoreSettings(scoreSettings)
        , m_scoreId(scoreId)
        , m_database(database)
        , m_rng(rng)
    {}

    virtual IZoneObjectPtr make(uint64_t rngFreq) = 0;
    virtual IZoneObjectPtr makeWithScore(uint64_t rngFreq, const FHScoreSettings& updatedScore)
    {
        (void) updatedScore;
        auto result = make(rngFreq);

        if (!result)
            throw std::runtime_error("Object factory failed to make an object!");
        return result;
    }

    uint64_t totalFreq() const override
    {
        return m_records.m_frequency;
    }
    size_t totalActiveRecords() const override
    {
        return m_records.m_active;
    }

    IZoneObjectPtr makeChecked(uint64_t rngFreq, Core::MapScore& currentScore, const Core::MapScore& targetScore) override // return null on fail
    {
        auto isScoreOverflow = [&targetScore](const Core::MapScore& current) {
            for (const auto& [key, val] : current) {
                if (!targetScore.contains(key))
                    return true;
                const auto targetVal = targetScore.at(key);
                if (val > targetVal)
                    return true;
            }
            return false;
        };
        /*
	  target = 20000
	  current = 14000
	  remain = 6000
	  max = 7000 -> now max is 6000
*/

        FHScoreSettings scoreSettings = m_scoreSettings; // targetScore can contain MORE than m_scoreSettings, be careful.
        for (const auto& [key, val] : currentScore) {
            if (!scoreSettings.m_score.contains(key))
                continue;
            auto&      scoreTarget = scoreSettings.m_score[key];
            const auto targetVal   = targetScore.at(key);
            const auto remain      = targetVal - val;
            if (remain <= 0) {
                scoreSettings.m_score.erase(key);
                continue;
            }
            scoreTarget.m_maxSingle = std::min(scoreTarget.m_maxSingle, remain);
        }

        auto obj = this->makeWithScore(rngFreq, scoreSettings);
        if (!obj)
            return nullptr;

        if (obj->getScore().empty())
            throw std::runtime_error("Object '" + obj->getId() + "' has no score!");

        Core::MapScore currentScoreTmp = currentScore + obj->getScore();
        if (isScoreOverflow(currentScoreTmp)) {
            //std::cout << "overflow '" << obj->getId() << "' score=" << obj->getScore() << ", current=" << currentScore << "\n";
            obj->setAccepted(false);
            return nullptr;
        }
        currentScore = currentScoreTmp;

        //std::cout << "add '" << obj->getId() << "' score=" << obj->getScore() << " guard=" << obj->getGuard() << "\n";
        obj->setAccepted(true);
        return obj;
    }

    CommonRecordList<Record> m_records;

    FHMap&                        m_map;
    const FHScoreSettings         m_scoreSettings;
    const std::string             m_scoreId;
    const Core::IGameDatabase*    m_database;
    Core::IRandomGenerator* const m_rng;
};

}
