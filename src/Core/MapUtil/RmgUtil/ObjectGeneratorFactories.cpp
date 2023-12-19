/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ObjectGeneratorFactories.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "LibraryMapBank.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryMapVisitable.hpp"

#include "../ScoreUtil.hpp"

namespace FreeHeroes {

namespace {
const Core::CombinedMask g_oneTileMask = []() {
    Core::CombinedMask result{
        .m_blocked{ { 0, 0 } },
        .m_visitable{ { 0, 0 } },
        .m_width  = 1,
        .m_height = 1,
    };
    result.m_rows.resize(1);
    result.m_rows[0].resize(1);
    result.m_rows[0][0] = Core::CombinedMask::Cell{ .m_blocked = true, .m_visitable = true };

    return result;
}();
}

// ---------------------------------------------------------------------------------------

struct ObjectGenerator::ObjectFactoryBank::ObjectBank : public AbstractObject<FHBank> {
    std::string                    getId() const override { return m_obj.m_id->id + " [" + std::to_string(m_obj.m_guardsVariant + 1) + "]"; }
    Type                           getType() const override { return Type::Visitable; }
    Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_id->objectDefs.get({}); }
    std::string                    getRepulseId() const override { return m_repulseId; }

    std::string m_repulseId;
};

ObjectGenerator::ObjectFactoryBank::ObjectFactoryBank(FHMap&                          map,
                                                      const FHRngZone::GeneratorBank& genSettings,
                                                      const FHScoreSettings&          scoreSettings,
                                                      const std::string&              scoreId,
                                                      const Core::IGameDatabase*      database,
                                                      Core::IRandomGenerator* const   rng,
                                                      ArtifactPool*                   artifactPool,
                                                      Core::LibraryTerrainConstPtr    terrain)
    : AbstractFactory<RecordBank>(map, scoreSettings, scoreId, database, rng)
    , m_artifactPool(artifactPool)
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

        int         baseFrequency = bank->frequency;
        int         guardValue    = bank->guardValue;
        std::string repulseId;

        {
            auto it = recordMap.find(bank);
            if (it != recordMap.cend()) {
                const FHRngZone::GeneratorBank::Record& rec = *(it->second);
                if (rec.m_frequency != -1)
                    baseFrequency = rec.m_frequency;
                if (rec.m_guard != -1)
                    guardValue = rec.m_guard;
                if (!rec.m_repulseId.empty())
                    repulseId = rec.m_repulseId;
                if (!rec.m_enabled)
                    continue;
            }
        }

        for (int i = 0, sz = (int) bank->variants.size(); i < sz; i++) {
            RecordBank record;
            record.m_guardsVariant = i;
            record.m_id            = bank;
            record.m_guardValue    = guardValue;
            record.m_repulseId     = repulseId;
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
                    const Core::MapScore score = estimateReward(reward, Core::ScoreAttr::Army);

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
                    record.m_attempts = 9;
            }

            m_records.m_records.push_back(record);
        }
    }
    m_records.updateFrequency();
}

IZoneObjectPtr ObjectGenerator::ObjectFactoryBank::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    const bool upgraded = record.m_id->upgradedStackIndex == -1 ? false : m_rng->genSmall(3) == 0;
    ObjectBank obj;

    std::vector<AcceptableArtifact> accArts;
    obj.m_obj.m_generationId  = m_scoreId;
    obj.m_obj.m_id            = record.m_id;
    obj.m_obj.m_guardsVariant = record.m_guardsVariant;
    obj.m_repulseId           = record.m_repulseId;
    obj.m_obj.m_upgradedStack = upgraded ? FHBank::UpgradedStack::Yes : FHBank::UpgradedStack::No;
    obj.m_map                 = &m_map;

    const Core::Reward& reward = record.m_id->rewards[record.m_id->variants[record.m_guardsVariant].rewardIndex];
    Core::MapScore      score  = estimateReward(reward, Core::ScoreAttr::Army);

    {
        const Core::ArtifactFilter firstFilter = reward.artifacts.empty() ? Core::ArtifactFilter{} : reward.artifacts[0];
        for (const auto& filter : reward.artifacts) {
            auto accArt = m_artifactPool->make(filter, filter, firstFilter == filter, m_scoreSettings);

            assert(accArt.m_art);
            obj.m_obj.m_artifacts.push_back(accArt.m_art);
            estimateArtScore(accArt.m_art, score);
            accArts.push_back(std::move(accArt));
        }
    }
    obj.m_obj.m_score = score;
    obj.m_obj.m_guard = record.m_guardValue;
    obj.m_onDisable   = [this, &record, accArts] {
        m_records.onDisable(record);
        for (const auto& art : accArts)
            art.m_onDiscard();
    };
    obj.m_onAccept = [this, &record] {
        m_records.onAccept(record);
    };

    return std::make_shared<ObjectBank>(std::move(obj));
}
// ---------------------------------------------------------------------------------------

