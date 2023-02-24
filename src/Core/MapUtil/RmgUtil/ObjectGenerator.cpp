/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "LibraryMapBank.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryMapVisitable.hpp"

#include "ObjectGenerator.hpp"

#include "../FHMap.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <iostream>
#include <functional>

namespace FreeHeroes {

namespace {

Core::MapScore estimateReward(const Core::Reward& reward)
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
        score[Core::ScoreAttr::Army] = armyValue;

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

class ArtifactPool {
public:
    using ArtifactSet = std::set<Core::LibraryArtifactConstPtr>;
    using ArtList     = std::vector<Core::LibraryArtifactConstPtr>;

    static bool okFilter(Core::LibraryArtifactConstPtr art, bool enableFilter, const FHScoreSettings& scoreSettings)
    {
        if (!enableFilter)
            return true;

        Core::MapScore score;
        estimateArtScore(art, score);

        bool isValid = scoreSettings.isValidScore(score);
        return isValid;
    }

    ArtifactPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : m_rng(rng)
    {
        for (auto* art : database->artifacts()->records()) {
            if (map.m_disabledArtifacts.isDisabled(map.m_isWaterMap, art))
                continue;

            m_artifacts.push_back(art);
        }
    }

    Core::LibraryArtifactConstPtr make(const Core::ArtifactFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings)
    {
        ArtList artList = filter.filterPossible(m_artifacts);
        if (artList.empty())
            return nullptr;

        ArtifactSet artSet(artList.cbegin(), artList.cend());
        m_pools[artSet].m_artList = artList;
        return m_pools[artSet].make(m_rng, enableFilter, scoreSettings);
    }
    bool isEmpty(const Core::ArtifactFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings) const
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

private:
    struct SubPool {
        ArtList m_artList;
        ArtList m_current;

        Core::LibraryArtifactConstPtr make(Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
        {
            bool hasReset = false;
            if (m_current.empty()) {
                m_current = m_artList;
                hasReset  = true;
            }
            auto art = makeOne(rng, enableFilter, scoreSettings);

            while (!art) {
                if (m_current.empty()) {
                    if (hasReset)
                        return nullptr;
                    m_current = m_artList;
                    hasReset  = true;
                }
                art = makeOne(rng, enableFilter, scoreSettings);
            }

            return art;
        }

        Core::LibraryArtifactConstPtr makeOne(Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
        {
            auto index = rng->gen(m_current.size() - 1);
            auto art   = m_current[index];
            m_current.erase(m_current.begin() + index);
            if (!okFilter(art, enableFilter, scoreSettings))
                return nullptr;

            return art;
        }
    };
    std::map<ArtifactSet, SubPool> m_pools;
    Core::IRandomGenerator* const  m_rng;

    ArtList m_artifacts;
};

class SpellPool {
public:
    using SpellSet  = std::set<Core::LibrarySpellConstPtr>;
    using SpellList = std::vector<Core::LibrarySpellConstPtr>;

    static bool okFilter(Core::LibrarySpellConstPtr spell, bool asAnySpell, const FHScoreSettings& scoreSettings)
    {
        Core::MapScore score;
        estimateSpellScore(spell, score, asAnySpell);

        bool isValid = scoreSettings.isValidScore(score);
        return isValid;
    }

    SpellPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
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

    Core::LibrarySpellConstPtr make(const Core::SpellFilter& filter, bool asAnySpell, const FHScoreSettings& scoreSettings)
    {
        SpellList spellList = filter.filterPossible(m_spells);
        if (spellList.empty())
            return nullptr;

        SpellSet artSet(spellList.cbegin(), spellList.cend());
        m_pools[artSet].m_spellList = spellList;
        return m_pools[artSet].make(m_rng, asAnySpell, scoreSettings);
    }
    bool isEmpty(const Core::SpellFilter& filter, bool asAnySpell, const FHScoreSettings& scoreSettings) const
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

private:
    struct SubPool {
        SpellList m_spellList;
        SpellList m_current;

        Core::LibrarySpellConstPtr make(Core::IRandomGenerator* rng, bool asAnySpell, const FHScoreSettings& scoreSettings)
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

        Core::LibrarySpellConstPtr makeOne(Core::IRandomGenerator* rng, bool asAnySpell, const FHScoreSettings& scoreSettings)
        {
            auto index = rng->gen(m_current.size() - 1);
            auto art   = m_current[index];
            m_current.erase(m_current.begin() + index);
            if (!okFilter(art, asAnySpell, scoreSettings))
                return nullptr;

            return art;
        }
    };
    std::map<SpellSet, SubPool>   m_pools;
    Core::IRandomGenerator* const m_rng;

