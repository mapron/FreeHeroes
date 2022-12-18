/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameDatabase.hpp"

#include "JsonHelper.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryGameRules.hpp"
#include "LibraryHero.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryResource.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryUnit.hpp"

#include "LibrarySerialize.hpp"
#include "FileFormatJson.hpp"
#include "FileIOUtils.hpp"

#include "IResourceLibrary.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <deque>
#include <unordered_map>
#include <cstddef>

namespace FreeHeroes::Core {
namespace {
template<class T>
struct LibraryContainerKey {
    static constexpr const void** scopeName = nullptr;
};

// clang-format off
template<> struct LibraryContainerKey<LibraryArtifact      > { static constexpr const char * scopeName = "artifacts" ; };
template<> struct LibraryContainerKey<LibraryDwelling      > { static constexpr const char * scopeName = "dwellings" ; };
template<> struct LibraryContainerKey<LibraryFaction       > { static constexpr const char * scopeName = "factions" ; };
template<> struct LibraryContainerKey<LibraryHero          > { static constexpr const char * scopeName = "heroes" ; };
template<> struct LibraryContainerKey<LibraryHeroSpec      > { static constexpr const char * scopeName = "specialities" ; };
template<> struct LibraryContainerKey<LibraryMapBank       > { static constexpr const char * scopeName = "mapBanks" ; };
template<> struct LibraryContainerKey<LibraryMapObstacle   > { static constexpr const char * scopeName = "mapObstacles"; };
template<> struct LibraryContainerKey<LibraryObjectDef     > { static constexpr const char * scopeName = "objectDefs" ; };
template<> struct LibraryContainerKey<LibraryResource      > { static constexpr const char * scopeName = "resources" ; };
template<> struct LibraryContainerKey<LibrarySecondarySkill> { static constexpr const char * scopeName = "skills" ; };
template<> struct LibraryContainerKey<LibrarySpell         > { static constexpr const char * scopeName = "spells" ; };
template<> struct LibraryContainerKey<LibraryTerrain       > { static constexpr const char * scopeName = "terrains" ; };
template<> struct LibraryContainerKey<LibraryUnit          > { static constexpr const char * scopeName = "units" ; };
// clang-format on
}

struct GameDatabase::Impl {
    template<class T>
    struct LibraryContainer : public ContainerInterface<T> {
        std::unordered_map<std::string, size_t> m_index;
        std::deque<T>                           m_objects;
        std::vector<T*>                         m_unsorted;
        std::vector<const T*>                   m_sorted;

        const T* find(const std::string& id) const override
        {
            auto it = m_index.find(id);
            return it == m_index.cend() ? nullptr : &m_objects[it->second];
        }
        T* findMutable(const std::string& id)
        {
            auto it = m_index.find(id);
            return it == m_index.end() ? nullptr : &m_objects[it->second];
        }
        const std::vector<const T*>& records() const override
        {
            return m_sorted;
        }
        std::vector<std::string> legacyOrderedIds() const override
        {
            const auto               records = legacyOrderedRecords();
            std::vector<std::string> result(records.size());
            for (size_t i = 0; i < records.size(); ++i)
                result[i] = records[i]->id;
            return result;
        }

        std::vector<const T*> legacyOrderedRecords() const override
        {
            std::map<int, const T*> resultMap;
            int                     maxId = 0;
            for (const T* obj : m_sorted) {
                if (obj->legacyId < 0)
                    continue;
                if (resultMap[obj->legacyId]) {
                    assert(0);
                    throw std::runtime_error("all legacyIds must be unique");
                }
                resultMap[obj->legacyId] = obj;
                maxId                    = std::max(maxId, obj->legacyId);
            }
            std::vector<const T*> result(maxId + 1);
            for (auto& [key, id] : resultMap)
                result[key] = id;
            return result;
        }

        T& insertObject(std::string id, T&& value)
        {
            m_objects.push_back(std::move(value));
            T& objRecord = m_objects.back();
            objRecord.id = id;

            m_index[id] = m_objects.size() - 1;
            m_unsorted.push_back(&objRecord);
            return objRecord;
        }
        T& insertObject(const std::string& id)
        {
            return insertObject(id, {});
        }

