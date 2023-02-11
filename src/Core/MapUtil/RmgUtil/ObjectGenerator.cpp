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

#include "ObjectGenerator.hpp"

#include <iostream>
#include <functional>

namespace FreeHeroes {

namespace {

FHScore estimateReward(const Core::Reward& reward)
{
    FHScore score;
    for (const auto& [id, count] : reward.resources.data) {
        const int amount = count / id->pileSize;
        const int value  = amount * id->value;
        auto      attr   = (id->rarity == Core::LibraryResource::Rarity::Gold) ? FHScoreAttr::Gold : FHScoreAttr::Resource;
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
        score[FHScoreAttr::Army] = armyValue;

    if (reward.gainedExp)
        score[FHScoreAttr::Experience] = reward.gainedExp * 5 / 4;

    return score;
}

void estimateArtScore(Core::LibraryArtifactConstPtr art, FHScore& score)
{
    auto attr = FHScoreAttr::ArtSupport;
    if (std::find(art->tags.cbegin(), art->tags.cend(), Core::LibraryArtifact::Tag::Stats) != art->tags.cend())
        attr = FHScoreAttr::ArtStat;
    if (std::find(art->tags.cbegin(), art->tags.cend(), Core::LibraryArtifact::Tag::Control) != art->tags.cend())
        attr = FHScoreAttr::Control;

    score[attr] = art->value;
}

void estimateSpellScore(Core::LibrarySpellConstPtr spell, FHScore& score)
{
    auto attr = FHScoreAttr::SpellCommon;
    if (std::find(spell->tags.cbegin(), spell->tags.cend(), Core::LibrarySpell::Tag::Control) != spell->tags.cend())
        attr = FHScoreAttr::Control;
    if (spell->type == Core::LibrarySpell::Type::Offensive || spell->type == Core::LibrarySpell::Type::Summon)
        attr = FHScoreAttr::SpellOffensive;

    score[attr] = spell->value;
}

}

template<class Child>
struct CommonRecord {
    bool     m_enabled   = true;
    uint64_t m_frequency = 0;
    uint64_t m_attempts  = 1;

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
            if (rec.m_enabled) {
                m_index[m_frequency] = i;
                m_frequency += rec.m_frequency;
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
};

class ArtifactPool {
public:
    using ArtifactSet = std::set<Core::LibraryArtifactConstPtr>;
    using ArtList     = std::vector<Core::LibraryArtifactConstPtr>;