    SpellList m_spells;
};

template<class T>
struct ObjectGenerator::AbstractObject : public IObject {
    void           setPos(FHPos pos) override { m_obj.m_pos = pos; }
    void           place() const override { m_map->m_objects.container<T>().push_back(m_obj); }
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
    AbstractFactory(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : m_rng(rng)
        , m_database(database)
        , m_map(map)
    {}

    uint64_t totalFreq() const override
    {
        return m_records.m_frequency;
    }
    size_t totalActiveRecords() const override
    {
        return m_records.m_active;
    }

    CommonRecordList<Record> m_records;

    Core::IRandomGenerator* const m_rng;
    const Core::IGameDatabase*    m_database;
    FHMap&                        m_map;
};

// ---------------------------------------------------------------------------------------

struct RecordBank : public CommonRecord<RecordBank> {
    Core::LibraryMapBankConstPtr m_id            = nullptr;
    int                          m_guardsVariant = 0;
    int                          m_guardValue    = 0;
};

struct ObjectGenerator::ObjectFactoryBank : public AbstractFactory<RecordBank> {
    struct ObjectBank : public AbstractObject<FHBank> {
        std::string                    getId() const override { return m_obj.m_id->id + " [" + std::to_string(m_obj.m_guardsVariant + 1) + "]"; }
        Type                           getType() const override { return Type::Visitable; }
        Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_id->objectDefs.get({}); }
    };

    ObjectFactoryBank(FHMap&                          map,
                      const FHRngZone::GeneratorBank& genSettings,
                      const FHScoreSettings&          scoreSettings,
                      const Core::IGameDatabase*      database,
                      Core::IRandomGenerator* const   rng,
                      ArtifactPool*                   artifactPool,
                      Core::LibraryTerrainConstPtr    terrain)
        : AbstractFactory<RecordBank>(map, database, rng)
        , m_artifactPool(artifactPool)
        , m_scoreSettings(scoreSettings)
    {
        if (!genSettings.m_isEnabled)
            return;

        std::map<Core::LibraryMapBankConstPtr, const FHRngZone::GeneratorBank::Record*> recordMap;
        for (auto&& [_, rec] : genSettings.m_records)
            recordMap[rec.m_bank] = &rec;

        for (auto* bank : database->mapBanks()->records()) {
            if (m_map.m_disabledBanks.isDisabled(m_map.m_isWaterMap, bank))
                continue;

            if (!ObjectGenerator::terrainViable(bank->objectDefs, terrain))
                continue;

            int totalFreq = 0;
            for (auto& var : bank->variants)
                totalFreq += var.frequencyRel;

            int baseFrequency = bank->frequency;
            int guardValue    = bank->guardValue;

            {
                auto it = recordMap.find(bank);
                if (it != recordMap.cend()) {
                    const FHRngZone::GeneratorBank::Record& rec = *(it->second);
                    if (rec.m_frequency != -1)
                        baseFrequency = rec.m_frequency;
                    if (rec.m_guard != -1)
                        guardValue = rec.m_guard;
                    if (!rec.m_enabled)
                        continue;
                }
            }

            for (int i = 0, sz = (int) bank->variants.size(); i < sz; i++) {
                RecordBank record;
                record.m_guardsVariant = i;
                record.m_id            = bank;
                record.m_guardValue    = guardValue;
                record.m_frequency     = baseFrequency * bank->variants[i].frequencyRel / totalFreq;

                {
                    const Core::Reward&        reward                = record.m_id->rewards[record.m_id->variants[record.m_guardsVariant].rewardIndex];
                    bool                       artifactRewardIsValid = true;
                    const Core::ArtifactFilter firstFilter           = reward.artifacts.empty() ? Core::ArtifactFilter{} : reward.artifacts[0];
                    for (const auto& filter : reward.artifacts) {
                        if (m_artifactPool->isEmpty(filter, firstFilter == filter, scoreSettings))
                            artifactRewardIsValid = false;
                    }
                    if (!artifactRewardIsValid) {
                        continue;
                    }

                    {
                        const Core::MapScore score = estimateReward(reward);

                        if (!scoreSettings.isValidScore(score)) {
                            continue;
                        }
                    }

                    const size_t artRewards = reward.artifacts.size();
                    if (artRewards == 0)
                        record.m_attempts = 1;
                    else if (artRewards == 1)
                        record.m_attempts = 3;
                    else
                        record.m_attempts = 5;
                }

                m_records.m_records.push_back(record);
            }
        }
        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        const bool upgraded = record.m_id->upgradedStackIndex == -1 ? false : m_rng->genSmall(3) == 0;
        ObjectBank obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_obj.m_id            = record.m_id;
        obj.m_obj.m_guardsVariant = record.m_guardsVariant;
        obj.m_obj.m_upgradedStack = upgraded ? FHBank::UpgradedStack::Yes : FHBank::UpgradedStack::No;
        obj.m_map                 = &m_map;

        const Core::Reward& reward = record.m_id->rewards[record.m_id->variants[record.m_guardsVariant].rewardIndex];
        Core::MapScore      score  = estimateReward(reward);

        {
            const Core::ArtifactFilter firstFilter = reward.artifacts.empty() ? Core::ArtifactFilter{} : reward.artifacts[0];
            for (const auto& filter : reward.artifacts) {
                auto art = m_artifactPool->make(filter, firstFilter == filter, m_scoreSettings);
                assert(art);
                obj.m_obj.m_artifacts.push_back(art);

                estimateArtScore(art, score);
            }
        }
        obj.m_obj.m_score = score;
        obj.m_obj.m_guard = record.m_guardValue;

        return std::make_shared<ObjectBank>(std::move(obj));
    }

