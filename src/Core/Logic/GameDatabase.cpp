/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameDatabase.hpp"

#include "JsonHelper.hpp"
#include "Logger.hpp"

#include "LibraryFaction.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryUnit.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryArtifact.hpp"
#include "LibraryHero.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryResource.hpp"
#include "LibraryMapObject.hpp"

#include "LibraryReflection.hpp"
#include "AdventureReflection.hpp"
#include "BattleReflection.hpp"

#include "LibraryIdResolver.hpp"

#include "LibrarySerialize.hpp"

#include "IResourceLibrary.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <deque>
#include <unordered_map>
#include <cstddef>



using namespace nlohmann;

namespace FreeHeroes::Core {
namespace {
template<class T>
struct LibraryContainerKey { static constexpr const void ** scopeName = nullptr;};
template<> struct LibraryContainerKey<LibraryTerrain       > { static constexpr const char * scopeName = "terrains" ; };
template<> struct LibraryContainerKey<LibraryResource      > { static constexpr const char * scopeName = "resources" ; };
template<> struct LibraryContainerKey<LibraryFaction       > { static constexpr const char * scopeName = "factions" ; };
template<> struct LibraryContainerKey<LibrarySecondarySkill> { static constexpr const char * scopeName = "skills" ; };
template<> struct LibraryContainerKey<LibrarySpell         > { static constexpr const char * scopeName = "spells" ; };
template<> struct LibraryContainerKey<LibraryUnit          > { static constexpr const char * scopeName = "units" ; };
template<> struct LibraryContainerKey<LibraryHeroSpec      > { static constexpr const char * scopeName = "specialities" ; };
template<> struct LibraryContainerKey<LibraryArtifact      > { static constexpr const char * scopeName = "artifacts" ; };
template<> struct LibraryContainerKey<LibraryHero          > { static constexpr const char * scopeName = "heroes" ; };
template<> struct LibraryContainerKey<LibraryMapObject     > { static constexpr const char * scopeName = "mapObjects" ; };

}

struct GameDatabase::Impl {


    template<class T>
    struct LibraryContainer : public ContainerInterface<T> {
        std::unordered_map<std::string, size_t> m_index;
        std::deque<T> m_objects;
        std::vector<T*> m_unsorted;
        std::vector<const T*> m_sorted;



        const T * find(const std::string & id) const override{
            auto it = m_index.find(id);
            return it == m_index.cend() ? nullptr : &m_objects[it->second];
        }
        T * findMutable(const std::string & id) {
            auto it = m_index.find(id);
            return it == m_index.end() ? nullptr : &m_objects[it->second];
        }
        const std::vector<const T*> & records() const override {
            return m_sorted;
        }

        T& insertObject(std::string id, T && value) {
            m_objects.push_back(std::move(value));
            T& objRecord = m_objects.back();
            objRecord.id = id;

            m_index[id] = m_objects.size() - 1;
            m_unsorted.push_back(&objRecord);
            return objRecord;
        }
        T& insertObject(const std::string & id) {
            return insertObject(id, {});
        }

        bool loadRecordList(Reflection::LibraryIdResolver& idResolver, const json & recordListMap){
            if (!recordListMap.contains(LibraryContainerKey<T>::scopeName))
                return true;
            const json & recordList = recordListMap[LibraryContainerKey<T>::scopeName];
            for (auto it = recordList.cbegin(); it != recordList.cend(); ++it) {
                const std::string & id = it.key();
                const auto & jsonObj = it.value();
                if (jsonObj.type() == json::value_t::null)
                    continue;

                auto * objRecord = findMutable(id);
                assert(objRecord);

                if (!Reflection::deserialize(idResolver, *objRecord, jsonObj))
                    return false;

            }
            return true;
        }