    static bool okFilter(Core::LibraryArtifactConstPtr art, bool enableFilter, const FHScoreSettings& scoreSettings)
    {
        if (!enableFilter)
            return true;

        FHScore score;
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

    static bool okFilter(Core::LibrarySpellConstPtr spell, bool enableFilter, const FHScoreSettings& scoreSettings)
    {
        if (!enableFilter)
            return true;

        FHScore score;
        estimateSpellScore(spell, score);

        bool isValid = scoreSettings.isValidScore(score);
        return isValid;
    }

    SpellPool(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : m_rng(rng)
    {
        for (auto* spell : database->spells()->records()) {
            if (map.m_disabledSpells.isDisabled(map.m_isWaterMap, spell))
                continue;

            m_spells.push_back(spell);
        }
    }

    Core::LibrarySpellConstPtr make(const Core::SpellFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings)
    {
        SpellList spellList = filter.filterPossible(m_spells);
        if (spellList.empty())
            return nullptr;

        SpellSet artSet(spellList.cbegin(), spellList.cend());
        m_pools[artSet].m_spellList = spellList;
        return m_pools[artSet].make(m_rng, enableFilter, scoreSettings);
    }
    bool isEmpty(const Core::SpellFilter& filter, bool enableFilter, const FHScoreSettings& scoreSettings) const
    {
        SpellList spellList = filter.filterPossible(m_spells);
        if (spellList.empty())
            return true;

        SpellList artListFiltered;
        for (auto art : spellList) {
            if (okFilter(art, enableFilter, scoreSettings))
                artListFiltered.push_back(art);
        }

        return artListFiltered.empty();
    }

private:
    struct SubPool {
        SpellList m_spellList;
        SpellList m_current;

        Core::LibrarySpellConstPtr make(Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
        {
            bool hasReset = false;
            if (m_current.empty()) {
                m_current = m_spellList;
                hasReset  = true;
            }
            auto art = makeOne(rng, enableFilter, scoreSettings);

            while (!art) {
                if (m_current.empty()) {
                    if (hasReset)
                        return nullptr;
                    m_current = m_spellList;
                    hasReset  = true;
                }
                art = makeOne(rng, enableFilter, scoreSettings);
            }

            return art;
        }

        Core::LibrarySpellConstPtr makeOne(Core::IRandomGenerator* rng, bool enableFilter, const FHScoreSettings& scoreSettings)
        {
            auto index = rng->gen(m_current.size() - 1);
            auto art   = m_current[index];
            m_current.erase(m_current.begin() + index);
            if (!okFilter(art, enableFilter, scoreSettings))
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
    void    setPos(FHPos pos) override { m_obj.m_pos = pos; }
    void    place() const override { m_map->m_objects.container<T>().push_back(m_obj); }
    FHScore getScore() const override { return m_obj.m_score; }
    void    disable() override { m_onDisable(); }
    int64_t getGuard() const override { return m_obj.m_guard; }

    T                     m_obj;
    FHMap*                m_map = nullptr;
    std::function<void()> m_onDisable;
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
};

struct ObjectGenerator::ObjectFactoryBank : public AbstractFactory<RecordBank> {
    struct ObjectBank : public AbstractObject<FHBank> {
        std::string getId() const override { return m_obj.m_id->id + " [" + std::to_string(m_obj.m_guardsVariant + 1) + "]"; }
    };

    ObjectFactoryBank(FHMap& map, const FHRngZone::GeneratorBank& genSettings, const FHScoreSettings& scoreSettings, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng, ArtifactPool* artifactPool)
        : AbstractFactory<RecordBank>(map, database, rng)
        , m_artifactPool(artifactPool)
        , m_scoreSettings(scoreSettings)
    {
        if (!genSettings.m_isEnabled)
            return;

        for (auto* bank : database->mapBanks()->records()) {
            if (m_map.m_disabledBanks.isDisabled(m_map.m_isWaterMap, bank))
                continue;

            for (int i = 0, sz = (int) bank->variants.size(); i < sz; i++) {
                RecordBank record;
                record.m_guardsVariant = i;
                record.m_id            = bank;
                record.m_frequency     = bank->variants[i].frequency;

                {
                    const Core::Reward&        reward                = record.m_id->rewards[record.m_id->variants[record.m_guardsVariant].rewardIndex];
                    bool                       artifactRewardIsValid = true;
                    const Core::ArtifactFilter firstFilter           = reward.artifacts.empty() ? Core::ArtifactFilter{} : reward.artifacts[0];
                    for (const auto& filter : reward.artifacts) {
                        if (m_artifactPool->isEmpty(filter, firstFilter == filter, scoreSettings))
                            artifactRewardIsValid = false;
                    }
                    if (!artifactRewardIsValid) {
                        std::cerr << "---- art - skipping " << bank->id << " [" << i << "]\n";
                        continue;
                    }

                    {
                        const FHScore score = estimateReward(reward);

                        if (!scoreSettings.isValidScore(score)) {
                            std::cerr << "---- res/army - skipping " << bank->id << " [" << i << "]\n";
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
        FHScore             score  = estimateReward(reward);

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
        obj.m_obj.m_guard = obj.m_obj.m_id->guardValue;

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
            const auto attr = value.m_resource->rarity == Core::LibraryResource::Rarity::Gold ? FHScoreAttr::Gold : FHScoreAttr::Resource;
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
};

struct RecordPandora : public CommonRecord<RecordPandora> {
    ObjectPandora m_obj;
};

struct ObjectGenerator::ObjectFactoryPandora : public AbstractFactory<RecordPandora> {
    ObjectFactoryPandora(FHMap& map, const FHRngZone::GeneratorPandora& genSettings, const FHScoreSettings& scoreSettings, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : AbstractFactory<RecordPandora>(map, database, rng)
    {
        if (!genSettings.m_isEnabled)
            return;

        for (const auto& [id, value] : genSettings.m_records) {
            ObjectPandora obj;
            obj.m_obj.m_generationId = id;
            obj.m_obj.m_guard        = value.m_guard;
            obj.m_obj.m_reward       = value.m_reward;
            obj.m_obj.m_score        = estimateReward(value.m_reward);
            auto maxValue            = maxScoreValue(obj.m_obj.m_score);
            if (!maxValue)
                throw std::runtime_error("Pandora '" + id + "' has no valid reward!");

            if (obj.m_obj.m_guard == -1)
                obj.m_obj.m_guard = maxValue * 2;

            if (scoreSettings.isValidScore(obj.m_obj.m_score))
                m_records.m_records.push_back(RecordPandora{ .m_obj = std::move(obj) }.setFreq(value.m_frequency));
            else
                std::cerr << " --- skip pandora " << id << " = " << obj.m_obj.m_score << "\n";
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
        obj.m_map = &m_map;

        return std::make_shared<ObjectPandora>(std::move(obj));
    }
};

// ---------------------------------------------------------------------------------------

struct RecordSpellShrine : public CommonRecord<RecordSpellShrine> {
    Core::SpellFilter                 m_filter;
    Core::LibraryMapVisitableConstPtr m_visitableId = nullptr;
    int                               m_guard       = -1;
};

struct ObjectGenerator::ObjectFactoryShrine : public AbstractFactory<RecordSpellShrine> {
    struct ObjectShrine : public AbstractObject<FHShrine> {
        std::string getId() const override { return "shrine " + this->m_obj.m_spellId->id; }
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
            if (m_spellPool->isEmpty(value.m_filter, true, scoreSettings))
                continue;

            auto rec = RecordSpellShrine{ .m_filter = value.m_filter, .m_visitableId = visitables[value.m_visualLevel], .m_guard = value.m_guard };

            m_records.m_records.push_back(rec.setFreq(value.m_frequency));
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
        obj.m_obj.m_spellId     = m_spellPool->make(record.m_filter, true, m_scoreSettings);

        assert(obj.m_obj.m_spellId);

        estimateSpellScore(obj.m_obj.m_spellId, obj.m_obj.m_score);

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
    int               m_guard = -1;
};

struct ObjectGenerator::ObjectFactoryScroll : public AbstractFactory<RecordSpellScroll> {
    struct ObjectScroll : public AbstractObjectWithId<FHArtifact> {
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
            if (m_spellPool->isEmpty(value.m_filter, true, scoreSettings))
                continue;

            auto rec = RecordSpellScroll{ .m_filter = value.m_filter, .m_guard = value.m_guard };

            m_records.m_records.push_back(rec.setFreq(value.m_frequency));
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
        auto spellId = m_spellPool->make(record.m_filter, true, m_scoreSettings);
        assert(spellId);
        obj.m_obj.m_id = m_scrollMapping.at(spellId);
        assert(obj.m_obj.m_id);

        estimateSpellScore(spellId, obj.m_obj.m_score);

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
    };

    ObjectFactoryDwelling(FHMap&                              map,
                          const FHRngZone::GeneratorDwelling& genSettings,
                          const FHScoreSettings&              scoreSettings,
                          const Core::IGameDatabase*          database,
                          Core::IRandomGenerator* const       rng,
                          Core::LibraryFactionConstPtr        mainFaction)
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
            if (f != mainFaction)
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
                int scoreValue = 0;
                for (auto* unit : dwell->creatureIds) {
                    const int v          = unit->value;
                    const int baseGrowth = unit->growth;
                    const int growth     = baseGrowth * value.m_weeks + value.m_castles;
                    scoreValue += growth * v;
                }

                auto rec = RecordDwelling{ .m_id = dwell, .m_value = scoreValue, .m_guard = value.m_guard };
                if (rec.m_guard == -1) {
                    rec.m_guard = scoreValue * 2;
                }
                rec.m_frequency = value.m_frequency;
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

        obj.m_obj.m_score[FHScoreAttr::Army] = record.m_value;

        obj.m_obj.m_guard = record.m_guard;

        return std::make_shared<ObjectDwelling>(std::move(obj));
    }

    Core::LibraryPlayerConstPtr m_none;
};

// ---------------------------------------------------------------------------------------

void ObjectGenerator::generate(const FHRngZone&             zoneSettings,
                               Core::LibraryFactionConstPtr mainFaction,
                               Core::LibraryFactionConstPtr rewardsFaction,
                               Core::LibraryTerrainConstPtr terrain)
{
    static const std::string indentBase("      ");

    auto tryGen = [this](const FHScore& targetScore, FHScore& currentScore, std::vector<IObjectFactoryPtr>& objectFactories) -> bool {
        static const std::string indent("        ");
        uint64_t                 totalWeight = 0;
        for (IObjectFactoryPtr& fac : objectFactories) {
            totalWeight += fac->totalFreq();
        }
        if (!totalWeight)
            return false;

        const uint64_t rngFreq = m_rng->gen(totalWeight - 1);

        auto isScoreOverflow = [&targetScore](const FHScore& current) {
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

                FHScore currentScoreTmp = currentScore + obj->getScore();
                if (isScoreOverflow(currentScoreTmp)) {
                    obj->disable();
                    // m_logOutput << "try disable '" << obj->getId() << "'\n";
                    return true;
                }
                currentScore = currentScoreTmp;

                m_logOutput << indent << "add '" << obj->getId() << "' score=" << obj->getScore() << " guard=" << obj->getGuard() << "; current factory freq=" << fac->totalFreq() << ", active=" << fac->totalActiveRecords() << "\n";
                //m_logOutput << "Updated score=" << m_currentScore << "\n";

                m_objects.push_back(obj);
                return true;
            }
            baseWeight = indexWeight;
        }

        assert(false);
        return false;
    };

    ArtifactPool artifactPool(m_map, m_database, m_rng);
    SpellPool    spellPool(m_map, m_database, m_rng);

    for (const auto& [scoreId, scoreSettings] : zoneSettings.m_scoreTargets) {
        if (!scoreSettings.m_isEnabled) {
            m_logOutput << indentBase << scoreId << " is disabled;\n";
            continue;
        }
        FHScore currentScore;
        FHScore targetScore;

        for (const auto& [key, val] : scoreSettings.m_score)
            targetScore[key] = val.m_target;

        std::vector<IObjectFactoryPtr> objectFactories;
        objectFactories.push_back(std::make_shared<ObjectFactoryBank>(m_map, zoneSettings.m_generators.m_banks, scoreSettings, m_database, m_rng, &artifactPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryArtifact>(m_map, zoneSettings.m_generators.m_artifacts, scoreSettings, m_database, m_rng, &artifactPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryResourcePile>(m_map, zoneSettings.m_generators.m_resources, scoreSettings, m_database, m_rng));
        objectFactories.push_back(std::make_shared<ObjectFactoryPandora>(m_map, zoneSettings.m_generators.m_pandoras, scoreSettings, m_database, m_rng));
        objectFactories.push_back(std::make_shared<ObjectFactoryShrine>(m_map, zoneSettings.m_generators.m_shrines, scoreSettings, m_database, m_rng, &spellPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryScroll>(m_map, zoneSettings.m_generators.m_scrolls, scoreSettings, m_database, m_rng, &spellPool));
        objectFactories.push_back(std::make_shared<ObjectFactoryDwelling>(m_map, zoneSettings.m_generators.m_dwellings, scoreSettings, m_database, m_rng, mainFaction));

        for (int i = 0; i < 1000000; i++) {
            if (!tryGen(targetScore, currentScore, objectFactories)) {
                m_logOutput << indentBase << scoreId << " finished on [" << i << "] iteration\n";
                break;
            }
        }

        m_logOutput << indentBase << scoreId << " target score:" << targetScore << "\n";
        m_logOutput << indentBase << scoreId << " end score:" << currentScore << "\n";
        m_logOutput << indentBase << scoreId << " deficit score:" << (targetScore - currentScore) << "\n";
    }
}

}