        bool loadRecordList(const IGameDatabase* idResolver, const PropertyTree& recordListMap)
        {
            if (!recordListMap.contains(LibraryContainerKey<T>::scopeName))
                return true;
            const PropertyTreeMap& recordList = recordListMap[LibraryContainerKey<T>::scopeName].getMap();
            for (auto it = recordList.cbegin(); it != recordList.cend(); ++it) {
                const std::string& id      = it->first;
                const auto&        jsonObj = it->second;
                if (jsonObj.isNull())
                    continue;

                auto* objRecord = findMutable(id);
                assert(objRecord);

                if (!Reflection::deserialize(idResolver, *objRecord, jsonObj))
                    return false;
            }
            return true;
        }

        void prepareObjectKeys(const PropertyTree& recordListMap)
        {
            if (!recordListMap.contains(LibraryContainerKey<T>::scopeName))
                return;

            const PropertyTreeMap& recordList = recordListMap[LibraryContainerKey<T>::scopeName].getMap();
            m_index.clear();
            m_objects.clear();
            m_unsorted.clear();
            m_sorted.clear();
            for (auto it = recordList.cbegin(); it != recordList.cend(); ++it) {
                const std::string& id      = it->first;
                const auto&        jsonObj = it->second;

                if (jsonObj.isNull())
                    continue;

                auto& objRecord = insertObject(id);
                objRecord.id    = id;
            }
            m_sorted.reserve(m_unsorted.size());
        }
    };
    LibraryContainer<LibraryArtifact>       m_artifacts;
    LibraryContainer<LibraryDwelling>       m_dwellings;
    LibraryContainer<LibraryFaction>        m_factions;
    LibraryContainer<LibraryHero>           m_heroes;
    LibraryContainer<LibraryHeroSpec>       m_heroSpecs;
    LibraryContainer<LibraryMapBank>        m_mapBanks;
    LibraryContainer<LibraryMapObstacle>    m_mapObstacles;
    LibraryContainer<LibraryObjectDef>      m_objectDefs;
    LibraryContainer<LibraryResource>       m_resources;
    LibraryContainer<LibrarySecondarySkill> m_skills;
    LibraryContainer<LibrarySpell>          m_spells;
    LibraryContainer<LibraryTerrain>        m_terrains;
    LibraryContainer<LibraryUnit>           m_units;

    LibraryGameRules m_gameRules;