struct ObjectGenerator::ObjectFactoryArtifact::ObjectArtifact : public AbstractObjectWithId<FHArtifact> {
    Type               getType() const override { return Type::Pickable; }
    bool               preventDuplicates() const override { return true; }
    std::string        getRepulseId() const override { return m_repulseId; }
    Core::CombinedMask getMask() const override { return g_oneTileMask; }

    std::string m_repulseId;
};

ObjectGenerator::ObjectFactoryArtifact::ObjectFactoryArtifact(FHMap&                              map,
                                                              const FHRngZone::GeneratorArtifact& genSettings,
                                                              const FHScoreSettings&              scoreSettings,
                                                              const std::string&                  scoreId,
                                                              const Core::IGameDatabase*          database,
                                                              Core::IRandomGenerator* const       rng,
                                                              ArtifactPool*                       artifactPool)
    : AbstractFactory<RecordArtifact>(map, scoreSettings, scoreId, database, rng)
    , m_artifactPool(artifactPool)
{
    if (!genSettings.m_isEnabled)
        return;

    for (const auto& [_, value] : genSettings.m_records) {
        if (m_artifactPool->isEmpty(value.m_filter, true, scoreSettings))
            continue;

        auto rec = RecordArtifact{ .m_filter = value.m_filter, .m_pool = value.m_pool, .m_repulseId = value.m_repulseId };

        rec.m_attempts = 3;

        m_records.m_records.push_back(rec.setFreq(value.m_frequency));
    }

    m_records.updateFrequency();
}

IZoneObjectPtr ObjectGenerator::ObjectFactoryArtifact::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectArtifact obj;
    obj.m_obj.m_generationId = m_scoreId;
    obj.m_map                = &m_map;
    auto accArt              = m_artifactPool->make(record.m_pool, record.m_filter, true, m_scoreSettings);
    obj.m_obj.m_id           = accArt.m_art;
    obj.m_repulseId          = record.m_repulseId;
    assert(obj.m_obj.m_id);
    obj.m_onDisable = [this, &record, accArt] {
        m_records.onDisable(record);
        accArt.m_onDiscard();
    };

    estimateArtScore(obj.m_obj.m_id, obj.m_obj.m_score);

    obj.m_obj.m_guard = obj.m_obj.m_id->guard;

    return std::make_shared<ObjectArtifact>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

Core::CombinedMask ObjectResourcePile::getMask() const
{
    return g_oneTileMask;
}

ObjectGenerator::ObjectFactoryResourcePile::ObjectFactoryResourcePile(FHMap&                                  map,
                                                                      const FHRngZone::GeneratorResourcePile& genSettings,
                                                                      const FHScoreSettings&                  scoreSettings,
                                                                      const std::string&                      scoreId,
                                                                      const Core::IGameDatabase*              database,
                                                                      Core::IRandomGenerator* const           rng)
    : AbstractFactory<RecordResourcePile>(map, scoreSettings, scoreId, database, rng)
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

IZoneObjectPtr ObjectGenerator::ObjectFactoryResourcePile::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectResourcePile obj = record.m_obj;
    obj.m_onDisable        = [this, &record] {
        m_records.onDisable(record);
    };
    obj.m_map                = &m_map;
    obj.m_obj.m_generationId = m_scoreId;

    return std::make_shared<ObjectResourcePile>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

Core::CombinedMask ObjectPandora::getMask() const
{
    return g_oneTileMask;
}