        void prepareObjectKeys(const json & recordListMap) {
            if (!recordListMap.contains(LibraryContainerKey<T>::scopeName))
                return;

            const json & recordList = recordListMap[LibraryContainerKey<T>::scopeName];
            m_index.clear();
            m_objects.clear();
            m_unsorted.clear();
            m_sorted.clear();
            for (auto it = recordList.cbegin(); it != recordList.cend(); ++it) {
                const std::string & id = it.key();
                const auto & jsonObj = it.value();

                if (jsonObj.type() == json::value_t::null)
                    continue;

                auto & objRecord = insertObject(id);
                objRecord.id = id;
            }
            m_sorted.reserve(m_unsorted.size());
        }


    };
    LibraryContainer<LibraryTerrain>        m_terrains;
    LibraryContainer<LibraryResource>       m_resources;

    LibraryContainer<LibraryFaction>        m_factions;
    LibraryContainer<LibrarySecondarySkill> m_skills;
    LibraryContainer<LibrarySpell>          m_spells;

    LibraryContainer<LibraryUnit>           m_units;
    LibraryContainer<LibraryHeroSpec>       m_specs;
    LibraryContainer<LibraryArtifact>       m_artifacts;

    LibraryContainer<LibraryHero>           m_heroes;
    LibraryContainer<LibraryMapObject>      m_mapObjects;


