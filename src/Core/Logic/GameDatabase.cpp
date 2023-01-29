/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GameDatabase.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryBuilding.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryGameRules.hpp"
#include "LibraryHero.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryResource.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryUnit.hpp"

#include "LibrarySerialize.hpp"

#include "MernelPlatform/Logger.hpp"

#include <deque>
#include <unordered_map>
#include <cstddef>
#include <stdexcept>

namespace FreeHeroes::Core {
using namespace Mernel;
namespace {
template<class T>
struct LibraryContainerKey {
    static constexpr const void** scopeName = nullptr;
};

// clang-format off
template<> struct LibraryContainerKey<LibraryArtifact      > { static constexpr const char * scopeName = "artifacts" ; };
template<> struct LibraryContainerKey<LibraryBuilding      > { static constexpr const char * scopeName = "buildings" ; };
template<> struct LibraryContainerKey<LibraryDwelling      > { static constexpr const char * scopeName = "dwellings" ; };
template<> struct LibraryContainerKey<LibraryFaction       > { static constexpr const char * scopeName = "factions" ; };
template<> struct LibraryContainerKey<LibraryHero          > { static constexpr const char * scopeName = "heroes" ; };
template<> struct LibraryContainerKey<LibraryHeroSpec      > { static constexpr const char * scopeName = "specialities" ; };
template<> struct LibraryContainerKey<LibraryMapBank       > { static constexpr const char * scopeName = "mapBanks" ; };
template<> struct LibraryContainerKey<LibraryMapObstacle   > { static constexpr const char * scopeName = "mapObstacles"; };
template<> struct LibraryContainerKey<LibraryMapVisitable  > { static constexpr const char * scopeName = "mapVisitables"; };
template<> struct LibraryContainerKey<LibraryObjectDef     > { static constexpr const char * scopeName = "objectDefs" ; };
template<> struct LibraryContainerKey<LibraryPlayer        > { static constexpr const char * scopeName = "players" ; };
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
                result[i] = records[i] ? records[i]->id : "";
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