ObjectGenerator::ObjectFactoryPandora::ObjectFactoryPandora(FHMap&                             map,
                                                            const FHRngZone::GeneratorPandora& genSettings,
                                                            const FHScoreSettings&             scoreSettings,
                                                            const std::string&                 scoreId,
                                                            const Core::IGameDatabase*         database,
                                                            Core::IRandomGenerator* const      rng,
                                                            Core::LibraryFactionConstPtr       rewardsFaction)
    : AbstractFactory<RecordPandora>(map, scoreSettings, scoreId, database, rng)
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
        obj.m_obj.m_key    = id;
        obj.m_obj.m_guard  = value.m_guard;
        obj.m_obj.m_reward = value.m_reward;
        obj.m_obj.m_score  = estimateReward(value.m_reward);
        obj.m_repulseId    = value.m_repulseId;
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

IZoneObjectPtr ObjectGenerator::ObjectFactoryPandora::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectPandora obj        = record.m_obj;
    obj.m_obj.m_generationId = m_scoreId;
    obj.m_onDisable          = [this, &record] {
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
            obj.m_obj.m_key += suffix;
        }
        obj.m_obj.m_reward.units.push_back({ unit, count });
    }

    return std::make_shared<ObjectPandora>(std::move(obj));
}

// ---------------------------------------------------------------------------------------
struct ObjectGenerator::ObjectFactoryShrine::ObjectShrine : public AbstractObject<FHShrine> {
    std::string getId() const override { return "shrine " + this->m_obj.m_spellId->id; }

    Type                           getType() const override { return Type::Visitable; }
    Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_visitableId->objectDefs.get({}); }
    std::string                    getRepulseId() const override { return m_repulseId; }

    std::string m_repulseId;
};

ObjectGenerator::ObjectFactoryShrine::ObjectFactoryShrine(FHMap&                            map,
                                                          const FHRngZone::GeneratorShrine& genSettings,
                                                          const FHScoreSettings&            scoreSettings,
                                                          const std::string&                scoreId,
                                                          const Core::IGameDatabase*        database,
                                                          Core::IRandomGenerator* const     rng,
                                                          SpellPool*                        spellPool)
    : AbstractFactory<RecordSpellShrine>(map, scoreSettings, scoreId, database, rng)
    , m_spellPool(spellPool)
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
                .m_repulseId   = value.m_repulseId,
            };

            m_records.m_records.push_back(rec.setFreq(value.m_frequency));
        }
    }

    m_records.updateFrequency();
}

IZoneObjectPtr ObjectGenerator::ObjectFactoryShrine::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectShrine obj;
    obj.m_onDisable = [this, &record] {
        m_records.onDisable(record);
    };
    obj.m_map                = &m_map;
    obj.m_obj.m_generationId = m_scoreId;
    obj.m_obj.m_visitableId  = record.m_visitableId;
    obj.m_obj.m_spellId      = m_spellPool->make(record.m_filter, record.m_asAnySpell, m_scoreSettings);
    obj.m_repulseId          = record.m_repulseId;

    assert(obj.m_obj.m_spellId);

    estimateSpellScore(obj.m_obj.m_spellId, obj.m_obj.m_score, record.m_asAnySpell);

    obj.m_obj.m_guard = record.m_guard;
    if (obj.m_obj.m_guard == -1)
        obj.m_obj.m_guard = obj.m_obj.m_spellId->value * 2 * 3 / 4; // shrine = 75% of spell value

    return std::make_shared<ObjectShrine>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

struct ObjectGenerator::ObjectFactoryScroll::ObjectScroll : public AbstractObjectWithId<FHArtifact> {
    Type               getType() const override { return Type::Pickable; }
    bool               preventDuplicates() const override { return true; }
    std::string        getRepulseId() const override { return m_repulseId; }
    Core::CombinedMask getMask() const override { return g_oneTileMask; }

    std::string m_repulseId;
};

ObjectGenerator::ObjectFactoryScroll::ObjectFactoryScroll(FHMap&                            map,
                                                          const FHRngZone::GeneratorScroll& genSettings,
                                                          const FHScoreSettings&            scoreSettings,
                                                          const std::string&                scoreId,
                                                          const Core::IGameDatabase*        database,
                                                          Core::IRandomGenerator* const     rng,
                                                          SpellPool*                        spellPool)
    : AbstractFactory<RecordSpellScroll>(map, scoreSettings, scoreId, database, rng)
    , m_spellPool(spellPool)
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

            auto rec = RecordSpellScroll{ .m_filter = value.m_filter, .m_guard = value.m_guard, .m_asAnySpell = asAnySpell, .m_repulseId = value.m_repulseId };

            m_records.m_records.push_back(rec.setFreq(value.m_frequency));
        }
    }

    m_records.updateFrequency();
}