    ArtifactPool* const   m_artifactPool;
    const FHScoreSettings m_scoreSettings;
};

// ---------------------------------------------------------------------------------------

struct RecordArtifact : public CommonRecord<RecordArtifact> {
    Core::ArtifactFilter m_filter;
};

struct ObjectGenerator::ObjectFactoryArtifact : public AbstractFactory<RecordArtifact> {
    struct ObjectArtifact : public AbstractObjectWithId<FHArtifact> {
        Type getType() const override { return Type::Pickable; }
        bool preventDuplicates() const override { return true; }
    };

    ObjectFactoryArtifact(FHMap& map, const FHRngZone::GeneratorArtifact& genSettings, const FHScoreSettings& scoreSettings, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng, ArtifactPool* artifactPool)
        : AbstractFactory<RecordArtifact>(map, database, rng)
        , m_artifactPool(artifactPool)
        , m_scoreSettings(scoreSettings)
    {
        if (!genSettings.m_isEnabled)
            return;

        for (const auto& [_, value] : genSettings.m_records) {
            if (m_artifactPool->isEmpty(value.m_filter, true, scoreSettings))
                continue;

            auto rec = RecordArtifact{ .m_filter = value.m_filter };

            rec.m_attempts = 3;

            m_records.m_records.push_back(rec.setFreq(value.m_frequency));
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectArtifact obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_map = &m_map;

        obj.m_obj.m_id = m_artifactPool->make(record.m_filter, true, m_scoreSettings);
        assert(obj.m_obj.m_id);

        estimateArtScore(obj.m_obj.m_id, obj.m_obj.m_score);

        obj.m_obj.m_guard = obj.m_obj.m_id->guard;

        return std::make_shared<ObjectArtifact>(std::move(obj));
    }

    ArtifactPool* const   m_artifactPool;
    const FHScoreSettings m_scoreSettings;
};

// ---------------------------------------------------------------------------------------

struct ObjectResourcePile : public ObjectGenerator::AbstractObjectWithId<FHResource> {
    Type getType() const override { return Type::Pickable; }
};

struct RecordResourcePile : public CommonRecord<RecordResourcePile> {
    ObjectResourcePile m_obj;
};

struct ObjectGenerator::ObjectFactoryResourcePile : public AbstractFactory<RecordResourcePile> {
    ObjectFactoryResourcePile(FHMap& map, const FHRngZone::GeneratorResourcePile& genSettings, const FHScoreSettings& scoreSettings, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : AbstractFactory<RecordResourcePile>(map, database, rng)
    {
        if (!genSettings.m_isEnabled)
            return;

        for (const auto& [_, value] : genSettings.m_records) {
            const auto attr = value.m_resource->rarity == Core::LibraryResource::Rarity::Gold ? Core::ScoreAttr::Gold : Core::ScoreAttr::Resource;
            for (int amount : value.m_amounts) {
                ObjectResourcePile obj;
                obj.m_obj.m_id          = value.m_resource;
                obj.m_obj.m_amount      = amount;
                obj.m_obj.m_guard       = value.m_guard;
                obj.m_obj.m_score[attr] = value.m_resource->value * (amount / value.m_resource->pileSize);
                if (scoreSettings.isValidValue(attr, obj.m_obj.m_score[attr]))
                    m_records.m_records.push_back(RecordResourcePile{ .m_obj = std::move(obj) }.setFreq(value.m_frequency));
            }
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectResourcePile obj = record.m_obj;
        obj.m_onDisable        = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_map = &m_map;

        return std::make_shared<ObjectResourcePile>(std::move(obj));
    }
};

// ---------------------------------------------------------------------------------------

struct ObjectPandora : public ObjectGenerator::AbstractObject<FHPandora> {
    std::string getId() const override { return m_obj.m_generationId; }
    Type        getType() const override { return Type::Pickable; }
    bool        preventDuplicates() const override { return true; }
};

struct RecordPandora : public CommonRecord<RecordPandora> {
    ObjectPandora m_obj;
    struct RandomUnit {
        std::vector<Core::LibraryUnitConstPtr> m_options;
        int                                    m_value = 0;
    };
    using RandomUnitList = std::vector<RandomUnit>;

    RandomUnitList m_unitRewards;
};

struct ObjectGenerator::ObjectFactoryPandora : public AbstractFactory<RecordPandora> {
    ObjectFactoryPandora(FHMap&                             map,
                         const FHRngZone::GeneratorPandora& genSettings,
                         const FHScoreSettings&             scoreSettings,
                         const Core::IGameDatabase*         database,
                         Core::IRandomGenerator* const      rng,
                         Core::LibraryFactionConstPtr       rewardsFaction)
        : AbstractFactory<RecordPandora>(map, database, rng)
        , m_rng(rng)
    {
        if (!genSettings.m_isEnabled)
            return;

        std::vector<Core::LibrarySpellConstPtr>  allSpells;
        std::map<int, Core::LibraryUnitConstPtr> factionUnits;

        for (auto* unit : rewardsFaction->units)
            factionUnits[unit->level] = unit;

        for (auto* spell : database->spells()->records()) {
            if (map.m_disabledSpells.isDisabled(map.m_isWaterMap, spell))
                continue;
            if (!spell->isTeachable)
                continue;

            allSpells.push_back(spell);
        }

        for (const auto& [id, value] : genSettings.m_records) {
            ObjectPandora obj;
            obj.m_obj.m_generationId = id;
            obj.m_obj.m_guard        = value.m_guard;
            obj.m_obj.m_reward       = value.m_reward;
            obj.m_obj.m_score        = estimateReward(value.m_reward);
            if (!obj.m_obj.m_reward.spells.isDefault()) {
                auto filteredSpells = obj.m_obj.m_reward.spells.filterPossible(allSpells);
                if (filteredSpells.empty())
                    throw std::runtime_error("Pandora '" + id + "' has invalid spell filter!");

                obj.m_obj.m_reward.spells            = {};
                obj.m_obj.m_reward.spells.onlySpells = filteredSpells;

                estimateSpellListScore(filteredSpells, obj.m_obj.m_score, false); // @todo: SpellAny variant for magic pandoras.
            }
            auto maxValue = maxScoreValue(obj.m_obj.m_score);
            if (!maxValue)
                throw std::runtime_error("Pandora '" + id + "' has no valid reward!");

            if (obj.m_obj.m_guard == -1)
                obj.m_obj.m_guard = maxValue * 2;

            bool                          validRandomUnits = true;
            RecordPandora::RandomUnitList unitRewards;
            for (auto&& randomUnit : obj.m_obj.m_reward.randomUnits) {
                std::vector<Core::LibraryUnitConstPtr> options;
                for (int level : randomUnit.m_levels) {
                    if (factionUnits.contains(level)) {
                        options.push_back(factionUnits[level]);
                    }
                }
                if (options.empty()) {
                    validRandomUnits = false;
                }
                unitRewards.push_back(RecordPandora::RandomUnit{ .m_options = std::move(options), .m_value = randomUnit.m_value });
            }
            if (!validRandomUnits)
                continue;

            obj.m_obj.m_reward.randomUnits.clear();

            if (scoreSettings.isValidScore(obj.m_obj.m_score))
                m_records.m_records.push_back(RecordPandora{ .m_obj = std::move(obj), .m_unitRewards = std::move(unitRewards) }.setFreq(value.m_frequency));
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectPandora obj = record.m_obj;
        obj.m_onDisable   = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_map      = &m_map;
        bool idChanged = false;
        for (const auto& unitOptions : record.m_unitRewards) {
            auto  rindex = m_rng->gen(unitOptions.m_options.size() - 1);
            auto* unit   = unitOptions.m_options[rindex];
            int   count  = unitOptions.m_value / unit->value;
            count        = std::max(count, 1);
            if (!idChanged) {
                idChanged          = true;
                auto        up     = unit->level % 10;
                std::string suffix = "";
                if (up == 1)
                    suffix = "u";
                else if (up > 1)
                    suffix = "uu";
                suffix += "-" + std::to_string(count);
                obj.m_obj.m_generationId += suffix;
            }
            obj.m_obj.m_reward.units.push_back({ unit, count });
        }

        return std::make_shared<ObjectPandora>(std::move(obj));
    }

    Core::IRandomGenerator* const m_rng;
};

// ---------------------------------------------------------------------------------------

struct RecordSpellShrine : public CommonRecord<RecordSpellShrine> {
    Core::SpellFilter                 m_filter;
    Core::LibraryMapVisitableConstPtr m_visitableId = nullptr;
    int                               m_guard       = -1;
    bool                              m_asAnySpell  = false;
};

struct ObjectGenerator::ObjectFactoryShrine : public AbstractFactory<RecordSpellShrine> {
    struct ObjectShrine : public AbstractObject<FHShrine> {
        std::string getId() const override { return "shrine " + this->m_obj.m_spellId->id; }

        Type                           getType() const override { return Type::Visitable; }
        Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_visitableId->objectDefs.get({}); }
    };

    ObjectFactoryShrine(FHMap& map, const FHRngZone::GeneratorShrine& genSettings, const FHScoreSettings& scoreSettings, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng, SpellPool* spellPool)
        : AbstractFactory<RecordSpellShrine>(map, database, rng)
        , m_spellPool(spellPool)
        , m_scoreSettings(scoreSettings)
    {
        if (!genSettings.m_isEnabled)
            return;

        std::map<int, Core::LibraryMapVisitableConstPtr> visitables;
        for (int level = 0; const char* id : { "sod.visitable.shrine1", "sod.visitable.shrine2", "sod.visitable.shrine3" }) {
            level++;
            auto* visitableId = database->mapVisitables()->find(id);
            assert(visitableId);
            visitables[level] = visitableId;
        }

        {
            auto* visitableId = database->mapVisitables()->find("hota.visitable.shrine4");
            if (!visitableId)
                visitableId = visitables[3];
            visitables[4] = visitableId;
        }

        for (const auto& [_, value] : genSettings.m_records) {
            for (bool asAnySpell : { false, true }) {
                if (m_spellPool->isEmpty(value.m_filter, asAnySpell, scoreSettings)) {
                    continue;
                }

                auto rec = RecordSpellShrine{
                    .m_filter      = value.m_filter,
                    .m_visitableId = visitables[value.m_visualLevel],
                    .m_guard       = value.m_guard,
                    .m_asAnySpell  = asAnySpell,
                };

                m_records.m_records.push_back(rec.setFreq(value.m_frequency));
            }
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectShrine obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_map               = &m_map;
        obj.m_obj.m_visitableId = record.m_visitableId;
        obj.m_obj.m_spellId     = m_spellPool->make(record.m_filter, record.m_asAnySpell, m_scoreSettings);

        assert(obj.m_obj.m_spellId);

        estimateSpellScore(obj.m_obj.m_spellId, obj.m_obj.m_score, record.m_asAnySpell);

        obj.m_obj.m_guard = record.m_guard;
        if (obj.m_obj.m_guard == -1)
            obj.m_obj.m_guard = obj.m_obj.m_spellId->value * 2 * 3 / 4; // shrine = 75% of spell value

        return std::make_shared<ObjectShrine>(std::move(obj));
    }

    SpellPool* const      m_spellPool;
    const FHScoreSettings m_scoreSettings;
};

// ---------------------------------------------------------------------------------------

struct RecordSpellScroll : public CommonRecord<RecordSpellScroll> {
    Core::SpellFilter m_filter;
    int               m_guard      = -1;
    bool              m_asAnySpell = false;
};

struct ObjectGenerator::ObjectFactoryScroll : public AbstractFactory<RecordSpellScroll> {
    struct ObjectScroll : public AbstractObjectWithId<FHArtifact> {
        Type getType() const override { return Type::Pickable; }
        bool preventDuplicates() const override { return true; }
    };

    ObjectFactoryScroll(FHMap& map, const FHRngZone::GeneratorScroll& genSettings, const FHScoreSettings& scoreSettings, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng, SpellPool* spellPool)
        : AbstractFactory<RecordSpellScroll>(map, database, rng)
        , m_spellPool(spellPool)
        , m_scoreSettings(scoreSettings)
    {
        if (!genSettings.m_isEnabled)
            return;

        for (auto* art : database->artifacts()->records()) {
            if (!art->scrollSpell)
                continue;
            m_scrollMapping[art->scrollSpell] = art;
        }

        for (const auto& [_, value] : genSettings.m_records) {
            for (bool asAnySpell : { false, true }) {
                if (m_spellPool->isEmpty(value.m_filter, asAnySpell, scoreSettings)) {
                    continue;
                }

                auto rec = RecordSpellScroll{ .m_filter = value.m_filter, .m_guard = value.m_guard, .m_asAnySpell = asAnySpell };

                m_records.m_records.push_back(rec.setFreq(value.m_frequency));
            }
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectScroll obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_map    = &m_map;
        auto spellId = m_spellPool->make(record.m_filter, record.m_asAnySpell, m_scoreSettings);
        assert(spellId);
        obj.m_obj.m_id = m_scrollMapping.at(spellId);
        assert(obj.m_obj.m_id);

        estimateSpellScore(spellId, obj.m_obj.m_score, record.m_asAnySpell);

        obj.m_obj.m_guard = record.m_guard;
        if (obj.m_obj.m_guard == -1)
            obj.m_obj.m_guard = spellId->value * 2; // scroll = 100% of spell value

        return std::make_shared<ObjectScroll>(std::move(obj));
    }

    std::map<Core::LibrarySpellConstPtr, Core::LibraryArtifactConstPtr> m_scrollMapping;
    SpellPool* const                                                    m_spellPool;
    const FHScoreSettings                                               m_scoreSettings;
};

// ---------------------------------------------------------------------------------------

struct RecordDwelling : public CommonRecord<RecordDwelling> {
    Core::LibraryDwellingConstPtr m_id    = nullptr;
    int                           m_value = -1;
    int                           m_guard = -1;
};

struct ObjectGenerator::ObjectFactoryDwelling : public AbstractFactory<RecordDwelling> {
    struct ObjectDwelling : public AbstractObjectWithId<FHDwelling> {
        Type                           getType() const override { return Type::Visitable; }
        Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_id->objectDefs.get({}); }
    };

    ObjectFactoryDwelling(FHMap&                              map,
                          const FHRngZone::GeneratorDwelling& genSettings,
                          const FHScoreSettings&              scoreSettings,
                          const Core::IGameDatabase*          database,
                          Core::IRandomGenerator* const       rng,
                          Core::LibraryFactionConstPtr        dwellFaction)
        : AbstractFactory<RecordDwelling>(map, database, rng)
    {
        if (!genSettings.m_isEnabled)
            return;

        std::map<int, std::vector<Core::LibraryDwellingConstPtr>> dwellByLevel;
        m_none = database->players()->find(std::string(Core::LibraryPlayer::s_none));

        for (auto* dwelling : database->dwellings()->records()) {
            if (dwelling->creatureIds.empty())
                continue;
            Core::LibraryFactionConstPtr f = dwelling->creatureIds[0]->faction;
            if (f != dwellFaction)
                continue;

            int level = 0;
            for (auto* unit : dwelling->creatureIds) {
                level = std::max(level, unit->level);
            }
            dwellByLevel[level].push_back(dwelling);
        }

        for (const auto& [_, value] : genSettings.m_records) {
            if (!dwellByLevel.contains(value.m_level))
                continue;
            std::vector<Core::LibraryDwellingConstPtr> dwells = dwellByLevel[value.m_level];

            for (auto* dwell : dwells) {
                int scoreValue = value.m_value;
                if (value.m_value == -1) {
                    scoreValue = 0;
                    for (auto* unit : dwell->creatureIds) {
                        const int v          = unit->value;
                        const int baseGrowth = unit->growth;
                        scoreValue += baseGrowth * v;
                    }
                }

                auto rec = RecordDwelling{ .m_id = dwell, .m_value = scoreValue, .m_guard = value.m_guard };
                if (rec.m_guard == -1) {
                    rec.m_guard = scoreValue * 2;
                }
                rec.m_frequency = value.m_frequency;
                if (scoreSettings.isValidValue(Core::ScoreAttr::ArmyDwelling, scoreValue))
                    m_records.m_records.push_back(rec);
            }
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectDwelling obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_map          = &m_map;
        obj.m_obj.m_id     = record.m_id;
        obj.m_obj.m_player = m_none;

        obj.m_obj.m_score[Core::ScoreAttr::ArmyDwelling] = record.m_value;

        obj.m_obj.m_guard = record.m_guard;

        return std::make_shared<ObjectDwelling>(std::move(obj));
    }

    Core::LibraryPlayerConstPtr m_none;
};

// ---------------------------------------------------------------------------------------

struct RecordVisitable : public CommonRecord<RecordVisitable> {
    FHVisitable m_obj;
};

struct ObjectGenerator::ObjectFactoryVisitable : public AbstractFactory<RecordVisitable> {
    struct ObjectVisitable : public AbstractObject<FHVisitable> {
        std::string                    getId() const override { return this->m_obj.m_visitableId->id; }
        Type                           getType() const override { return m_obj.m_visitableId->visitKind == Core::LibraryMapVisitable::VisitKind::Normal ? Type::Visitable : Type::Pickable; }
        Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_visitableId->objectDefs.get({}); }
        FHPos                          getOffset() const override
        {
            if (this->m_obj.m_visitableId->id == "hota.visitable.ancientLamp") {
                // @todo: scan for visitable mask;
                return FHPos{ +1, 0 };
            }
            return FHPos{};
        }
    };

    ObjectFactoryVisitable(FHMap&                               map,
                           const FHRngZone::GeneratorVisitable& genSettings,
                           const FHScoreSettings&               scoreSettings,
                           const Core::IGameDatabase*           database,
                           Core::IRandomGenerator* const        rng,
                           Core::LibraryTerrainConstPtr         terrain)
        : AbstractFactory<RecordVisitable>(map, database, rng)
    {
        if (!genSettings.m_isEnabled)
            return;

        for (auto* visitable : database->mapVisitables()->records()) {
            if (visitable->attr == Core::ScoreAttr::Invalid)
                continue;

            if (!ObjectGenerator::terrainViable(visitable->objectDefs, terrain))
                continue;

            RecordVisitable record;
            record.m_obj.m_visitableId = visitable;
            record.m_frequency         = visitable->frequency;
            record.m_maxLimit          = visitable->maxZone;
            record.m_minLimit          = visitable->minZone;
            Core::MapScore score;
            const int      scoreValue = visitable->value;
            if (!scoreValue)
                throw std::runtime_error("'" + visitable->id + "' has no valid score!");

            Core::ScoreAttr attr = visitable->attr;

            record.m_obj.m_score[attr] = scoreValue;

            record.m_obj.m_guard = scoreValue * 2;

            if (scoreSettings.isValidValue(attr, scoreValue)) {
                m_records.m_records.push_back(record);
            }
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectVisitable obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_onAccept = [this, &record] {
            m_records.onAccept(record);
        };
        obj.m_map = &m_map;
        obj.m_obj = record.m_obj;

        return std::make_shared<ObjectVisitable>(std::move(obj));
    }
};

// ---------------------------------------------------------------------------------------

struct RecordMine : public CommonRecord<RecordMine> {
    FHMine m_obj;
};

struct ObjectGenerator::ObjectFactoryMine : public AbstractFactory<RecordMine> {
    struct ObjectMine : public AbstractObject<FHMine> {
        std::string                    getId() const override { return "mine." + this->m_obj.m_id->id; }
        Type                           getType() const override { return Type::Visitable; }
        Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_id->minesDefs.get(m_obj.m_defIndex); }
    };

    ObjectFactoryMine(FHMap&                          map,
                      const FHRngZone::GeneratorMine& genSettings,
                      const FHScoreSettings&          scoreSettings,
                      const Core::IGameDatabase*      database,
                      Core::IRandomGenerator* const   rng,
                      Core::LibraryTerrainConstPtr    terrain)
        : AbstractFactory<RecordMine>(map, database, rng)
    {
        if (!genSettings.m_isEnabled)
            return;

        auto none = database->players()->find(std::string(Core::LibraryPlayer::s_none));

        for (const auto& [_, value] : genSettings.m_records) {
            RecordMine record;
            record.m_obj.m_id    = value.m_resourceId;
            Core::ScoreAttr attr = Core::ScoreAttr::ResourceGen;

            auto scoreValue    = value.m_value;
            record.m_frequency = value.m_frequency;
            record.m_maxLimit  = value.m_maxZone;
            record.m_minLimit  = value.m_minZone;
            ObjectGenerator::correctObjIndex(record.m_obj.m_defIndex, record.m_obj.m_id->minesDefs, terrain);
            record.m_obj.m_guard       = value.m_guard;
            record.m_obj.m_player      = none;
            record.m_obj.m_score[attr] = scoreValue;
            if (scoreSettings.isValidValue(attr, scoreValue))
                m_records.m_records.push_back(record);
        }

        m_records.updateFrequency();
    }

    IObjectPtr make(uint64_t rngFreq) override
    {
        const size_t index  = m_records.getFreqIndex(rngFreq);
        auto&        record = m_records.m_records[index];

        ObjectMine obj;
        obj.m_onDisable = [this, &record] {
            m_records.onDisable(record);
        };
        obj.m_onAccept = [this, &record] {
            m_records.onAccept(record);
        };
        obj.m_map = &m_map;
        obj.m_obj = record.m_obj;

        return std::make_shared<ObjectMine>(std::move(obj));
    }
};

// ---------------------------------------------------------------------------------------

void ObjectGenerator::generate(const FHRngZone&             zoneSettings,
                               Core::LibraryFactionConstPtr rewardsFaction,
                               Core::LibraryFactionConstPtr dwellFaction,
                               Core::LibraryTerrainConstPtr terrain,
                               int64_t                      armyPercent,
                               int64_t                      goldPercent)
{
    static const std::string indentBase("      ");
    Mernel::ProfilerScope    scope("generate");

    ArtifactPool artifactPool(m_map, m_database, m_rng);
    SpellPool    spellPool(m_map, m_database, m_rng);

    for (const auto& [scoreId, scoreSettings] : zoneSettings.m_scoreTargets) {
        if (!scoreSettings.m_isEnabled) {
            m_logOutput << indentBase << scoreId << " is disabled;\n";
            continue;
        }
        Core::MapScore targetScore;

        ObjectGroup group;
        group.m_id           = scoreId;
        group.m_guardPercent = scoreSettings.m_guardPercent;

        group.m_scoreSettings = scoreSettings;
        group.scale(armyPercent, goldPercent);

        for (const auto& [key, val] : group.m_scoreSettings.m_score)
            targetScore[key] = val.m_target;

        group.m_targetScore      = targetScore;
        group.m_targetScoreTotal = totalScoreValue(targetScore);
        m_groups.push_back(group);
    }

    std::sort(m_groups.begin(), m_groups.end(), [](const ObjectGroup& l, const ObjectGroup& r) {
        return l.m_targetScoreTotal > r.m_targetScoreTotal;
    });

    const bool doLog = true;

    for (ObjectGroup& group : m_groups) {
        const auto& scoreSettings = group.m_scoreSettings;

        if (doLog)
            m_logOutput << indentBase << group.m_id << " start\n";
        Core::MapScore currentScore;

        std::vector<IObjectFactoryPtr> objectFactories;
        objectFactories.push_back(std::make_shared<ObjectFactoryBank>(m_map, zoneSettings.m_generators.m_banks, scoreSettings, m_database, m_rng, &artifactPool, terrain));
        objectFactories.push_back(std::make_shared<ObjectFactoryArtifact>(m_map, zoneSettings.m_generators.m_artifacts, scoreSettings, m_database, m_rng, &artifactPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryResourcePile>(m_map, zoneSettings.m_generators.m_resources, scoreSettings, m_database, m_rng));
        objectFactories.push_back(std::make_shared<ObjectFactoryPandora>(m_map, zoneSettings.m_generators.m_pandoras, scoreSettings, m_database, m_rng, rewardsFaction));
        objectFactories.push_back(std::make_shared<ObjectFactoryShrine>(m_map, zoneSettings.m_generators.m_shrines, scoreSettings, m_database, m_rng, &spellPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryScroll>(m_map, zoneSettings.m_generators.m_scrolls, scoreSettings, m_database, m_rng, &spellPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryDwelling>(m_map, zoneSettings.m_generators.m_dwellings, scoreSettings, m_database, m_rng, dwellFaction));
        objectFactories.push_back(std::make_shared<ObjectFactoryVisitable>(m_map, zoneSettings.m_generators.m_visitables, scoreSettings, m_database, m_rng, terrain));
        objectFactories.push_back(std::make_shared<ObjectFactoryMine>(m_map, zoneSettings.m_generators.m_mines, scoreSettings, m_database, m_rng, terrain));

        const int iterLimit = 100000;
        int       i         = 0;
        for (; i < iterLimit; i++) {
            if (!generateOneObject(group.m_targetScore, currentScore, objectFactories, group)) {
                if (doLog)
                    m_logOutput << indentBase << group.m_id << " finished on [" << i << "] iteration\n";
                break;
            }
        }
        if (i >= iterLimit - 1)
            throw std::runtime_error("Iteration limit reached.");

        auto deficitScore   = (group.m_targetScore - currentScore);
        auto targetSum      = group.m_targetScoreTotal;
        auto deficitSum     = totalScoreValue(deficitScore);
        auto allowedDeficit = targetSum * scoreSettings.m_tolerancePercent / 100;

        if (doLog) {
            m_logOutput << indentBase << group.m_id << " target score:" << group.m_targetScore << "\n";
            m_logOutput << indentBase << group.m_id << " end score:" << currentScore << "\n";
            m_logOutput << indentBase << group.m_id << " deficit score:" << deficitScore << "\n";
            m_logOutput << indentBase << group.m_id << " checking deficit tolerance: " << deficitSum << " <= " << allowedDeficit << "...\n";
        }
        if (deficitSum > allowedDeficit)
            throw std::runtime_error("Deficit score for '" + group.m_id + "' exceed tolerance!");
    }
}

bool ObjectGenerator::generateOneObject(const Core::MapScore& targetScore, Core::MapScore& currentScore, std::vector<IObjectFactoryPtr>& objectFactories, ObjectGroup& group)
{
    Mernel::ProfilerScope    scope("oneObject");
    static const std::string indent("        ");
    uint64_t                 totalWeight = 0;
    for (IObjectFactoryPtr& fac : objectFactories) {
        totalWeight += fac->totalFreq();
    }
    if (!totalWeight)
        return false;

    const uint64_t rngFreq = m_rng->gen(totalWeight - 1);

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

    uint64_t indexWeight = 0, baseWeight = 0;
    for (IObjectFactoryPtr& fac : objectFactories) {
        indexWeight += fac->totalFreq();
        if (indexWeight > rngFreq && fac->totalFreq()) {
            const uint64_t rngFreqForFactory = rngFreq - baseWeight;
            auto           obj               = fac->make(rngFreqForFactory);
            if (!obj)
                throw std::runtime_error("Object factory failed to make an object!");
            if (obj->getScore().empty())
                throw std::runtime_error("Object '" + obj->getId() + "' has no score!");

            Core::MapScore currentScoreTmp = currentScore + obj->getScore();
            if (isScoreOverflow(currentScoreTmp)) {
                obj->setAccepted(false);
                return true;
            }
            currentScore = currentScoreTmp;

            // m_logOutput << indent << "add '" << obj->getId() << "' score=" << obj->getScore() << " guard=" << obj->getGuard() << "; current factory freq=" << fac->totalFreq() << ", active=" << fac->totalActiveRecords() << "\n";
            obj->setAccepted(true);
            group.m_objects.push_back(obj);
            return true;
        }
        baseWeight = indexWeight;
    }

    assert(false);
    return false;
}

bool ObjectGenerator::correctObjIndex(Core::ObjectDefIndex& defIndex, const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain)
{
    for (const auto& [variant, defSource] : defMapping.variants) {
        if (defSource->terrainsSoftCache.contains(requiredTerrain)) {
            defIndex.variant      = variant;
            defIndex.substitution = "";
            return true;
        }

        for (const auto& [subsitutionId, def] : defSource->substitutions) {
            if (def->terrainsSoftCache.contains(requiredTerrain)) {
                defIndex.variant      = variant;
                defIndex.substitution = subsitutionId;
                return true;
            }
        }
    }
    return false;
}

bool ObjectGenerator::terrainViable(const Core::ObjectDefMappings& defMapping, Core::LibraryTerrainConstPtr requiredTerrain)
{
    Core::ObjectDefIndex index;
    return correctObjIndex(index, defMapping, requiredTerrain);
}

void ObjectGenerator::ObjectGroup::scale(int64_t armyPercent, int64_t goldPercent)
{
    auto applyPercent = [](FHScoreSettings::ScoreScope& scope, int64_t percent) {
        scope.m_target = scope.m_target * percent / 100;
        if (percent < 100 && scope.m_minSingle != -1) {
            scope.m_minSingle = scope.m_minSingle * percent / 100;
        }
    };

    if (armyPercent != 100) {
        if (m_scoreSettings.m_score.contains(Core::ScoreAttr::Army)) {
            auto& armyScore = m_scoreSettings.m_score[Core::ScoreAttr::Army];
            applyPercent(armyScore, armyPercent);
        }
    }
    if (goldPercent != 100) {
        if (m_scoreSettings.m_score.contains(Core::ScoreAttr::Gold)) {
            auto& goldScore = m_scoreSettings.m_score[Core::ScoreAttr::Gold];
            applyPercent(goldScore, goldPercent);
        }
    }
}
}
