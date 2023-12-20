/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ObjectGenerator.hpp"
#include "ObjectGeneratorUtils.hpp"

namespace FreeHeroes {

// ---------------------------------------------------------------------------------------

struct RecordBank : public CommonRecord<RecordBank> {
    Core::LibraryMapBankConstPtr m_id            = nullptr;
    int                          m_guardsVariant = 0;
    int                          m_guardValue    = 0;
};

struct ObjectGenerator::ObjectFactoryBank : public AbstractFactory<RecordBank> {
    struct ObjectBank;

    ObjectFactoryBank(FHMap&                          map,
                      const FHRngZone::GeneratorBank& genSettings,
                      const FHScoreSettings&          scoreSettings,
                      const std::string&              scoreId,
                      const Core::IGameDatabase*      database,
                      Core::IRandomGenerator* const   rng,
                      ArtifactPool*                   artifactPool,
                      Core::LibraryTerrainConstPtr    terrain);

    IZoneObjectPtr make(uint64_t rngFreq) override;

    ArtifactPool* const m_artifactPool;
};

// ---------------------------------------------------------------------------------------

struct RecordArtifact : public CommonRecord<RecordArtifact> {
    Core::ArtifactFilter m_filter;
    Core::ArtifactFilter m_pool;
};

struct ObjectGenerator::ObjectFactoryArtifact : public AbstractFactory<RecordArtifact> {
    struct ObjectArtifact;

    ObjectFactoryArtifact(FHMap&                              map,
                          const FHRngZone::GeneratorArtifact& genSettings,
                          const FHScoreSettings&              scoreSettings,
                          const std::string&                  scoreId,
                          const Core::IGameDatabase*          database,
                          Core::IRandomGenerator* const       rng,
                          ArtifactPool*                       artifactPool);

    IZoneObjectPtr make(uint64_t rngFreq) override;

    ArtifactPool* const m_artifactPool;
};

// ---------------------------------------------------------------------------------------

struct ObjectResourcePile : public ObjectGenerator::AbstractObjectWithId<FHResource> {
    Type               getType() const override { return Type::Pickable; }
    Core::CombinedMask getMask() const override;
};

struct RecordResourcePile : public CommonRecord<RecordResourcePile> {
    ObjectResourcePile m_obj;
};

struct ObjectGenerator::ObjectFactoryResourcePile : public AbstractFactory<RecordResourcePile> {
    ObjectFactoryResourcePile(FHMap&                                  map,
                              const FHRngZone::GeneratorResourcePile& genSettings,
                              const FHScoreSettings&                  scoreSettings,
                              const std::string&                      scoreId,
                              const Core::IGameDatabase*              database,
                              Core::IRandomGenerator* const           rng);

    IZoneObjectPtr make(uint64_t rngFreq) override;
};

// ---------------------------------------------------------------------------------------

struct ObjectPandora : public ObjectGenerator::AbstractObject<FHPandora> {
    std::string        getId() const override { return "pandora_" + m_obj.m_key; }
    Type               getType() const override { return Type::Pickable; }
    bool               preventDuplicates() const override { return true; }
    Core::CombinedMask getMask() const override;
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
                         const std::string&                 scoreId,
                         const Core::IGameDatabase*         database,
                         Core::IRandomGenerator* const      rng,
                         Core::LibraryFactionConstPtr       rewardsFaction);

    IZoneObjectPtr make(uint64_t rngFreq) override;
};

// ---------------------------------------------------------------------------------------

struct RecordSpellShrine : public CommonRecord<RecordSpellShrine> {
    Core::SpellFilter                 m_filter;
    Core::LibraryMapVisitableConstPtr m_visitableId = nullptr;
    int                               m_guard       = -1;
    bool                              m_asAnySpell  = false;
};

struct ObjectGenerator::ObjectFactoryShrine : public AbstractFactory<RecordSpellShrine> {
    struct ObjectShrine;