IZoneObjectPtr ObjectGenerator::ObjectFactoryScroll::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectScroll obj;
    obj.m_onDisable = [this, &record] {
        m_records.onDisable(record);
    };
    obj.m_map       = &m_map;
    obj.m_repulseId = record.m_repulseId;
    auto spellId    = m_spellPool->make(record.m_filter, record.m_asAnySpell, m_scoreSettings);
    assert(spellId);
    obj.m_obj.m_generationId = m_scoreId;
    obj.m_obj.m_id           = m_scrollMapping.at(spellId);
    assert(obj.m_obj.m_id);

    estimateSpellScore(spellId, obj.m_obj.m_score, record.m_asAnySpell);

    obj.m_obj.m_guard = record.m_guard;
    if (obj.m_obj.m_guard == -1)
        obj.m_obj.m_guard = spellId->value * 2; // scroll = 100% of spell value

    return std::make_shared<ObjectScroll>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

Core::LibraryObjectDefConstPtr ObjectGenerator::ObjectFactoryDwelling::ObjectDwelling::getDef() const
{
    return m_obj.m_id->objectDefs.get({});
}

ObjectGenerator::ObjectFactoryDwelling::ObjectFactoryDwelling(FHMap&                              map,
                                                              const FHRngZone::GeneratorDwelling& genSettings,
                                                              const FHScoreSettings&              scoreSettings,
                                                              const std::string&                  scoreId,
                                                              const Core::IGameDatabase*          database,
                                                              Core::IRandomGenerator* const       rng,
                                                              Core::LibraryFactionConstPtr        dwellFaction)
    : AbstractFactory<RecordDwelling>(map, scoreSettings, scoreId, database, rng)
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
            if (value.m_factionValues.contains(dwellFaction)) {
                rec.m_value = value.m_factionValues.at(dwellFaction);
            }
            rec.m_frequency = value.m_frequency;
            if (scoreSettings.isValidValue(Core::ScoreAttr::ArmyDwelling, rec.m_value))
                m_records.m_records.push_back(rec);
        }
    }

    m_records.updateFrequency();
}

IZoneObjectPtr ObjectGenerator::ObjectFactoryDwelling::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectDwelling obj;
    obj.m_onDisable = [this, &record] {
        m_records.onDisable(record);
    };
    obj.m_map                = &m_map;
    obj.m_obj.m_generationId = m_scoreId;
    obj.m_obj.m_id           = record.m_id;
    obj.m_obj.m_player       = m_none;

    obj.m_obj.m_score[Core::ScoreAttr::ArmyDwelling] = record.m_value;

    obj.m_obj.m_guard = record.m_guard;

    return std::make_shared<ObjectDwelling>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

struct ObjectGenerator::ObjectFactoryVisitable::ObjectVisitable : public AbstractObject<FHVisitable> {
    std::string getId() const override { return this->m_obj.m_visitableId->id; }
    Type        getType() const override
    {
        using enum Core::LibraryMapVisitable::VisitKind;
        switch (m_obj.m_visitableId->visitKind) {
            case Invalid:
                return Type::Visitable;
            case Normal:
                return Type::Visitable;
            case Pick:
                return Type::Pickable;
            case Remove:
                return Type::Removable;
        }
        assert(0);
        return Type::Visitable;
    }
    Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_visitableId->objectDefs.get({}); }
};

ObjectGenerator::ObjectFactoryVisitable::ObjectFactoryVisitable(FHMap&                               map,
                                                                const FHRngZone::GeneratorVisitable& genSettings,
                                                                const FHScoreSettings&               scoreSettings,
                                                                const std::string&                   scoreId,
                                                                const Core::IGameDatabase*           database,
                                                                Core::IRandomGenerator* const        rng,
                                                                Core::LibraryTerrainConstPtr         terrain)
    : AbstractFactory<RecordVisitable>(map, scoreSettings, scoreId, database, rng)
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

        const int scoreValue = visitable->value;
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