    template<typename T>
    LibraryContainer<T> & getContainer() {
        static_assert(sizeof (T) == 3);
    }
    template<typename T>
    T* findMutable(const T* obj) {
        return getContainer<T>().findMutable(obj->id);
    }
};
template<>
GameDatabase::Impl::LibraryContainer<LibraryTerrain> & GameDatabase::Impl::getContainer<LibraryTerrain>() {
    return m_terrains;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryResource> & GameDatabase::Impl::getContainer<LibraryResource>() {
    return m_resources;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryFaction> & GameDatabase::Impl::getContainer<LibraryFaction>() {
    return m_factions;
}
template<>
GameDatabase::Impl::LibraryContainer<LibrarySecondarySkill> & GameDatabase::Impl::getContainer<LibrarySecondarySkill>() {
    return m_skills;
}
template<>
GameDatabase::Impl::LibraryContainer<LibrarySpell> & GameDatabase::Impl::getContainer<LibrarySpell>() {
    return m_spells;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryUnit> & GameDatabase::Impl::getContainer<LibraryUnit>() {
    return m_units;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryHeroSpec> & GameDatabase::Impl::getContainer<LibraryHeroSpec>() {
    return m_specs;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryArtifact> & GameDatabase::Impl::getContainer<LibraryArtifact>() {
    return m_artifacts;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryHero> & GameDatabase::Impl::getContainer<LibraryHero>() {
    return m_heroes;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryMapObject> & GameDatabase::Impl::getContainer<LibraryMapObject>() {
    return m_mapObjects;
}


GameDatabase::GameDatabase(const std::string & dataBaseId, const IResourceLibrary & resourceLibrary)
    : m_impl(std::make_unique<Impl>())
{
    std::vector<std_path> files;
    const ResourceDatabase & desc = resourceLibrary.getDatabase(dataBaseId);
    for (auto & f : desc.filesFullPathsWithDeps) {
        files.push_back(f);
    }
    load(files); // @todo: exception throw??

    // just hacks to make linker happy...
    Reflection::libraryReflectionStub();
    Reflection::adventureReflectionStub();
    Reflection::battleReflectionStub();
}

GameDatabase::~GameDatabase()
{
}

IGameDatabase::LibraryUnitContainerPtr GameDatabase::units() const
{
    return &m_impl->m_units;
}

IGameDatabase::LibraryHeroContainerPtr GameDatabase::heroes() const
{
    return &m_impl->m_heroes;
}

IGameDatabase::LibraryArtifactContainerPtr GameDatabase::artifacts() const
{
    return &m_impl->m_artifacts;
}

IGameDatabase::LibrarySecondarySkillContainerPtr GameDatabase::secSkills() const
{
    return &m_impl->m_skills;
}

IGameDatabase::LibraryFactionContainerPtr GameDatabase::factions() const
{
    return &m_impl->m_factions;
}

IGameDatabase::LibrarySpellContainerPtr GameDatabase::spells() const
{
    return &m_impl->m_spells;
}

IGameDatabase::LibraryResourceContainerPtr GameDatabase::resources() const
{
    return &m_impl->m_resources;
}

IGameDatabase::LibraryTerrainContainerPtr GameDatabase::terrains() const
{
    return &m_impl->m_terrains;
}

IGameDatabase::LibraryMapObjectContainerPtr GameDatabase::mapObjects() const
{
    return &m_impl->m_mapObjects;
}

IGameDatabase::LibraryHeroSpecContainerPtr GameDatabase::heroSpecs() const
{
    return &m_impl->m_specs;
}


bool GameDatabase::load(const std::vector<std_path>& files)
{
    json recordObjectMaps = json::object();
    for (const auto & filename : files) {
        std::ifstream ifsMain(filename);
        if (!ifsMain)
            return false;
        json main;
        ifsMain >> main;

        const int totalRecordsFound = addJsonObjectToIndex(recordObjectMaps, main);
        Logger(Logger::Info) << "Database JSON parsing finished: " << filename << ", total records:" << totalRecordsFound;
    }


    m_impl->m_terrains   .prepareObjectKeys(recordObjectMaps);
    m_impl->m_resources  .prepareObjectKeys(recordObjectMaps);
    m_impl->m_factions   .prepareObjectKeys(recordObjectMaps);
    m_impl->m_skills     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_spells     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_units      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_specs      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_artifacts  .prepareObjectKeys(recordObjectMaps);
    m_impl->m_heroes     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_mapObjects .prepareObjectKeys(recordObjectMaps);

    Reflection::LibraryIdResolver idResolver(*this);

    const bool result =
               m_impl->m_terrains   .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_resources  .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_factions   .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_skills     .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_spells     .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_units      .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_specs      .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_artifacts  .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_heroes     .loadRecordList(idResolver, recordObjectMaps)
            && m_impl->m_mapObjects .loadRecordList(idResolver, recordObjectMaps)
            ;

    if (!result)
        return false;

    // making object links.
    for (auto spec : m_impl->m_specs.m_unsorted) {
        if (spec->type == LibraryHeroSpec::Type::Unit
                || spec->type == LibraryHeroSpec::Type::UnitUpgrade
                || spec->type == LibraryHeroSpec::Type::UnitNonStd
                || spec->type == LibraryHeroSpec::Type::SpecialCannon
                || spec->type == LibraryHeroSpec::Type::SpecialBallista) {

            assert(spec->unit);
        }
        if (spec->type == LibraryHeroSpec::Type::Skill) {
            assert(spec->skill);
        }
        if (spec->type == LibraryHeroSpec::Type::Spell) {
            assert(spec->spell);
        }

    }
    for (auto unit : m_impl->m_units.m_unsorted) {
        assert(unit->faction); // actually missing faction is error
        assert(!unit->faction->id.empty());
    }
    for (auto hero : m_impl->m_heroes.m_unsorted) {

        assert(hero->faction);
        assert(hero->spec);

        for (auto & subSkill : hero->secondarySkills) {
            assert(subSkill.skill);
        }
        for (auto & unit : hero->startStacks) {
            assert(unit.unit);
        }
    }
    // factions postproc
    auto sortedFactions = m_impl->m_factions.m_unsorted;
    for (auto faction : sortedFactions) {
        if (faction->presentationParams.goesAfterId.empty())
            faction->generatedOrder = 1;
        else
            faction->generatedOrder = 0;
    }
    while (true) {
        bool updates = false;
        for (auto faction : sortedFactions) {
            if (faction->generatedOrder == 0) {
                  auto beforeFaction = m_impl->m_factions.find(faction->presentationParams.goesAfterId);
                  assert(beforeFaction);
                  if (beforeFaction->generatedOrder > 0) {
                      faction->generatedOrder = beforeFaction->generatedOrder + 1;
                      updates = true;
                  }
            }
        }
        if (!updates)
            break;
    }

    std::sort(sortedFactions.begin(), sortedFactions.end(), [](LibraryFaction * l, LibraryFaction * r){
        if (l->alignment != r->alignment)
            return l->alignment < r->alignment;
        return l->generatedOrder < r->generatedOrder;
    });

    int index = 0;
    for (auto faction : sortedFactions) {
        faction->generatedOrder = index++;
        for (auto * skill : m_impl->m_skills.m_unsorted){
            assert(skill);
            if (!faction->mageClass.secondarySkillWeights.contains(skill))
                faction->mageClass.secondarySkillWeights[skill] = 0;
            if (!faction->fighterClass.secondarySkillWeights.contains(skill))
                faction->fighterClass.secondarySkillWeights[skill] = 0;
        }
        // @todo: WTF? it contains nullptr!
        if (faction->mageClass.secondarySkillWeights.contains(nullptr))
            faction->mageClass.secondarySkillWeights.erase(nullptr);
        if (faction->fighterClass.secondarySkillWeights.contains(nullptr))
            faction->fighterClass.secondarySkillWeights.erase(nullptr);

        if (faction->alignment != LibraryFaction::Alignment::Special) {
            assert(!faction->mageClass.lowLevelIncrease.empty());
            assert(!faction->mageClass.highLevelIncrease.empty());
            assert(!faction->fighterClass.lowLevelIncrease.empty());
            assert(!faction->fighterClass.highLevelIncrease.empty());
        }

        m_impl->m_factions.m_sorted.push_back(faction);
    }

    // skills postproc
    std::sort(m_impl->m_skills.m_unsorted.begin(), m_impl->m_skills.m_unsorted.end(), [](auto * l, auto * r){
        return l->presentationParams.order < r->presentationParams.order;
    });

    for (auto * skill : m_impl->m_skills.m_unsorted) {
        m_impl->m_skills.m_sorted.push_back(skill);
    }

    // spells postproc
    std::sort(m_impl->m_spells.m_unsorted.begin(), m_impl->m_spells.m_unsorted.end(), [](auto * l, auto * r){
        if (l->level != r->level)
            return l->level < r->level;
        if (l->school != r->school)
            return l->school < r->school;
        return l->presentationParams.configOrder < r->presentationParams.configOrder;
    });
    index = 0;
    for (auto * spell : m_impl->m_spells.m_unsorted) {
        spell->presentationParams.order = index++;
        m_impl->m_spells.m_sorted.push_back(spell);
    }
    // additional autogenerated objects for artifacts.
    for (auto spell : m_impl->m_spells.m_unsorted) {
        if (!spell->isTeachable)
            continue;

        LibraryArtifact art;
        art.id = "sod.artifact." + spell->id;
        art.scrollSpell = spell;
        art.slot = ArtifactSlotType::Misc;
        art.untranslatedName = "Scroll " + spell->untranslatedName;
        art.presentationParams.iconStash = art.presentationParams.iconBonus = spell->presentationParams.iconScroll;
        art.presentationParams.order = spell->level * 1000 + spell->presentationParams.order;
        art.presentationParams.orderGroup = 1000;
        art.treasureClass = LibraryArtifact::TreasureClass::Scroll;
        m_impl->m_artifacts.insertObject(art.id, std::move(art));
    }

    // units postproc
    std::sort(m_impl->m_units.m_unsorted.begin(), m_impl->m_units.m_unsorted.end(), [](auto * l, auto * r){
        if (l->faction != r->faction)
            return l->faction->generatedOrder < r->faction->generatedOrder;

        return l->level < r->level;
    });
    for (auto * unit : m_impl->m_units.m_unsorted) {
        for (auto * upgUnit : unit->upgrades) {
            m_impl->findMutable(upgUnit)->prevUpgrade = unit;
        }

        m_impl->m_units.m_sorted.push_back(unit);
        m_impl->findMutable(unit->faction)->units.push_back(unit);

        unit->abilities.immunes.fillFilterCache(m_impl->m_spells.m_unsorted);
        unit->abilities.vulnerable.fillFilterCache(m_impl->m_spells.m_unsorted);
    }
    for (auto * unit : m_impl->m_units.m_unsorted) {
        LibraryUnitConstPtr unitIt = unit;
        while (unitIt->prevUpgrade) {
            unitIt = unitIt->prevUpgrade;
        }
        unit->baseUpgrade = unitIt;
    }
    // spec postproc - do we need an order at all?
    for (auto * spec : m_impl->m_specs.m_unsorted) {
        m_impl->m_specs.m_sorted.push_back(spec);
    }
    // artifacts postproc
    std::sort(m_impl->m_artifacts.m_unsorted.begin(), m_impl->m_artifacts.m_unsorted.end(), [](auto * l, auto * r){
        return l->sortOrdering() < r->sortOrdering();
    });
    for (auto * artifact : m_impl->m_artifacts.m_unsorted) {
        ArtifactSlotRequirement req;
        for (auto * setPart : artifact->parts) {
            assert(!setPart->partOfSet);
            assert(setPart->parts.empty());
            m_impl->findMutable(setPart)->partOfSet = artifact;

            req.add(setPart->slot);
        }
        if (artifact->parts.empty())
            req.add(artifact->slot);
        artifact->slotReq = req;
        artifact->provideSpells.fillFilterCache(m_impl->m_spells.m_unsorted);
        artifact->protectSpells.fillFilterCache(m_impl->m_spells.m_unsorted);
        artifact->forbidSpells.fillFilterCache(m_impl->m_spells.m_unsorted);

        m_impl->m_artifacts.m_sorted.push_back(artifact);
    }
    // heroes postproc
    std::sort(m_impl->m_heroes.m_unsorted.begin(), m_impl->m_heroes.m_unsorted.end(), [](auto * l, auto * r){
        if (l->faction != r->faction)
            return l->faction->generatedOrder < r->faction->generatedOrder;

        if ( l->isFighter != r->isFighter)
            return l->isFighter > r->isFighter;
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto * hero : m_impl->m_heroes.m_unsorted) {
        m_impl->m_heroes.m_sorted.push_back(hero);
        m_impl->findMutable(hero->faction)->heroes.push_back(hero);
    }
    // resource postproc
    std::sort(m_impl->m_resources.m_unsorted.begin(), m_impl->m_resources.m_unsorted.end(), [](auto * l, auto * r){
        return l->presentationParams.orderKingdom < r->presentationParams.orderKingdom;
    });
    for (auto * resource : m_impl->m_resources.m_unsorted) {
        m_impl->m_resources.m_sorted.push_back(resource);
    }
    // terrain postproc
    std::sort(m_impl->m_terrains.m_unsorted.begin(), m_impl->m_terrains.m_unsorted.end(), [](auto * l, auto * r){
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto * terrain : m_impl->m_terrains.m_unsorted) {
        m_impl->m_terrains.m_sorted.push_back(terrain);
    }
    // mapObjects postproc
    std::sort(m_impl->m_mapObjects.m_unsorted.begin(), m_impl->m_mapObjects.m_unsorted.end(), [](auto * l, auto * r){
        return l->order < r->order;
    });
    for (auto * obj : m_impl->m_mapObjects.m_unsorted) {

        for (auto & variant : obj->variants) {

            for (auto & guard : variant.guards) {
                assert(guard.unit);
            }
        }
        m_impl->m_mapObjects.m_sorted.push_back(obj);
    }

    return true;
}


}