    ObjectFactoryShrine(FHMap&                            map,
                        const FHRngZone::GeneratorShrine& genSettings,
                        const FHScoreSettings&            scoreSettings,
                        const std::string&                scoreId,
                        const Core::IGameDatabase*        database,
                        Core::IRandomGenerator* const     rng,
                        SpellPool*                        spellPool);

    IZoneObjectPtr make(uint64_t rngFreq) override;

    SpellPool* const m_spellPool;
};

// ---------------------------------------------------------------------------------------

struct RecordSpellScroll : public CommonRecord<RecordSpellScroll> {
    Core::SpellFilter m_filter;
    int               m_guard      = -1;
    bool              m_asAnySpell = false;
};

struct ObjectGenerator::ObjectFactoryScroll : public AbstractFactory<RecordSpellScroll> {
    struct ObjectScroll;

    ObjectFactoryScroll(FHMap&                            map,
                        const FHRngZone::GeneratorScroll& genSettings,
                        const FHScoreSettings&            scoreSettings,
                        const std::string&                scoreId,
                        const Core::IGameDatabase*        database,
                        Core::IRandomGenerator* const     rng,
                        SpellPool*                        spellPool);

    IZoneObjectPtr make(uint64_t rngFreq) override;

    std::map<Core::LibrarySpellConstPtr, Core::LibraryArtifactConstPtr> m_scrollMapping;
    SpellPool* const                                                    m_spellPool;
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
        Core::LibraryObjectDefConstPtr getDef() const override;
    };

    ObjectFactoryDwelling(FHMap&                              map,
                          const FHRngZone::GeneratorDwelling& genSettings,
                          const FHScoreSettings&              scoreSettings,
                          const std::string&                  scoreId,
                          const Core::IGameDatabase*          database,
                          Core::IRandomGenerator* const       rng,
                          Core::LibraryFactionConstPtr        dwellFaction);

    IZoneObjectPtr make(uint64_t rngFreq) override;

    Core::LibraryPlayerConstPtr m_none;
};

// ---------------------------------------------------------------------------------------

struct RecordVisitable : public CommonRecord<RecordVisitable> {
    FHVisitable m_obj;
};

struct ObjectGenerator::ObjectFactoryVisitable : public AbstractFactory<RecordVisitable> {
    struct ObjectVisitable;

    ObjectFactoryVisitable(FHMap&                               map,
                           const FHRngZone::GeneratorVisitable& genSettings,
                           const FHScoreSettings&               scoreSettings,
                           const std::string&                   scoreId,
                           const Core::IGameDatabase*           database,
                           Core::IRandomGenerator* const        rng,
                           Core::LibraryTerrainConstPtr         terrain);

    IZoneObjectPtr make(uint64_t rngFreq) override;
};

// ---------------------------------------------------------------------------------------

struct RecordMine : public CommonRecord<RecordMine> {
    FHMine m_obj;
};

struct ObjectGenerator::ObjectFactoryMine : public AbstractFactory<RecordMine> {
    struct ObjectMine;

    ObjectFactoryMine(FHMap&                          map,
                      const FHRngZone::GeneratorMine& genSettings,
                      const FHScoreSettings&          scoreSettings,
                      const std::string&              scoreId,
                      const Core::IGameDatabase*      database,
                      Core::IRandomGenerator* const   rng,
                      Core::LibraryTerrainConstPtr    terrain);

    IZoneObjectPtr make(uint64_t rngFreq) override;
};

// ---------------------------------------------------------------------------------------

struct RecordSkillHut : public CommonRecord<RecordVisitable> {
    FHSkillHut m_obj;
};

struct ObjectGenerator::ObjectFactorySkillHut : public AbstractFactory<RecordSkillHut> {
    struct ObjectSkillHut;

    ObjectFactorySkillHut(FHMap&                              map,
                          const FHRngZone::GeneratorSkillHut& genSettings,
                          const FHScoreSettings&              scoreSettings,
                          const std::string&                  scoreId,
                          const Core::IGameDatabase*          database,
                          Core::IRandomGenerator* const       rng);

    IZoneObjectPtr make(uint64_t rngFreq) override;
};

// ---------------------------------------------------------------------------------------

}