IZoneObjectPtr ObjectGenerator::ObjectFactoryVisitable::make(uint64_t rngFreq)
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
    obj.m_map                = &m_map;
    obj.m_obj                = record.m_obj;
    obj.m_obj.m_generationId = m_scoreId;

    return std::make_shared<ObjectVisitable>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

struct ObjectGenerator::ObjectFactoryMine::ObjectMine : public AbstractObject<FHMine> {
    std::string                    getId() const override { return "mine." + this->m_obj.m_id->id; }
    Type                           getType() const override { return Type::Visitable; }
    Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_id->minesDefs.get(m_obj.m_defIndex); }
};

ObjectGenerator::ObjectFactoryMine::ObjectFactoryMine(FHMap&                          map,
                                                      const FHRngZone::GeneratorMine& genSettings,
                                                      const FHScoreSettings&          scoreSettings,
                                                      const std::string&              scoreId,
                                                      const Core::IGameDatabase*      database,
                                                      Core::IRandomGenerator* const   rng,
                                                      Core::LibraryTerrainConstPtr    terrain)
    : AbstractFactory<RecordMine>(map, scoreSettings, scoreId, database, rng)
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

IZoneObjectPtr ObjectGenerator::ObjectFactoryMine::make(uint64_t rngFreq)
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
    obj.m_map                = &m_map;
    obj.m_obj                = record.m_obj;
    obj.m_obj.m_generationId = m_scoreId;

    return std::make_shared<ObjectMine>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

struct ObjectGenerator::ObjectFactorySkillHut::ObjectSkillHut : public AbstractObject<FHSkillHut> {
    std::string                    getId() const override { return this->m_obj.m_visitableId->id; }
    Type                           getType() const override { return Type::Visitable; }
    Core::LibraryObjectDefConstPtr getDef() const override { return m_obj.m_visitableId->objectDefs.get({}); }
};

ObjectGenerator::ObjectFactorySkillHut::ObjectFactorySkillHut(FHMap&                              map,
                                                              const FHRngZone::GeneratorSkillHut& genSettings,
                                                              const FHScoreSettings&              scoreSettings,
                                                              const std::string&                  scoreId,
                                                              const Core::IGameDatabase*          database,
                                                              Core::IRandomGenerator* const       rng)
    : AbstractFactory<RecordSkillHut>(map, scoreSettings, scoreId, database, rng)
{
    if (!genSettings.m_isEnabled)
        return;

    auto* visitable = database->mapVisitables()->find("sod.visitable.witchHut");
    assert(visitable);

    for (auto* skill : database->secSkills()->records()) {
        if (map.m_disabledSkills.isDisabled(map.m_isWaterMap, skill))
            continue;
        if (!skill->isTeachable)
            continue;

        RecordSkillHut record;
        record.m_obj.m_visitableId = visitable;
        record.m_frequency         = genSettings.m_frequency;
        record.m_maxLimit          = visitable->maxZone;
        record.m_minLimit          = visitable->minZone;

        const int scoreValue = skill->value;
        if (!scoreValue)
            throw std::runtime_error("'" + skill->id + "' has no valid score!");

        const Core::ScoreAttr attr = Core::ScoreAttr::Upgrade;

        record.m_obj.m_score[attr] = scoreValue;

        record.m_obj.m_guard = genSettings.m_guard;

        if (scoreSettings.isValidValue(attr, scoreValue)) {
            m_records.m_records.push_back(record);
        }
    }

    m_records.updateFrequency();
}

IZoneObjectPtr ObjectGenerator::ObjectFactorySkillHut::make(uint64_t rngFreq)
{
    const size_t index  = m_records.getFreqIndex(rngFreq);
    auto&        record = m_records.m_records[index];

    ObjectSkillHut obj;
    obj.m_onDisable = [this, &record] {
        m_records.onDisable(record);
    };
    obj.m_onAccept = [this, &record] {
        m_records.onAccept(record);
    };
    obj.m_map                = &m_map;
    obj.m_obj                = record.m_obj;
    obj.m_obj.m_generationId = m_scoreId;

    return std::make_shared<ObjectSkillHut>(std::move(obj));
}

// ---------------------------------------------------------------------------------------

}