    template<typename T>
    LibraryContainer<T>& getContainer()
    {
        static_assert(sizeof(T) == 3);
    }
    template<typename T>
    T* findMutable(const T* obj)
    {
        return getContainer<T>().findMutable(obj->id);
    }
};
template<>
GameDatabase::Impl::LibraryContainer<LibraryArtifact>& GameDatabase::Impl::getContainer<LibraryArtifact>()
{
    return m_artifacts;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryDwelling>& GameDatabase::Impl::getContainer<LibraryDwelling>()
{
    return m_dwellings;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryFaction>& GameDatabase::Impl::getContainer<LibraryFaction>()
{
    return m_factions;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryHero>& GameDatabase::Impl::getContainer<LibraryHero>()
{
    return m_heroes;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryHeroSpec>& GameDatabase::Impl::getContainer<LibraryHeroSpec>()
{
    return m_heroSpecs;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryMapBank>& GameDatabase::Impl::getContainer<LibraryMapBank>()
{
    return m_mapBanks;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryMapObstacle>& GameDatabase::Impl::getContainer<LibraryMapObstacle>()
{
    return m_mapObstacles;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryObjectDef>& GameDatabase::Impl::getContainer<LibraryObjectDef>()
{
    return m_objectDefs;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryResource>& GameDatabase::Impl::getContainer<LibraryResource>()
{
    return m_resources;
}
template<>
GameDatabase::Impl::LibraryContainer<LibrarySecondarySkill>& GameDatabase::Impl::getContainer<LibrarySecondarySkill>()
{
    return m_skills;
}
template<>
GameDatabase::Impl::LibraryContainer<LibrarySpell>& GameDatabase::Impl::getContainer<LibrarySpell>()
{
    return m_spells;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryTerrain>& GameDatabase::Impl::getContainer<LibraryTerrain>()
{
    return m_terrains;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryUnit>& GameDatabase::Impl::getContainer<LibraryUnit>()
{
    return m_units;
}

GameDatabase::GameDatabase(const std::vector<Resource>& resourceFiles)
    : m_impl(std::make_unique<Impl>())
{
    load(resourceFiles); // @todo: exception throw??
}

GameDatabase::GameDatabase(const std::string& dataBaseId, const IResourceLibrary* resourceLibrary)
    : GameDatabase(loadLibrary(dataBaseId, resourceLibrary))
{
}

GameDatabase::~GameDatabase()
{
}

IGameDatabase::LibraryArtifactContainerPtr GameDatabase::artifacts() const
{
    return &m_impl->m_artifacts;
}
IGameDatabase::LibraryDwellingContainerPtr GameDatabase::dwellings() const
{
    return &m_impl->m_dwellings;
}
IGameDatabase::LibraryFactionContainerPtr GameDatabase::factions() const
{
    return &m_impl->m_factions;
}
IGameDatabase::LibraryHeroContainerPtr GameDatabase::heroes() const
{
    return &m_impl->m_heroes;
}
IGameDatabase::LibraryHeroSpecContainerPtr GameDatabase::heroSpecs() const
{
    return &m_impl->m_heroSpecs;
}
IGameDatabase::LibraryMapBankContainerPtr GameDatabase::mapBanks() const
{
    return &m_impl->m_mapBanks;
}
IGameDatabase::LibraryMapObstacleContainerPtr GameDatabase::mapObstacles() const
{
    return &m_impl->m_mapObstacles;
}
IGameDatabase::LibraryObjectDefContainerPtr GameDatabase::objectDefs() const
{
    return &m_impl->m_objectDefs;
}
IGameDatabase::LibraryResourceContainerPtr GameDatabase::resources() const
{
    return &m_impl->m_resources;
}
IGameDatabase::LibrarySecondarySkillContainerPtr GameDatabase::secSkills() const
{
    return &m_impl->m_skills;
}
IGameDatabase::LibrarySpellContainerPtr GameDatabase::spells() const
{
    return &m_impl->m_spells;
}
IGameDatabase::LibraryTerrainContainerPtr GameDatabase::terrains() const
{
    return &m_impl->m_terrains;
}
IGameDatabase::LibraryUnitContainerPtr GameDatabase::units() const
{
    return &m_impl->m_units;
}

LibraryGameRulesConstPtr GameDatabase::gameRules() const
{
    return &m_impl->m_gameRules;
}

std::vector<GameDatabase::Resource> GameDatabase::loadLibrary(const std::string& dataBaseId, const IResourceLibrary* resourceLibrary)
{
    ProfilerScope                       scope("GameDatabase::loadLibrary");
    std::vector<GameDatabase::Resource> files;
    const ResourceDatabase&             desc = resourceLibrary->getDatabase(dataBaseId);
    for (auto& f : desc.filesFullPathsWithDeps) {
        std::string    buffer;
        const std_path path{ string2path(f) };
        {
            if (!readFileIntoBuffer(path, buffer)) {
                Logger(Logger::Warning) << "Skipped file unable to read:" << f << " (exist:" << std_fs::exists(path) << ")";
                continue;
            }
        }
        PropertyTree tree;
        {
            if (!readJsonFromBuffer(buffer, tree)) {
                Logger(Logger::Warning) << "Skipped invalid json file:" << f;
                continue;
            }
        }
        files.push_back({ std::move(tree), path2string(path.filename()) });
    }
    return files;
}

bool GameDatabase::load(const std::vector<Resource>& resourceFiles)
{
    Logger(Logger::Info) << "Preparing to load GameDatabase, total files:" << resourceFiles.size();
    PropertyTree recordObjectMaps;
    recordObjectMaps.convertToMap();
    for (const auto& resourceFile : resourceFiles) {
        const int totalRecordsFound = addJsonObjectToIndex(recordObjectMaps, resourceFile.m_jsonData);
        Logger(Logger::Info) << "Database JSON parsing finished: " << resourceFile.m_filename << ", total records:" << totalRecordsFound;
    }

    // clang-format off
    m_impl->m_artifacts    .prepareObjectKeys(recordObjectMaps);
    m_impl->m_dwellings    .prepareObjectKeys(recordObjectMaps);
    m_impl->m_factions     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_heroSpecs    .prepareObjectKeys(recordObjectMaps);
    m_impl->m_heroes       .prepareObjectKeys(recordObjectMaps);
    m_impl->m_mapBanks     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_mapObstacles .prepareObjectKeys(recordObjectMaps);
    m_impl->m_objectDefs   .prepareObjectKeys(recordObjectMaps);
    m_impl->m_resources    .prepareObjectKeys(recordObjectMaps);
    m_impl->m_skills       .prepareObjectKeys(recordObjectMaps);
    m_impl->m_spells       .prepareObjectKeys(recordObjectMaps);
    m_impl->m_terrains     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_units        .prepareObjectKeys(recordObjectMaps);
    // clang-format on

    // clang-format off
    const bool result =
               m_impl->m_artifacts    .loadRecordList(this, recordObjectMaps)
            && m_impl->m_dwellings    .loadRecordList(this, recordObjectMaps)
            && m_impl->m_factions     .loadRecordList(this, recordObjectMaps)
            && m_impl->m_heroSpecs    .loadRecordList(this, recordObjectMaps)
            && m_impl->m_heroes       .loadRecordList(this, recordObjectMaps)
            && m_impl->m_mapBanks     .loadRecordList(this, recordObjectMaps)
            && m_impl->m_mapObstacles .loadRecordList(this, recordObjectMaps)
            && m_impl->m_objectDefs   .loadRecordList(this, recordObjectMaps)
            && m_impl->m_resources    .loadRecordList(this, recordObjectMaps)
            && m_impl->m_skills       .loadRecordList(this, recordObjectMaps)
            && m_impl->m_spells       .loadRecordList(this, recordObjectMaps)
            && m_impl->m_terrains     .loadRecordList(this, recordObjectMaps)
            && m_impl->m_units        .loadRecordList(this, recordObjectMaps)
            ;
    // clang-format on

    if (!result)
        return false;

    if (!Reflection::deserialize(this, m_impl->m_gameRules, recordObjectMaps["gameRules"][""]))
        return false;

    // making object links.
    for (auto spec : m_impl->m_heroSpecs.m_unsorted) {
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
    for ([[maybe_unused]] auto unit : m_impl->m_units.m_unsorted) {
        assert(unit->faction); // actually missing faction is error
        assert(!unit->faction->id.empty());
    }
    for (auto hero : m_impl->m_heroes.m_unsorted) {
        assert(hero->faction);
        assert(hero->spec);

        for ([[maybe_unused]] auto& subSkill : hero->secondarySkills) {
            assert(subSkill.skill);
        }
        for ([[maybe_unused]] auto& unit : hero->startStacks) {
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
                    updates                 = true;
                }
            }
        }
        if (!updates)
            break;
    }

    std::sort(sortedFactions.begin(), sortedFactions.end(), [](LibraryFaction* l, LibraryFaction* r) {
        if (l->alignment != r->alignment)
            return l->alignment < r->alignment;
        return l->generatedOrder < r->generatedOrder;
    });

    int index = 0;
    for (auto faction : sortedFactions) {
        faction->generatedOrder = index++;
        for (auto* skill : m_impl->m_skills.m_unsorted) {
            assert(skill);
            if (!faction->mageClass.secondarySkillWeights.contains(skill))
                faction->mageClass.secondarySkillWeights[skill] = 0;
            if (!faction->warriorClass.secondarySkillWeights.contains(skill))
                faction->warriorClass.secondarySkillWeights[skill] = 0;
        }
        // @todo: WTF? it contains nullptr!
        if (faction->mageClass.secondarySkillWeights.contains(nullptr))
            faction->mageClass.secondarySkillWeights.erase(nullptr);
        if (faction->warriorClass.secondarySkillWeights.contains(nullptr))
            faction->warriorClass.secondarySkillWeights.erase(nullptr);

        if (faction->alignment != LibraryFaction::Alignment::Special) {
            assert(!faction->mageClass.lowLevelIncrease.empty());
            assert(!faction->mageClass.highLevelIncrease.empty());
            assert(!faction->warriorClass.lowLevelIncrease.empty());
            assert(!faction->warriorClass.highLevelIncrease.empty());
        }
        if (faction->alignment != LibraryFaction::Alignment::Special && faction->alignment != LibraryFaction::Alignment::Independent) {
            auto copyHeroDef = [this](const std::string& def) {
                auto& objRecord        = m_impl->m_objectDefs.insertObject(def + "e");
                objRecord              = *(m_impl->m_objectDefs.find("hero"));
                objRecord.id           = def + "e";
                objRecord.defFile      = objRecord.id;
                objRecord.terrainsHard = objRecord.terrainsSoft;
            };
            copyHeroDef(faction->mageClass.presentationParams.adventureSpriteMale);
            copyHeroDef(faction->mageClass.presentationParams.adventureSpriteFemale);
            copyHeroDef(faction->warriorClass.presentationParams.adventureSpriteMale);
            copyHeroDef(faction->warriorClass.presentationParams.adventureSpriteFemale);
        }

        m_impl->m_factions.m_sorted.push_back(faction);
    }

    // skills postproc
    std::sort(m_impl->m_skills.m_unsorted.begin(), m_impl->m_skills.m_unsorted.end(), [](auto* l, auto* r) {
        return l->presentationParams.order < r->presentationParams.order;
    });

    for (auto* skill : m_impl->m_skills.m_unsorted) {
        m_impl->m_skills.m_sorted.push_back(skill);
    }

    // spells postproc
    std::sort(m_impl->m_spells.m_unsorted.begin(), m_impl->m_spells.m_unsorted.end(), [](auto* l, auto* r) {
        if (l->level != r->level)
            return l->level < r->level;
        if (l->school != r->school)
            return l->school < r->school;
        return l->presentationParams.configOrder < r->presentationParams.configOrder;
    });
    index = 0;
    for (auto* spell : m_impl->m_spells.m_unsorted) {
        spell->presentationParams.order = index++;
        m_impl->m_spells.m_sorted.push_back(spell);
    }
    // additional autogenerated objects for artifacts.
    for (auto spell : m_impl->m_spells.m_unsorted) {
        if (!spell->isTeachable)
            continue;

        LibraryArtifact art;
        art.id                           = "sod.artifact." + spell->id;
        art.scrollSpell                  = spell;
        art.slot                         = ArtifactSlotType::Misc;
        art.untranslatedName             = "Scroll " + spell->untranslatedName;
        art.presentationParams.iconStash = art.presentationParams.iconBonus = spell->presentationParams.iconScroll;

        art.presentationParams.order         = spell->level * 1000 + spell->presentationParams.order;
        art.presentationParams.orderGroup    = 0;
        art.presentationParams.orderCategory = LibraryArtifact::OrderCategory::Scrolls;
        art.treasureClass                    = LibraryArtifact::TreasureClass::Scroll;
        art.mapObjectDef                     = m_impl->m_objectDefs.find("ava0001");
        m_impl->m_artifacts.insertObject(art.id, std::move(art));
    }

    // units postproc
    std::sort(m_impl->m_units.m_unsorted.begin(), m_impl->m_units.m_unsorted.end(), [](auto* l, auto* r) {
        if (l->faction != r->faction)
            return l->faction->generatedOrder < r->faction->generatedOrder;

        return l->level < r->level;
    });
    for (auto* unit : m_impl->m_units.m_unsorted) {
        for (auto* upgUnit : unit->upgrades) {
            m_impl->findMutable(upgUnit)->prevUpgrade = unit;
        }

        m_impl->m_units.m_sorted.push_back(unit);
        m_impl->findMutable(unit->faction)->units.push_back(unit);
    }
    for (auto* unit : m_impl->m_units.m_unsorted) {
        LibraryUnitConstPtr unitIt = unit;
        while (unitIt->prevUpgrade) {
            unitIt = unitIt->prevUpgrade;
        }
        unit->baseUpgrade = unitIt;
    }
    // spec postproc - do we need an order at all?
    for (auto* spec : m_impl->m_heroSpecs.m_unsorted) {
        m_impl->m_heroSpecs.m_sorted.push_back(spec);
    }
    // artifacts postproc
    std::sort(m_impl->m_artifacts.m_unsorted.begin(), m_impl->m_artifacts.m_unsorted.end(), [](auto* l, auto* r) {
        return l->sortOrdering() < r->sortOrdering();
    });
    for (auto* artifact : m_impl->m_artifacts.m_unsorted) {
        ArtifactSlotRequirement req;
        for (auto* setPart : artifact->parts) {
            assert(!setPart->partOfSet);
            assert(setPart->parts.empty());
            m_impl->findMutable(setPart)->partOfSet = artifact;

            req.add(setPart->slot);
        }
        if (artifact->parts.empty())
            req.add(artifact->slot);
        artifact->slotReq            = req;
        auto cache                   = artifact->provideSpells.filterPossible(m_impl->m_spells.m_unsorted);
        artifact->provideSpellsCache = std::set<LibrarySpellConstPtr>(cache.cbegin(), cache.cend());

        m_impl->m_artifacts.m_sorted.push_back(artifact);
    }
    // heroes postproc
    std::sort(m_impl->m_heroes.m_unsorted.begin(), m_impl->m_heroes.m_unsorted.end(), [](auto* l, auto* r) {
        if (l->faction != r->faction)
            return l->faction->generatedOrder < r->faction->generatedOrder;

        if (l->isWarrior != r->isWarrior)
            return l->isWarrior > r->isWarrior;
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto* hero : m_impl->m_heroes.m_unsorted) {
        m_impl->m_heroes.m_sorted.push_back(hero);
        m_impl->findMutable(hero->faction)->heroes.push_back(hero);
    }
    // resource postproc
    std::sort(m_impl->m_resources.m_unsorted.begin(), m_impl->m_resources.m_unsorted.end(), [](auto* l, auto* r) {
        return l->presentationParams.orderKingdom < r->presentationParams.orderKingdom;
    });
    for (auto* resource : m_impl->m_resources.m_unsorted) {
        m_impl->m_resources.m_sorted.push_back(resource);
    }
    // terrain postproc
    std::sort(m_impl->m_terrains.m_unsorted.begin(), m_impl->m_terrains.m_unsorted.end(), [](auto* l, auto* r) {
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto* terrain : m_impl->m_terrains.m_unsorted) {
        m_impl->m_terrains.m_sorted.push_back(terrain);
    }
    // mapBanks postproc
    std::sort(m_impl->m_mapBanks.m_unsorted.begin(), m_impl->m_mapBanks.m_unsorted.end(), [](auto* l, auto* r) {
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto* obj : m_impl->m_mapBanks.m_unsorted) {
        for (auto& variant : obj->variants) {
            for ([[maybe_unused]] auto& guard : variant.guards) {
                assert(guard.unit);
            }
        }
        m_impl->m_mapBanks.m_sorted.push_back(obj);
    }
    for (auto* obj : m_impl->m_mapObstacles.m_unsorted) {
        m_impl->m_mapObstacles.m_sorted.push_back(obj);
    }
    for (auto* d : m_impl->m_dwellings.m_unsorted) {
        m_impl->m_dwellings.m_sorted.push_back(d);
    }
    for (auto* d : m_impl->m_objectDefs.m_unsorted) {
        m_impl->m_objectDefs.m_sorted.push_back(d);
    }

    return true;
}

}