        void loadRecordList(const IGameDatabase* idResolver, const PropertyTree& recordListMap)
        {
            if (!recordListMap.contains(LibraryContainerKey<T>::scopeName))
                return;
            const PropertyTreeMap& recordList = recordListMap[LibraryContainerKey<T>::scopeName].getMap();
            for (auto it = recordList.cbegin(); it != recordList.cend(); ++it) {
                const std::string& id      = it->first;
                const auto&        jsonObj = it->second;
                if (jsonObj.isNull())
                    continue;

                auto* objRecord = findMutable(id);
                assert(objRecord);

                deserialize(idResolver, *objRecord, jsonObj);
            }
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
    LibraryContainer<LibraryBuilding>       m_buildings;
    LibraryContainer<LibraryDwelling>       m_dwellings;
    LibraryContainer<LibraryFaction>        m_factions;
    LibraryContainer<LibraryHero>           m_heroes;
    LibraryContainer<LibraryHeroSpec>       m_heroSpecs;
    LibraryContainer<LibraryMapBank>        m_mapBanks;
    LibraryContainer<LibraryMapObstacle>    m_mapObstacles;
    LibraryContainer<LibraryMapVisitable>   m_mapVisitables;
    LibraryContainer<LibraryObjectDef>      m_objectDefs;
    LibraryContainer<LibraryPlayer>         m_players;
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
        assert(obj);
        return getContainer<T>().findMutable(obj->id);
    }
};
template<>
GameDatabase::Impl::LibraryContainer<LibraryArtifact>& GameDatabase::Impl::getContainer<LibraryArtifact>()
{
    return m_artifacts;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryBuilding>& GameDatabase::Impl::getContainer<LibraryBuilding>()
{
    return m_buildings;
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
GameDatabase::Impl::LibraryContainer<LibraryMapVisitable>& GameDatabase::Impl::getContainer<LibraryMapVisitable>()
{
    return m_mapVisitables;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryObjectDef>& GameDatabase::Impl::getContainer<LibraryObjectDef>()
{
    return m_objectDefs;
}
template<>
GameDatabase::Impl::LibraryContainer<LibraryPlayer>& GameDatabase::Impl::getContainer<LibraryPlayer>()
{
    return m_players;
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

IGameDatabase::LibraryArtifactContainerPtr GameDatabase::artifacts() const
{
    return &m_impl->m_artifacts;
}
IGameDatabase::LibraryBuildingContainerPtr GameDatabase::buildings() const
{
    return &m_impl->m_buildings;
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
IGameDatabase::LibraryMapVisitableContainerPtr GameDatabase::mapVisitables() const
{
    return &m_impl->m_mapVisitables;
}
IGameDatabase::LibraryPlayerContainerPtr GameDatabase::players() const
{
    return &m_impl->m_players;
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

GameDatabase::GameDatabase(const Mernel::PropertyTree& recordObjectMaps)
    : m_impl(std::make_unique<Impl>())
{
    // clang-format off
    m_impl->m_artifacts      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_buildings      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_dwellings      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_factions       .prepareObjectKeys(recordObjectMaps);
    m_impl->m_heroSpecs      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_heroes         .prepareObjectKeys(recordObjectMaps);
    m_impl->m_mapBanks       .prepareObjectKeys(recordObjectMaps);
    m_impl->m_mapObstacles   .prepareObjectKeys(recordObjectMaps);
    m_impl->m_mapVisitables  .prepareObjectKeys(recordObjectMaps);
    m_impl->m_objectDefs     .prepareObjectKeys(recordObjectMaps);
    m_impl->m_players        .prepareObjectKeys(recordObjectMaps);
    m_impl->m_resources      .prepareObjectKeys(recordObjectMaps);
    m_impl->m_skills         .prepareObjectKeys(recordObjectMaps);
    m_impl->m_spells         .prepareObjectKeys(recordObjectMaps);
    m_impl->m_terrains       .prepareObjectKeys(recordObjectMaps);
    m_impl->m_units          .prepareObjectKeys(recordObjectMaps);
    // clang-format on

    // clang-format off
    m_impl->m_artifacts      .loadRecordList(this, recordObjectMaps);
    m_impl->m_buildings      .loadRecordList(this, recordObjectMaps);
    m_impl->m_dwellings      .loadRecordList(this, recordObjectMaps);
    m_impl->m_factions       .loadRecordList(this, recordObjectMaps);
    m_impl->m_heroSpecs      .loadRecordList(this, recordObjectMaps);
    m_impl->m_heroes         .loadRecordList(this, recordObjectMaps);
    m_impl->m_mapBanks       .loadRecordList(this, recordObjectMaps);
    m_impl->m_mapObstacles   .loadRecordList(this, recordObjectMaps);
    m_impl->m_mapVisitables  .loadRecordList(this, recordObjectMaps);
    m_impl->m_objectDefs     .loadRecordList(this, recordObjectMaps);
    m_impl->m_players        .loadRecordList(this, recordObjectMaps);
    m_impl->m_resources      .loadRecordList(this, recordObjectMaps);
    m_impl->m_skills         .loadRecordList(this, recordObjectMaps);
    m_impl->m_spells         .loadRecordList(this, recordObjectMaps);
    m_impl->m_terrains       .loadRecordList(this, recordObjectMaps);
    m_impl->m_units          .loadRecordList(this, recordObjectMaps);
    // clang-format on

    deserialize(this, m_impl->m_gameRules, recordObjectMaps["gameRules"][""]);

    auto checkLink = []<typename T>(const auto* id, const T* context) {
        if (id)
            return;
        throw std::runtime_error("Link check violated, id == nullptr, scope '" + std::string(LibraryContainerKey<T>::scopeName) + "', context=" + context->id);
    };

    // making object links.
    for (auto spec : m_impl->m_heroSpecs.m_unsorted) {
        if (spec->type == LibraryHeroSpec::Type::Unit
            || spec->type == LibraryHeroSpec::Type::UnitUpgrade
            || spec->type == LibraryHeroSpec::Type::UnitNonStd
            || spec->type == LibraryHeroSpec::Type::SpecialCannon
            || spec->type == LibraryHeroSpec::Type::SpecialBallista) {
            checkLink(spec->unit, spec);
        }
        if (spec->type == LibraryHeroSpec::Type::Skill) {
            checkLink(spec->skill, spec);
        }
        if (spec->type == LibraryHeroSpec::Type::Spell) {
            checkLink(spec->spell, spec);
        }
    }
    for (auto* unit : m_impl->m_units.m_unsorted) {
        checkLink(unit->faction, unit); // actually missing faction is error
        assert(!unit->faction->id.empty());
    }
    for (auto* hero : m_impl->m_heroes.m_unsorted) {
        checkLink(hero->faction, hero);
        checkLink(hero->spec, hero);

        for ([[maybe_unused]] auto& subSkill : hero->secondarySkills) {
            checkLink(subSkill.skill, hero);
        }
        for ([[maybe_unused]] auto& unit : hero->startStacks) {
            checkLink(unit.unit, hero);
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
    for (auto* faction : sortedFactions) {
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

        for (const auto& [key, objConst] : faction->objectDefs.variants) {
            checkLink(objConst, faction);
            auto* objMutable                 = m_impl->findMutable(objConst);
            objMutable->mappings.factionTown = faction;
            assert(objMutable->mappings.key.empty());
            objMutable->mappings.key = key;
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
        art.objectDefs                       = ObjectDefMappings{ .variants = { { "", m_impl->m_objectDefs.find("ava0001") } } };
        art.value                            = 0;
        art.cost                             = 2000;
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

        for (const auto& [key, objConst] : unit->objectDefs.variants) {
            checkLink(objConst, unit);
            auto* objMutable          = m_impl->findMutable(objConst);
            objMutable->mappings.unit = unit;
            assert(objMutable->mappings.key.empty());
            objMutable->mappings.key = key;
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

        for (const auto& [key, objConst] : artifact->objectDefs.variants) {
            checkLink(objConst, artifact);
            auto* obj              = m_impl->findMutable(objConst);
            obj->mappings.artifact = artifact;
            assert(obj->mappings.key.empty());
            obj->mappings.key = key;
        }
        if (artifact->value == -1) {
            assert(artifact->statBonus.nonEmptyAmount());
            const int statBouns = artifact->statBonus.ad.attack + artifact->statBonus.ad.defense + artifact->statBonus.magic.intelligence + artifact->statBonus.magic.spellPower;
            artifact->value     = statBouns * 1000;
        }
        if (artifact->cost == -1)
            artifact->cost = artifact->value;
        if (artifact->guard == -1)
            artifact->guard = artifact->value * 2;

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
        for (const auto& [key, objConst] : resource->objectDefs.variants) {
            checkLink(objConst, resource);
            auto* obj              = m_impl->findMutable(objConst);
            obj->mappings.resource = resource;
            assert(obj->mappings.key.empty());
            obj->mappings.key = key;
        }
        for (const auto& [key, objConst] : resource->minesDefs.variants) {
            checkLink(objConst, resource);
            auto* obj = m_impl->findMutable(objConst);
            assert(obj->mappings.key.empty());
            obj->mappings.resourceMine = resource;
            obj->mappings.key          = key;
        }

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
            for (auto& guard : variant.guards) {
                checkLink(guard.unit, obj);
            }
        }
        for (const auto& [key, objConst] : obj->objectDefs.variants) {
            checkLink(objConst, obj);
            auto* objMutable             = m_impl->findMutable(objConst);
            objMutable->mappings.mapBank = obj;
            assert(objMutable->mappings.key.empty());
            objMutable->mappings.key = key;
        }
        m_impl->m_mapBanks.m_sorted.push_back(obj);
    }
    for (auto* obj : m_impl->m_mapObstacles.m_unsorted) {
        for (const auto& [key, objConst] : obj->objectDefs.variants) {
            checkLink(objConst, obj);
            auto* objMutable                 = m_impl->findMutable(objConst);
            objMutable->mappings.mapObstacle = obj;
            assert(objMutable->mappings.key.empty());
            objMutable->mappings.key = key;
        }
        m_impl->m_mapObstacles.m_sorted.push_back(obj);
    }
    for (auto* obj : m_impl->m_mapVisitables.m_unsorted) {
        for (const auto& [key, objConst] : obj->objectDefs.variants) {
            checkLink(objConst, obj);
            auto* objMutable                  = m_impl->findMutable(objConst);
            objMutable->mappings.mapVisitable = obj;
            assert(objMutable->mappings.key.empty());
            objMutable->mappings.key = key;
        }
        m_impl->m_mapVisitables.m_sorted.push_back(obj);
    }
    for (auto* obj : m_impl->m_dwellings.m_unsorted) {
        for (const auto& [key, objConst] : obj->objectDefs.variants) {
            checkLink(objConst, obj);
            auto* objMutable              = m_impl->findMutable(objConst);
            objMutable->mappings.dwelling = obj;
            assert(objMutable->mappings.key.empty());
            objMutable->mappings.key = key;
        }
        m_impl->m_dwellings.m_sorted.push_back(obj);
    }
    // players postproc
    std::sort(m_impl->m_players.m_unsorted.begin(), m_impl->m_players.m_unsorted.end(), [](auto* l, auto* r) {
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto* obj : m_impl->m_players.m_unsorted) {
        m_impl->m_players.m_sorted.push_back(obj);
    }
    // players postproc
    std::sort(m_impl->m_buildings.m_unsorted.begin(), m_impl->m_buildings.m_unsorted.end(), [](auto* l, auto* r) {
        return l->presentationParams.order < r->presentationParams.order;
    });
    for (auto* obj : m_impl->m_buildings.m_unsorted) {
        m_impl->m_buildings.m_sorted.push_back(obj);
    }
    const size_t terrainsSize   = m_impl->m_terrains.legacyOrderedRecords().size();
    auto         makePlanarMask = [](const std::vector<uint8_t>& data, bool invert) -> LibraryObjectDef::PlanarMask {
        LibraryObjectDef::PlanarMask res;
        for (size_t y = 0; y < 6; ++y) {
            for (size_t x = 0; x < 8; ++x) {
                uint8_t bit = data[y * 8 + x] ^ uint8_t(invert);
                if (bit) {
                    res.width  = std::max(res.width, x + 1);
                    res.height = std::max(res.height, y + 1);
                }
            }
        }
        res.data.resize(res.height);
        for (auto& row : res.data)
            row.resize(res.width);
        for (size_t x = 0; x < 8; ++x) {
            for (size_t y = 0; y < 6; ++y) {
                uint8_t bit = data[y * 8 + x] ^ uint8_t(invert);
                if (bit) {
                    res.data[res.height - y - 1][res.width - x - 1] = 1;
                }
            }
        }
        return res;
    };
    for (auto* obj : m_impl->m_objectDefs.m_unsorted) {
        if (!obj->substituteKey.empty()) {
            checkLink(obj->substituteFor, obj);
            m_impl->findMutable(obj->substituteFor)->substitutions[obj->substituteKey] = obj;
        }
        std::reverse(obj->terrainsHard.begin(), obj->terrainsHard.end());
        std::reverse(obj->terrainsSoft.begin(), obj->terrainsSoft.end());
        obj->terrainsHard.resize(terrainsSize);
        obj->terrainsSoft.resize(terrainsSize);
        std::reverse(obj->terrainsHard.begin(), obj->terrainsHard.end());
        std::reverse(obj->terrainsSoft.begin(), obj->terrainsSoft.end());

        obj->blockMapPlanar = makePlanarMask(obj->blockMap, true);
        obj->visitMapPlanar = makePlanarMask(obj->visitMap, false);

        for (auto* terrain : m_impl->m_terrains.m_unsorted) {
            int legacyId = terrain->legacyId;
            if (legacyId < 0)
                continue;
            if (obj->terrainsSoft.at(obj->terrainsSoft.size() - legacyId - 1))
                obj->terrainsSoftCache.insert(terrain);
        }

        m_impl->m_objectDefs.m_sorted.push_back(obj);
    }
}

GameDatabase::~GameDatabase()
{
}

}
