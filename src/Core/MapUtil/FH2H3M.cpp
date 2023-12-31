/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FH2H3M.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryBuilding.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryPlayer.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryTerrain.hpp"

#include "MernelPlatform/Logger.hpp"
#include "MernelPlatform/StringUtils.hpp"

#define assume(cond) \
    if (!(cond)) \
        throw std::runtime_error(#cond " - violated, invalid FH map provided.");

namespace FreeHeroes {

class FH2H3MConverter::ObjectTemplateCache {
    uint32_t add(ObjectTemplate def, Core::LibraryObjectDefConstPtr record)
    {
        auto index = static_cast<uint32_t>(m_objectDefs.size());
        m_objectDefs.push_back(std::move(def));
        m_objectDefsLibrary.push_back(*record);
        return index;
    }

public:
    ObjectTemplateCache(const Core::IGameDatabase* database)
        : m_database(database)
    {}

    uint32_t add(Core::LibraryObjectDefConstPtr record)
    {
        assert(record);
        if (m_index.contains(record))
            return m_index.at(record);

        ObjectTemplate res = makeTemplateFromRecord(record);

        if (res.m_animationFile.find('.') == std::string::npos)
            res.m_animationFile += ".def";
        const size_t terrainsCount = m_database->terrains()->legacyOrderedIds().size();
        res.m_terrainsHard.resize(terrainsCount);
        res.m_terrainsSoft.resize(terrainsCount);

        const auto index = add(res, record);
        m_index[record]  = index;
        return index;
    }

    uint32_t addId(std::string defId)
    {
        auto* defsContainer = m_database->objectDefs();
        if (defId.ends_with(".def"))
            defId = defId.substr(0, defId.size() - 4);
        defId = Mernel::strToLower(defId);
        return add(defsContainer->find(defId));
    }

    uint32_t addHero(Core::LibraryHeroConstPtr hero, bool onBoat)
    {
        std::string id = hero->getAdventureSpriteForMap(onBoat);
        return addId(std::move(id));
    }

    ObjectTemplate makeTemplateFromRecord(Core::LibraryObjectDefConstPtr record)
    {
        ObjectTemplate res{
            .m_animationFile = record->defFile,
            .m_blockMask     = record->blockMap,
            .m_visitMask     = record->visitMap,
            .m_terrainsHard  = record->terrainsHard,
            .m_terrainsSoft  = record->terrainsSoft,

            .m_id           = static_cast<uint32_t>(record->objId),
            .m_subid        = static_cast<uint32_t>(record->subId),
            .m_type         = static_cast<ObjectTemplate::Type>(record->type),
            .m_drawPriority = static_cast<uint8_t>(record->priority),
        };
        if (!Mernel::strToLower(res.m_animationFile).ends_with(".def"))
            res.m_animationFile += ".def";
        return res;
    }

    void init(const std::vector<Core::LibraryObjectDef>& fhDefs)
    {
        m_objectDefsLibrary = fhDefs;
        m_objectDefs.resize(fhDefs.size());
        for (size_t i = 0; i < fhDefs.size(); ++i) {
            Core::LibraryObjectDefConstPtr embedded = &fhDefs[i];
            m_objectDefs[i]                         = makeTemplateFromRecord(embedded);
            if (embedded->substituteFor) {
                m_index[embedded->substituteFor] = static_cast<uint32_t>(i);
            }
        }
    }

    std::map<Core::LibraryObjectDefConstPtr, uint32_t> m_index;
    std::vector<ObjectTemplate>                        m_objectDefs;
    std::vector<Core::LibraryObjectDef>                m_objectDefsLibrary;
    const Core::IGameDatabase* const                   m_database;
};

FH2H3MConverter::FH2H3MConverter(const Core::IGameDatabase* database)
    : m_database(database)
{
}

void FH2H3MConverter::convertMap(const FHMap& src, H3Map& dest) const
{
    dest              = {};
    dest.m_mapName    = src.m_name;
    dest.m_mapDescr   = src.m_descr;
    dest.m_difficulty = src.m_difficulty;

    assume(src.m_tileMap.m_width == src.m_tileMap.m_height && src.m_tileMap.m_width > 0);

    dest.m_format                 = static_cast<MapFormat>(src.m_format);
    dest.m_anyPlayers             = src.m_anyPlayers;
    dest.m_tiles.m_size           = src.m_tileMap.m_width;
    dest.m_tiles.m_hasUnderground = src.m_tileMap.m_depth > 1;
    dest.m_tiles.updateSize();

    dest.m_hotaVer.m_ver1 = src.m_config.m_hotaVersion.m_ver1;
    dest.m_hotaVer.m_ver2 = src.m_config.m_hotaVersion.m_ver2;
    dest.m_hotaVer.m_ver3 = src.m_config.m_hotaVersion.m_ver3;

    dest.updateFeatures();
    dest.prepareArrays();

    dest.m_hotaVer.m_allowSpecialWeeks = src.m_config.m_allowSpecialWeeks;
    dest.m_hotaVer.m_roundLimit        = src.m_config.m_hasRoundLimit ? src.m_config.m_roundLimit : 0xffffffffU;
    dest.m_levelLimit                  = static_cast<uint8_t>(src.m_config.m_levelLimit);

    {
        auto& srcCond                 = src.m_victoryCondition;
        auto& destCond                = dest.m_victoryCondition;
        destCond.m_type               = static_cast<decltype(destCond.m_type)>(srcCond.m_type);
        destCond.m_allowNormalVictory = srcCond.m_allowNormalVictory;
        destCond.m_appliesToAI        = srcCond.m_appliesToAI;
        destCond.m_artID              = srcCond.m_artID ? static_cast<uint16_t>(srcCond.m_artID->legacyId) : 0;
        destCond.m_creatureID         = srcCond.m_creature.unit ? static_cast<uint16_t>(srcCond.m_creature.unit->legacyId) : 0;
        destCond.m_creatureCount      = static_cast<uint32_t>(srcCond.m_creature.count);
        destCond.m_resourceID         = srcCond.m_resourceID ? static_cast<uint8_t>(srcCond.m_resourceID->legacyId) : 0;
        destCond.m_resourceAmount     = srcCond.m_resourceAmount;
        destCond.m_pos                = int3fromPos(srcCond.m_pos);
        destCond.m_hallLevel          = srcCond.m_hallLevel;
        destCond.m_castleLevel        = srcCond.m_castleLevel;
        destCond.m_days               = srcCond.m_days;
    }
    {
        auto& srcCond   = src.m_lossCondition;
        auto& destCond  = dest.m_lossCondition;
        destCond.m_type = static_cast<decltype(destCond.m_type)>(srcCond.m_type);
        destCond.m_pos  = int3fromPos(srcCond.m_pos);
        destCond.m_days = srcCond.m_days;
    }

    for (auto& rumor : src.m_rumors)
        dest.m_rumors.push_back({ rumor.m_name, rumor.m_text });

    src.m_tileMap.eachPosTile([&dest](const FHPos& pos, const FHTileMap::Tile& tile, size_t) {
        auto& destTile       = dest.m_tiles.get(pos.m_x, pos.m_y, pos.m_z);
        destTile.m_terType   = static_cast<uint8_t>(tile.m_terrainId->legacyId);
        destTile.m_terView   = tile.m_terrainView.m_view;
        destTile.m_riverType = tile.m_riverType == FHRiverType::Invalid ? 0 : static_cast<uint8_t>(tile.m_riverType);
        destTile.m_riverDir  = tile.m_riverView.m_view;
        destTile.m_roadType  = tile.m_roadType == FHRoadType::Invalid ? 0 : static_cast<uint8_t>(tile.m_roadType);
        destTile.m_roadDir   = tile.m_roadView.m_view;

        destTile.m_flipHor  = tile.m_terrainView.m_flipHor;
        destTile.m_flipVert = tile.m_terrainView.m_flipVert;

        destTile.m_riverFlipHor  = tile.m_riverView.m_flipHor;
        destTile.m_riverFlipVert = tile.m_riverView.m_flipVert;

        destTile.m_roadFlipHor  = tile.m_roadView.m_flipHor;
        destTile.m_roadFlipVert = tile.m_roadView.m_flipVert;

        destTile.m_coastal = tile.m_coastal;
    });

    bool    hasTeams = false;
    uint8_t maxTeam  = 0;
    for (auto& [playerId, fhPlayer] : src.m_players) {
        hasTeams = hasTeams || fhPlayer.m_team >= 0;
    }
    for (auto& [playerId, fhPlayer] : src.m_players) {
        auto  index    = playerId->legacyId;
        auto& h3player = dest.m_players[index];

        h3player.m_canHumanPlay             = fhPlayer.m_humanPossible;
        h3player.m_canComputerPlay          = fhPlayer.m_aiPossible;
        h3player.m_generateHeroAtMainTown   = fhPlayer.m_generateHeroAtMainTown;
        h3player.m_hasRandomHero            = fhPlayer.m_hasRandomHero;
        h3player.m_isFactionRandom          = fhPlayer.m_isFactionRandom;
        h3player.m_unused1                  = fhPlayer.m_unused1;
        h3player.m_placeholder              = fhPlayer.m_placeholder;
        h3player.m_generatedHeroTownFaction = fhPlayer.m_generatedHeroTownFaction;
        h3player.m_mainCustomHeroPortrait   = fhPlayer.m_mainCustomHeroPortrait;
        h3player.m_mainCustomHeroName       = fhPlayer.m_mainCustomHeroName;
        h3player.m_aiTactic                 = static_cast<PlayerInfo::AiTactic>(fhPlayer.m_aiTactic);

        if (hasTeams) {
            if (fhPlayer.m_team < 0) {
                h3player.m_team = 0;
            } else {
                h3player.m_team = static_cast<uint8_t>(fhPlayer.m_team);
            }
            maxTeam = std::max(maxTeam, h3player.m_team);
        }
        for (const auto& fhHeroName : fhPlayer.m_heroesNames) {
            h3player.m_heroesNames.push_back({ .m_heroId = (uint8_t) fhHeroName.m_hero->legacyId, .m_heroName = fhHeroName.m_name });
        }

        uint16_t factionsBitmask = 0;
        for (Core::LibraryFactionConstPtr faction : fhPlayer.m_startingFactions) {
            assume(faction != nullptr);
            factionsBitmask |= 1U << uint32_t(faction->legacyId);
        }
        h3player.m_allowedFactionsBitmask = factionsBitmask;
    }
    if (hasTeams)
        dest.m_teamCount = maxTeam + 1;

    for (auto& bit : dest.m_allowedHeroes)
        bit = 1;

    ObjectTemplateCache tmplCache(m_database);
    tmplCache.init(src.m_objectDefs);

    tmplCache.addId("avwmrnd0");
    tmplCache.addId("avlholg0");

    auto addObject = [&dest, &tmplCache](const FHCommonObject& obj, std::shared_ptr<IMapObject> impl, auto defNumMaker, bool calcXoffset = false) {
        Object o;
        o.m_pos   = int3fromPos(obj.m_pos);
        o.m_order = obj.m_order;
        if (obj.m_defIndex.forcedIndex >= 0)
            o.m_defnum = static_cast<uint32_t>(obj.m_defIndex.forcedIndex);
        else
            o.m_defnum = defNumMaker();
        if (calcXoffset) {
            auto& def = tmplCache.m_objectDefsLibrary[o.m_defnum];

            auto blockMapPlanar = Core::LibraryObjectDef::makePlanarMask(def.blockMap, true);
            auto visitMapPlanar = Core::LibraryObjectDef::makePlanarMask(def.visitMap, false);
            auto combinedMask   = Core::LibraryObjectDef::makeCombinedMask(blockMapPlanar, visitMapPlanar);
            o.m_pos.m_x         = o.m_pos.m_x - combinedMask.m_visitable.begin()->m_x;
        }
        o.m_impl = std::move(impl);
        dest.m_objects.push_back(std::move(o));
    };
    auto addObjectCommon = [&addObject, &tmplCache](const auto& obj, std::shared_ptr<IMapObject> impl) {
        addObject(obj, std::move(impl), [&obj, &tmplCache] { return tmplCache.add(obj.m_id->objectDefs.get(obj.m_defIndex)); });
    };
    auto addObjectVisitable = [&addObject, &tmplCache](const auto& obj, std::shared_ptr<IMapObject> impl) {
        addObject(obj, std::move(impl), [&obj, &tmplCache] {
            return tmplCache.add(obj.m_visitableId ? obj.m_visitableId->objectDefs.get(obj.m_defIndex) : obj.m_fixedDef);
        });
    };

    for (auto& fhTown : src.m_towns) {
        auto playerIndex = fhTown.m_player->legacyId;
        if (playerIndex >= 0) {
            auto& h3player = dest.m_players[playerIndex];
            if (fhTown.m_isMain) {
                const int townGateOffset = 2;
                h3player.m_hasMainTown   = true;
                h3player.m_posOfMainTown = int3fromPos(fhTown.m_pos, -townGateOffset);
                //if (fhTown.m_factionId)
                //    h3player.m_generatedHeroTownFaction = static_cast<uint8_t>(fhTown.m_factionId->legacyId);
            }
        }
        auto cas1                  = std::make_unique<MapTown>();
        cas1->m_playerOwner        = playerIndex;
        cas1->m_hasFort            = fhTown.m_hasFort;
        cas1->m_questIdentifier    = fhTown.m_questIdentifier;
        cas1->m_spellResearch      = fhTown.m_spellResearch;
        cas1->m_alignment          = static_cast<uint8_t>(fhTown.m_alignment);
        cas1->m_hasCustomBuildings = fhTown.m_hasCustomBuildings;
        cas1->m_hasGarison         = fhTown.m_hasGarison;
        cas1->m_hasName            = fhTown.m_hasName;
        cas1->m_formation          = fhTown.m_groupedFormation;
        cas1->m_name               = fhTown.m_name;

        cas1->m_obligatorySpells = fhTown.m_obligatorySpells;
        cas1->m_possibleSpells   = fhTown.m_possibleSpells;

        //cas1->m_formation       = 0xCC;
        cas1->prepareArrays(dest.m_features.get());
        if (cas1->m_hasCustomBuildings) {
            for (auto* building : fhTown.m_buildings)
                cas1->m_builtBuildings[building->legacyId] = 1;
            for (auto* building : fhTown.m_forbiddenBuildings)
                cas1->m_forbiddenBuildings[building->legacyId] = 1;
        }
        for (auto& srcEvent : fhTown.m_events) {
            MapTownEvent destEvent;

            destEvent.m_name                         = srcEvent.m_name;
            destEvent.m_message                      = srcEvent.m_message;
            destEvent.m_resourceSet.m_resourceAmount = convertResources(srcEvent.m_resources);
            destEvent.m_players                      = srcEvent.m_players;
            destEvent.m_humanAffected                = srcEvent.m_humanAffected;
            destEvent.m_computerAffected             = srcEvent.m_computerAffected;
            destEvent.m_firstOccurence               = srcEvent.m_firstOccurence;
            destEvent.m_nextOccurence                = srcEvent.m_nextOccurence;
            destEvent.m_buildings                    = srcEvent.m_buildings;
            destEvent.m_creaturesAmounts             = srcEvent.m_creaturesAmounts;

            cas1->m_events.push_back(std::move(destEvent));
        }
        if (cas1->m_hasGarison) {
            convertSquad(fhTown.m_garison, cas1->m_garison);
        }

        if (fhTown.m_randomTown) {
            auto* def = fhTown.m_randomId;
            assert(def);

            addObject(fhTown, std::move(cas1), [&tmplCache, def] { return tmplCache.add(def); });
            continue;
        }
        auto* libraryFaction = fhTown.m_factionId;
        assert(libraryFaction);

        auto* def = libraryFaction->objectDefs.get(fhTown.m_defIndex);

        addObject(fhTown, std::move(cas1), [&tmplCache, def] { return tmplCache.add(def); });
    }

    for (auto& fhHero : src.m_wanderingHeroes) {
        auto  playerIndex = fhHero.m_player->legacyId;
        auto* libraryHero = fhHero.m_data.m_army.hero.library;

        const uint8_t heroId = libraryHero ? uint8_t(libraryHero->legacyId) : 0xFFU;

        if (playerIndex >= 0) {
            auto& h3player = dest.m_players[playerIndex];
            if (fhHero.m_isMain) {
                h3player.m_mainCustomHeroId = heroId;
            }
        }

        auto hero               = std::make_unique<MapHero>();
        hero->m_playerOwner     = playerIndex;
        hero->m_subID           = heroId;
        hero->m_questIdentifier = fhHero.m_questIdentifier;
        hero->m_formation       = fhHero.m_groupedFormation;
        hero->m_patrolRadius    = static_cast<uint8_t>(fhHero.m_patrolRadius);

        hero->prepareArrays(dest.m_features.get());
        {
            hero->m_hasExp                             = fhHero.m_data.m_hasExp;
            hero->m_hasSecSkills                       = fhHero.m_data.m_hasSecSkills;
            hero->m_primSkillSet.m_hasCustomPrimSkills = fhHero.m_data.m_hasPrimSkills;
            hero->m_spellSet.m_hasCustomSpells         = fhHero.m_data.m_hasSpells;
            hero->m_hasArmy                            = fhHero.m_data.m_hasArmy;
            hero->m_artSet.m_hasArts                   = fhHero.m_data.m_hasArts;
            hero->m_hasName                            = fhHero.m_data.m_hasName;
            hero->m_hasCustomBio                       = fhHero.m_data.m_hasCustomBio;
            hero->m_hasPortrait                        = fhHero.m_data.m_hasPortrait;
            if (hero->m_hasExp)
                hero->m_exp = static_cast<uint32_t>(fhHero.m_data.m_army.hero.experience);

            if (hero->m_hasName) {
                hero->m_name = fhHero.m_data.m_name;
            }
            if (hero->m_hasCustomBio) {
                hero->m_bio = fhHero.m_data.m_bio;
            }
            if (hero->m_hasPortrait) {
                hero->m_portrait = static_cast<uint8_t>(fhHero.m_data.m_portrait);
            }
            if (fhHero.m_data.m_sex != -1)
                hero->m_sex = static_cast<uint8_t>(fhHero.m_data.m_sex);

            if (hero->m_primSkillSet.m_hasCustomPrimSkills) {
                auto& prim = hero->m_primSkillSet.m_primSkills;
                prim.resize(4);
                std::tie(prim[0], prim[1]) = fhHero.m_data.m_army.hero.currentBasePrimary.ad.asTuple();
                std::tie(prim[2], prim[3]) = fhHero.m_data.m_army.hero.currentBasePrimary.magic.asTuple();
            }
            if (hero->m_hasSecSkills) {
                for (auto& sk : fhHero.m_data.m_army.hero.secondarySkills) {
                    hero->m_secSkills.push_back({ static_cast<uint8_t>(sk.skill->legacyId), static_cast<uint8_t>(sk.level + 1) });
                }
            }
            if (hero->m_spellSet.m_hasCustomSpells) {
                for (auto* spell : fhHero.m_data.m_army.hero.spellbook)
                    hero->m_spellSet.m_spells[spell->legacyId] = 1;
            }
            if (hero->m_hasArmy) {
                convertSquad(fhHero.m_data.m_army.squad, hero->m_garison);
            }
            if (hero->m_artSet.m_hasArts) {
                convertHeroArtifacts(fhHero.m_data.m_army.hero, hero->m_artSet);
            }
        }
        if (libraryHero)
            dest.m_allowedHeroes[heroId] = 0;
        if (playerIndex < 0) {
            addObject(fhHero, std::move(hero), [&tmplCache] { return tmplCache.addId("avxprsn0"); });
        } else if (fhHero.m_isRandom) {
            addObject(
                fhHero, std::move(hero), [&tmplCache] { return tmplCache.addId("ahrandom"); }, true);
        } else {
            bool onBoat = src.m_tileMap.get(fhHero.m_pos).m_terrainId->id == Core::LibraryTerrain::s_terrainWater;
            addObject(
                fhHero, std::move(hero), [&tmplCache, libraryHero, onBoat] { return tmplCache.addHero(libraryHero, onBoat); }, true);
        }
    }

    for (auto& allowed : dest.m_allowedHeroes)
        allowed = 1;
    for (auto& allowed : dest.m_allowedArtifacts)
        allowed = 1;
    for (auto& allowed : dest.m_allowedSpells)
        allowed = 1;
    for (auto& allowed : dest.m_allowedSecSkills)
        allowed = 1;

    for (auto* hero : src.m_placeholderHeroes) {
        dest.m_placeholderHeroes.push_back(static_cast<uint8_t>(hero->legacyId));
    }
    for (auto& srcHero : src.m_disposedHeroes) {
        DisposedHero destHero;
        destHero.prepareArrays(dest.m_features.get());
        for (auto* player : srcHero.m_players)
            destHero.m_players[player->legacyId] = 1;
        destHero.m_heroId   = static_cast<uint8_t>(srcHero.m_heroId->legacyId);
        destHero.m_portrait = static_cast<uint8_t>(srcHero.m_portrait);
        destHero.m_name     = srcHero.m_name;

        dest.m_disposedHeroes.push_back(std::move(destHero));
    }

    for (const auto heroId : m_database->heroes()->records()) {
        if (heroId->legacyId < 0)
            continue;
        if (!src.m_disabledHeroes.isDisabled(src.m_isWaterMap, heroId))
            continue;
        const auto legacyId = (heroId)->legacyId;
        if (legacyId < (int) dest.m_allowedHeroes.size())
            dest.m_allowedHeroes[legacyId] = 0;
    }

    if (src.m_format >= FHMap::MapFormat::HOTA1 && src.m_format <= FHMap::MapFormat::HOTA3) {
        // these artifact ids are just unexistent. @todo: do I really need this logic?
        dest.m_allowedArtifacts[145] = 1;
        dest.m_allowedArtifacts[144] = 1;
    }
    for (const auto artId : m_database->artifacts()->records()) {
        if (artId->legacyId < 0)
            continue;
        if (!src.m_disabledArtifacts.isDisabled(src.m_isWaterMap, artId))
            continue;
        const auto legacyId = (artId)->legacyId;
        if (legacyId < (int) dest.m_allowedArtifacts.size())
            dest.m_allowedArtifacts[legacyId] = 0;
    }
    for (const auto spellId : m_database->spells()->records()) {
        if (spellId->legacyId < 0)
            continue;
        if (!src.m_disabledSpells.isDisabled(src.m_isWaterMap, spellId))
            continue;
        const auto legacyId = (spellId)->legacyId;
        if (legacyId < (int) dest.m_allowedSpells.size())
            dest.m_allowedSpells[legacyId] = 0;
    }
    for (const auto secSkillId : m_database->secSkills()->records()) {
        if (secSkillId->legacyId < 0)
            continue;
        if (!src.m_disabledSkills.isDisabled(src.m_isWaterMap, secSkillId))
            continue;
        const auto legacyId               = (secSkillId)->legacyId;
        dest.m_allowedSecSkills[legacyId] = 0;
    }
    for (auto& customHero : src.m_customHeroes) {
        const auto legacyId = customHero.m_army.hero.library->legacyId;
        auto&      destHero = dest.m_customHeroData[legacyId];
        destHero.prepareArrays(dest.m_features.get());
        destHero.m_enabled                            = true;
        destHero.m_hasExp                             = customHero.m_hasExp;
        destHero.m_hasCustomBio                       = customHero.m_hasCustomBio;
        destHero.m_hasSkills                          = customHero.m_hasSecSkills;
        destHero.m_primSkillSet.m_hasCustomPrimSkills = customHero.m_hasPrimSkills;
        destHero.m_spellSet.m_hasCustomSpells         = customHero.m_hasSpells;
        destHero.m_artSet.m_hasArts                   = customHero.m_hasArts;
        if (customHero.m_sex != -1)
            destHero.m_sex = static_cast<uint8_t>(customHero.m_sex);
        if (customHero.m_hasPrimSkills) {
            auto& prim = destHero.m_primSkillSet.m_primSkills;
            prim.resize(4);
            std::tie(prim[0], prim[1]) = customHero.m_army.hero.currentBasePrimary.ad.asTuple();
            std::tie(prim[2], prim[3]) = customHero.m_army.hero.currentBasePrimary.magic.asTuple();
        }
        if (customHero.m_hasSecSkills) {
            for (auto& sk : customHero.m_army.hero.secondarySkills) {
                destHero.m_skills.push_back({ static_cast<uint8_t>(sk.skill->legacyId), static_cast<uint8_t>(sk.level + 1) });
            }
        }
        if (destHero.m_spellSet.m_hasCustomSpells) {
            for (auto* spell : customHero.m_army.hero.spellbook)
                destHero.m_spellSet.m_spells[spell->legacyId] = 1;
        }
        if (destHero.m_hasCustomBio) {
            destHero.m_bio = customHero.m_bio;
        }

        if (customHero.m_hasExp)
            destHero.m_exp = static_cast<uint32_t>(customHero.m_army.hero.experience);
        if (customHero.m_hasArts) {
            convertHeroArtifacts(customHero.m_army.hero, destHero.m_artSet);
        }
    }

    for (auto& fhRes : src.m_objects.m_resources) {
        auto res      = std::make_unique<MapResource>();
        res->m_amount = fhRes.m_amount / fhRes.m_id->pileSize;
        res->prepareArrays(dest.m_features.get());
        convertMessage(fhRes.m_messageWithBattle, res->m_message);

        addObjectCommon(fhRes, std::move(res));
    }

    for (auto& fhRes : src.m_objects.m_resourcesRandom) {
        auto res      = std::make_unique<MapResource>();
        res->m_amount = fhRes.m_amount;
        res->prepareArrays(dest.m_features.get());

        convertMessage(fhRes.m_messageWithBattle, res->m_message);
        addObject(fhRes, std::move(res), [&tmplCache]() { return tmplCache.addId("avtrndm0"); });
    }

    for (auto& fhArt : src.m_objects.m_artifacts) {
        auto art = std::make_unique<MapArtifact>(false);
        if (fhArt.m_id->scrollSpell) {
            art->m_spellId = fhArt.m_id->scrollSpell->legacyId;
            art->m_isSpell = true;
        }
        art->prepareArrays(dest.m_features.get());
        convertMessage(fhArt.m_messageWithBattle, art->m_message);

        addObjectCommon(fhArt, std::move(art));
    }
    for (auto& fhArt : src.m_objects.m_artifactsRandom) {
        auto art = std::make_unique<MapArtifact>(false);
        art->prepareArrays(dest.m_features.get());
        convertMessage(fhArt.m_messageWithBattle, art->m_message);
        std::string id = "";
        switch (fhArt.m_type) {
            case FHRandomArtifact::Type::Any:
                id = "avarand";
                break;
            case FHRandomArtifact::Type::Treasure:
                id = "avarnd1";
                break;
            case FHRandomArtifact::Type::Minor:
                id = "avarnd2";
                break;
            case FHRandomArtifact::Type::Major:
                id = "avarnd3";
                break;
            case FHRandomArtifact::Type::Relic:
                id = "avarnd4";
                break;
            case FHRandomArtifact::Type::Invalid:
                break;
        }

        addObject(fhArt, std::move(art), [&tmplCache, id]() { return tmplCache.addId(id); });
    }

    for (auto& fhMon : src.m_objects.m_monsters) {
        auto monster = std::make_unique<MapMonster>();
        monster->prepareArrays(dest.m_features.get());
        monster->m_count           = static_cast<uint16_t>(fhMon.m_count);
        monster->m_questIdentifier = fhMon.m_questIdentifier;
        if (fhMon.m_aggressionMax == fhMon.m_aggressionMin) {
            if (fhMon.m_aggressionMax == 0)
                monster->m_joinAppeal = 0;
            else if (fhMon.m_aggressionMax == 10)
                monster->m_joinAppeal = 4;
            else {
                monster->m_joinAppeal      = 5;
                monster->m_aggressionExact = fhMon.m_aggressionMax;
            }
        } else if (fhMon.m_aggressionMin == 1 && fhMon.m_aggressionMax == 7) {
            monster->m_joinAppeal = 1;
        } else if (fhMon.m_aggressionMin == 1 && fhMon.m_aggressionMax == 10) {
            monster->m_joinAppeal = 2;
        } else if (fhMon.m_aggressionMin == 4 && fhMon.m_aggressionMax == 10) {
            monster->m_joinAppeal = 3;
        } else {
            throw std::runtime_error("unsupported monster appeal");
        }
        monster->m_joinOnlyForMoney = fhMon.m_joinOnlyForMoney;
        monster->m_joinPercent      = fhMon.m_joinPercent;
        monster->m_neverFlees       = fhMon.m_neverFlees;
        monster->m_notGrowingTeam   = fhMon.m_notGrowingTeam;
        if (fhMon.m_upgradedStack == FHMonster::UpgradedStack::No)
            monster->m_upgradedStack = 0;
        if (fhMon.m_upgradedStack == FHMonster::UpgradedStack::Yes)
            monster->m_upgradedStack = 1;

        if (fhMon.m_splitStackType == FHMonster::SplitStack::Exact)
            monster->m_splitStack = static_cast<uint32_t>(fhMon.m_splitStackExact);
        else if (fhMon.m_splitStackType == FHMonster::SplitStack::Average)
            monster->m_splitStack = 4294967293U;
        else if (fhMon.m_splitStackType == FHMonster::SplitStack::OneMore)
            monster->m_splitStack = 0;
        else if (fhMon.m_splitStackType == FHMonster::SplitStack::OneLess)
            monster->m_splitStack = 4294967294U;

        monster->m_artID = uint16_t(-1);

        monster->m_hasMessage = fhMon.m_hasMessage;
        if (monster->m_hasMessage) {
            monster->m_message                      = fhMon.m_message;
            monster->m_resourceSet.m_resourceAmount = convertResources(fhMon.m_reward.resources);
            if (!fhMon.m_reward.artifacts.empty()) {
                monster->m_artID = uint16_t(fhMon.m_reward.artifacts[0].onlyArtifacts.at(0)->legacyId);
            }
        }
        if (fhMon.m_randomLevel >= 0) {
            static const std::vector<std::string> s_randomIds{
                "avwmrnd0",
                "avwmon1",
                "avwmon2",
                "avwmon3",
                "avwmon4",
                "avwmon5",
                "avwmon6",
                "avwmon7",
            };
            auto id = s_randomIds[fhMon.m_randomLevel];
            addObject(
                fhMon, std::move(monster), [&tmplCache, id] { return tmplCache.addId(id); }, true);
            continue;
        }
        auto* def = fhMon.m_id->objectDefs.get({});
        addObject(
            fhMon, std::move(monster), [&tmplCache, def]() { return tmplCache.add(def); }, true);
    }

    for (auto& fhDwelling : src.m_objects.m_dwellings) {
        std::unique_ptr<IMapObject> impl;
        if (fhDwelling.m_id->hasPlayer) {
            auto dwell     = std::make_unique<MapObjectWithOwner>();
            dwell->m_owner = static_cast<uint8_t>(fhDwelling.m_player->legacyId);
            impl           = std::move(dwell);
        } else {
            impl = std::make_unique<MapObjectSimple>();
        }

        addObjectCommon(fhDwelling, std::move(impl));
    }
    for (auto& fhDwelling : src.m_objects.m_randomDwellings) {
        auto dwell     = std::make_unique<MapDwelling>(fhDwelling.m_hasFaction, fhDwelling.m_hasLevel);
        dwell->m_owner = static_cast<uint8_t>(fhDwelling.m_player->legacyId);

        dwell->m_factionId   = fhDwelling.m_factionId;
        dwell->m_factionMask = fhDwelling.m_factionMask;
        dwell->m_minLevel    = fhDwelling.m_minLevel;
        dwell->m_maxLevel    = fhDwelling.m_maxLevel;

        addObject(fhDwelling, std::move(dwell), [&fhDwelling, &tmplCache] { return tmplCache.add(fhDwelling.m_id); });
    }

    for (auto& fhMine : src.m_objects.m_mines) {
        auto mine     = std::make_unique<MapObjectWithOwner>();
        mine->m_owner = static_cast<uint8_t>(fhMine.m_player->legacyId);

        addObject(fhMine, std::move(mine), [&fhMine, &tmplCache] { return tmplCache.add(fhMine.m_id->minesDefs.get(fhMine.m_defIndex)); });
    }
    for (auto& fhMine : src.m_objects.m_abandonedMines) {
        auto mine = std::make_unique<MapAbandonedMine>();
        mine->prepareArrays();
        for (auto* res : fhMine.m_resources)
            mine->m_resourceBits[res->legacyId] = 1;

        addObjectVisitable(fhMine, std::move(mine));
    }

    for (auto& fhBank : src.m_objects.m_banks) {
        auto bank = std::make_unique<MapObjectCreatureBank>();

        if (fhBank.m_guardsVariant != -1) {
            bank->m_content = fhBank.m_guardsVariant;
            if (fhBank.m_id->upgradedStackIndex != -1) {
                if (fhBank.m_upgradedStack == FHBank::UpgradedStack::No)
                    bank->m_upgraded = 0;
                if (fhBank.m_upgradedStack == FHBank::UpgradedStack::Yes)
                    bank->m_upgraded = 1;
            }
        }

        for (auto* art : fhBank.m_artifacts)
            bank->m_artifacts.push_back(art ? (uint32_t) art->legacyId : uint32_t(-1));

        addObjectCommon(fhBank, std::move(bank));
    }
    for (auto& fhObstacle : src.m_objects.m_obstacles) {
        auto obj = std::make_unique<MapObjectSimple>();

        addObjectCommon(fhObstacle, std::move(obj));
    }
    for (auto& fhVisitable : src.m_objects.m_visitables) {
        auto obj = std::make_unique<MapObjectSimple>();
        addObjectVisitable(fhVisitable, std::move(obj));
    }
    for (auto& fhVisitable : src.m_objects.m_controlledVisitables) {
        auto obj     = std::make_unique<MapObjectWithOwner>();
        obj->m_owner = static_cast<uint8_t>(fhVisitable.m_player->legacyId);
        addObjectVisitable(fhVisitable, std::move(obj));
    }
    for (auto& fhShrine : src.m_objects.m_shrines) {
        auto obj = std::make_unique<MapShrine>();
        if (fhShrine.m_spellId)
            obj->m_spell = static_cast<uint8_t>(fhShrine.m_spellId->legacyId);
        else
            obj->m_spell = 0xffU;

        addObjectVisitable(fhShrine, std::move(obj));
    }
    for (auto& fhSkillHut : src.m_objects.m_skillHuts) {
        auto obj = std::make_unique<MapWitchHut>();
        obj->prepareArrays(dest.m_features.get());
        for (auto* allowedSkill : fhSkillHut.m_skillIds)
            obj->m_allowedSkills[allowedSkill->legacyId] = 1;

        addObjectVisitable(fhSkillHut, std::move(obj));
    }
    for (auto& fhScholar : src.m_objects.m_scholars) {
        auto obj         = std::make_unique<MapScholar>();
        obj->m_bonusType = fhScholar.m_type == FHScholar::Type::Random ? 0xffU : static_cast<uint8_t>(fhScholar.m_type);
        if (fhScholar.m_type == FHScholar::Type::Primary)
            obj->m_bonusId = static_cast<uint8_t>(fhScholar.m_primaryType);
        else if (fhScholar.m_type == FHScholar::Type::Secondary)
            obj->m_bonusId = static_cast<uint8_t>(fhScholar.m_skillId->legacyId);
        else if (fhScholar.m_type == FHScholar::Type::Spell)
            obj->m_bonusId = static_cast<uint8_t>(fhScholar.m_spellId->legacyId);

        addObjectVisitable(fhScholar, std::move(obj));
    }

    for (auto& fhQuestHut : src.m_objects.m_questHuts) {
        auto obj = std::make_unique<MapSeerHut>();
        obj->m_questsOneTime.resize(fhQuestHut.m_questsOneTime.size());
        obj->m_questsRecurring.resize(fhQuestHut.m_questsRecurring.size());
        for (size_t i = 0; i < obj->m_questsOneTime.size(); ++i) {
            obj->m_questsOneTime[i].m_quest.prepareArrays(dest.m_features.get());
            convertRewardHut(fhQuestHut.m_questsOneTime[i].m_reward, obj->m_questsOneTime[i]);
            convertQuest(fhQuestHut.m_questsOneTime[i].m_quest, obj->m_questsOneTime[i].m_quest);
        }
        for (size_t i = 0; i < obj->m_questsRecurring.size(); ++i) {
            obj->m_questsRecurring[i].m_quest.prepareArrays(dest.m_features.get());
            convertRewardHut(fhQuestHut.m_questsRecurring[i].m_reward, obj->m_questsRecurring[i]);
            convertQuest(fhQuestHut.m_questsRecurring[i].m_quest, obj->m_questsRecurring[i].m_quest);
        }

        addObjectVisitable(fhQuestHut, std::move(obj));
    }
    for (auto& fhQuestGuard : src.m_objects.m_questGuards) {
        auto obj = std::make_unique<MapQuestGuard>();
        obj->prepareArrays(dest.m_features.get());
        convertQuest(fhQuestGuard.m_quest, obj->m_quest);

        addObjectVisitable(fhQuestGuard, std::move(obj));
    }
    for (auto& fhPandora : src.m_objects.m_pandoras) {
        if (fhPandora.m_openPandora && fhPandora.m_reward.units.size()) {
            const Core::UnitWithCount& u       = fhPandora.m_reward.units[0];
            auto                       monster = std::make_unique<MapMonster>();
            monster->m_count                   = static_cast<uint16_t>(u.count);
            monster->m_joinAppeal              = 0;
            monster->m_upgradedStack           = 0;

            auto* def = u.unit->objectDefs.get({});

            addObject(
                fhPandora, std::move(monster), [&tmplCache, def] { return tmplCache.add(def); }, true);
            continue;
        }
        auto obj = std::make_unique<MapPandora>();
        obj->prepareArrays(dest.m_features.get());
        convertReward(fhPandora.m_reward, obj->m_reward);
        convertMessage(fhPandora.m_messageWithBattle, obj->m_message);

        bool water = src.m_tileMap.get(fhPandora.m_pos).m_terrainId->id == Core::LibraryTerrain::s_terrainWater;
        addObject(fhPandora, std::move(obj), [&tmplCache, water] { return tmplCache.addId(water ? "ava0128w" : "ava0128"); });
    }

    for (auto& fhEvent : src.m_objects.m_localEvents) {
        auto obj = std::make_unique<MapEvent>();

        obj->prepareArrays(dest.m_features.get());
        convertMessage(fhEvent.m_message, obj->m_message);
        convertReward(fhEvent.m_reward, obj->m_reward);
        for (auto* player : fhEvent.m_players)
            obj->m_players[player->legacyId] = 1;

        obj->m_computerActivate = fhEvent.m_computerActivate;
        obj->m_removeAfterVisit = fhEvent.m_removeAfterVisit;
        obj->m_humanActivate    = fhEvent.m_humanActivate;

        addObject(fhEvent, std::move(obj), [&tmplCache] { return tmplCache.addId("avzevnt0"); });
    }
    for (auto& fhSign : src.m_objects.m_signs) {
        auto obj       = std::make_unique<MapSignBottle>();
        obj->m_message = fhSign.m_text;

        addObjectVisitable(fhSign, std::move(obj));
    }
    for (auto& fhGarison : src.m_objects.m_garisons) {
        auto obj = std::make_unique<MapGarison>();
        obj->prepareArrays(dest.m_features.get());
        convertSquad(fhGarison.m_garison, obj->m_garison);
        obj->m_owner          = static_cast<uint8_t>(fhGarison.m_player->legacyId);
        obj->m_removableUnits = fhGarison.m_removableUnits;

        addObjectVisitable(fhGarison, std::move(obj));
    }

    for (auto& fhObj : src.m_objects.m_heroPlaceholders) {
        auto obj         = std::make_unique<MapHeroPlaceholder>();
        obj->m_owner     = static_cast<uint8_t>(fhObj.m_player->legacyId);
        obj->m_hero      = fhObj.m_hero;
        obj->m_powerRank = fhObj.m_powerRank;
        addObject(fhObj, std::move(obj), [&tmplCache] { return tmplCache.addId("ahplace"); });
    }
    for (auto& fhObj : src.m_objects.m_grails) {
        auto obj      = std::make_unique<MapGrail>();
        obj->m_radius = fhObj.m_radius;
        addObject(fhObj, std::move(obj), [&tmplCache] { return tmplCache.addId("avzgrail"); });
    }
    for (auto& fhObj : src.m_objects.m_unknownObjects) {
        auto obj = std::make_unique<MapObjectSimple>();

        addObject(fhObj, std::move(obj), [&tmplCache, &fhObj] { return tmplCache.addId(fhObj.m_defId); });
    }

    for (auto& fhEvent : src.m_globalEvents) {
        GlobalMapEvent event;
        event.prepareArrays(dest.m_features.get());
        convertEvent(fhEvent, event);
        dest.m_globalEvents.push_back(std::move(event));
    }

    dest.m_objectDefs = std::move(tmplCache.m_objectDefs);

    std::sort(dest.m_objects.begin(), dest.m_objects.end(), [](const auto& lh, const auto& rh) { return lh.m_order < rh.m_order; });
}

std::vector<uint32_t> FH2H3MConverter::convertResources(const Core::ResourceAmount& amount) const
{
    std::vector<uint32_t> res(7);
    for (const auto& [id, count] : amount.data)
        res[id->legacyId] = count;
    return res;
}
void FH2H3MConverter::convertReward(const Core::Reward& fhReward, MapReward& reward) const
{
    reward.m_gainedExp  = fhReward.gainedExp;
    reward.m_manaDiff   = fhReward.manaDiff;
    reward.m_luckDiff   = fhReward.rngBonus.luck;
    reward.m_moraleDiff = fhReward.rngBonus.morale;

    reward.m_resourceSet.m_resourceAmount = convertResources(fhReward.resources);
    reward.m_primSkillSet.m_prim          = convertPrimaryStats(fhReward.statBonus);

    for (const auto& artSet : fhReward.artifacts) {
        for (auto* art : artSet.onlyArtifacts) {
            reward.m_artifacts.push_back(static_cast<uint16_t>(art->legacyId));
        }
    }
    for (const Core::SkillHeroItem& skill : fhReward.secSkills)
        reward.m_secSkills.push_back(MapHeroSkill{ .m_id = static_cast<uint8_t>(skill.skill->legacyId), .m_level = static_cast<uint8_t>(skill.level + 1) });

    for (auto* spell : fhReward.spells.onlySpells)
        reward.m_spells.push_back(static_cast<uint8_t>(spell->legacyId));

    reward.m_creatures.m_stacks = convertStacks(fhReward.units);
}

void FH2H3MConverter::convertRewardHut(const Core::Reward& fhReward, MapSeerHut::MapQuestWithReward& questWithReward) const
{
    if (fhReward.gainedExp) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::EXPERIENCE;
        questWithReward.m_rVal   = static_cast<uint32_t>(fhReward.gainedExp);
    }
    if (fhReward.manaDiff) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::MANA_POINTS;
        questWithReward.m_rVal   = static_cast<uint32_t>(fhReward.manaDiff);
    }
    if (fhReward.rngBonus.morale) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::MORALE_BONUS;
        questWithReward.m_rVal   = static_cast<uint32_t>(fhReward.rngBonus.morale);
    }
    if (fhReward.rngBonus.luck) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::LUCK_BONUS;
        questWithReward.m_rVal   = static_cast<uint32_t>(fhReward.rngBonus.luck);
    }
    if (fhReward.resources.nonEmptyAmount()) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::RESOURCES;
        for (const auto& [res, count] : fhReward.resources.data) {
            questWithReward.m_rVal = static_cast<uint32_t>(count);
            questWithReward.m_rID  = static_cast<uint32_t>(res->legacyId);
        }
    }

    auto applySkill = [&questWithReward](int id, int val) {
        if (val == 0)
            return;
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::PRIMARY_SKILL;
        questWithReward.m_rVal   = static_cast<uint32_t>(val);
        questWithReward.m_rID    = static_cast<uint32_t>(id);
    };

    applySkill(0, fhReward.statBonus.ad.attack);
    applySkill(1, fhReward.statBonus.ad.defense);
    applySkill(2, fhReward.statBonus.magic.spellPower);
    applySkill(3, fhReward.statBonus.magic.intelligence);

    if (!fhReward.secSkills.empty()) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::SECONDARY_SKILL;
        questWithReward.m_rID    = fhReward.secSkills[0].skill->legacyId;
        questWithReward.m_rVal   = fhReward.secSkills[0].level + 1;
    }
    if (!fhReward.artifacts.empty()) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::ARTIFACT;
        questWithReward.m_rID    = fhReward.artifacts[0].onlyArtifacts[0]->legacyId;
    }
    if (!fhReward.spells.isDefault()) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::SPELL;
        questWithReward.m_rID    = fhReward.spells.onlySpells[0]->legacyId;
    }
    if (!fhReward.units.empty()) {
        questWithReward.m_reward = MapSeerHut::MapQuestWithReward::RewardType::CREATURE;
        questWithReward.m_rID    = fhReward.units[0].unit->legacyId;
        questWithReward.m_rVal   = fhReward.units[0].count;
    }
}

std::vector<StackBasicDescriptor> FH2H3MConverter::convertStacks(const std::vector<Core::UnitWithCount>& stacks) const
{
    std::vector<StackBasicDescriptor> result;
    for (auto& stack : stacks)
        result.push_back({ static_cast<uint16_t>(stack.unit->legacyId), static_cast<uint16_t>(stack.count) });
    return result;
}

std::vector<uint8_t> FH2H3MConverter::convertPrimaryStats(const Core::HeroPrimaryParams& stats) const
{
    std::vector<uint8_t> res(4);
    res[0] = stats.ad.attack;
    res[1] = stats.ad.defense;
    res[2] = stats.magic.spellPower;
    res[3] = stats.magic.intelligence;
    return res;
}

void FH2H3MConverter::convertQuest(const FHQuest& fhQuest, MapQuest& quest) const
{
    switch (fhQuest.m_type) {
        case FHQuest::Type::GetHeroLevel:
        {
            quest.m_missionType = MapQuest::Mission::LEVEL;
            quest.m_134val      = fhQuest.m_level;
        } break;
        case FHQuest::Type::GetPrimaryStat:
        {
            quest.m_missionType = MapQuest::Mission::PRIMARY_STAT;
            quest.m_2stats      = convertPrimaryStats(fhQuest.m_primary);
        } break;
        case FHQuest::Type::BringArtifacts:
        {
            quest.m_missionType = MapQuest::Mission::ART;
            for (auto* art : fhQuest.m_artifacts)
                quest.m_5arts.push_back(static_cast<uint16_t>(art->legacyId));
        } break;
        case FHQuest::Type::BringCreatures:
        {
            quest.m_missionType = MapQuest::Mission::ARMY;
            for (const auto& stack : fhQuest.m_units)
                quest.m_6creatures.m_stacks.push_back({ static_cast<uint16_t>(stack.unit->legacyId), static_cast<uint16_t>(stack.count) });
        } break;
        case FHQuest::Type::BringResource:
        {
            quest.m_missionType = MapQuest::Mission::RESOURCES;
            quest.m_7resources  = convertResources(fhQuest.m_resources);
        } break;
        case FHQuest::Type::KillCreature:
        {
            quest.m_missionType = MapQuest::Mission::KILL_CREATURE;
            quest.m_134val      = fhQuest.m_targetQuestId;
        } break;
        case FHQuest::Type::KillHero:
        {
            quest.m_missionType = MapQuest::Mission::KILL_HERO;
            quest.m_134val      = fhQuest.m_targetQuestId;
        } break;
        case FHQuest::Type::BeHero:
        {
            quest.m_missionType = MapQuest::Mission::HERO;
            quest.m_89val       = fhQuest.m_targetQuestId;
        } break;
        case FHQuest::Type::BePlayer:
        {
            quest.m_missionType = MapQuest::Mission::PLAYER;
            quest.m_89val       = fhQuest.m_targetQuestId;
        } break;
        case FHQuest::Type::Invalid:
        {
            quest.m_missionType = MapQuest::Mission::NONE;
        } break;
        default:
            assert(!"Unsupported");
            break;
    }

    quest.m_firstVisitText = fhQuest.m_firstVisitText;
    quest.m_nextVisitText  = fhQuest.m_nextVisitText;
    quest.m_completedText  = fhQuest.m_completedText;

    quest.m_lastDay = fhQuest.m_lastDay;
}

void FH2H3MConverter::convertEvent(const FHGlobalMapEvent& fhEvent, GlobalMapEvent& event) const
{
    event.m_name                         = fhEvent.m_name;
    event.m_message                      = fhEvent.m_message;
    event.m_resourceSet.m_resourceAmount = convertResources(fhEvent.m_resources);
    for (auto* player : fhEvent.m_players)
        event.m_players[player->legacyId] = 1;

    event.m_humanAffected    = fhEvent.m_humanAffected;
    event.m_computerAffected = fhEvent.m_computerAffected;
    event.m_firstOccurence   = fhEvent.m_firstOccurence;
    event.m_nextOccurence    = fhEvent.m_nextOccurence;
}

void FH2H3MConverter::convertMessage(const FHMessageWithBattle& fhMessage, MapMessage& message) const
{
    message.m_hasMessage = fhMessage.m_hasMessage;
    if (message.m_hasMessage) {
        message.m_message            = fhMessage.m_message;
        message.m_guards.m_hasGuards = fhMessage.m_guards.m_hasGuards;
        convertSquad(fhMessage.m_guards.m_creatures, message.m_guards.m_creatures);
    }
}

void FH2H3MConverter::convertSquad(const Core::AdventureSquad& squad, StackSetFixed& fixedStacks) const
{
    for (size_t i = 0; const auto& stack : squad.stacks) {
        if (stack.count && stack.library)
            fixedStacks.m_stacks.at(i) = StackBasicDescriptor{ .m_id = static_cast<uint16_t>(stack.library->legacyId), .m_count = static_cast<uint16_t>(stack.count) };
        else if (stack.count && stack.randomTier >= 0)
            fixedStacks.m_stacks.at(i) = StackBasicDescriptor{ .m_id = static_cast<uint16_t>(65534 - stack.randomTier), .m_count = static_cast<uint16_t>(stack.count) };
        else
            fixedStacks.m_stacks.at(i) = StackBasicDescriptor{ .m_id = (uint16_t) -1 };
        i++;
    }
}

void FH2H3MConverter::convertHeroArtifacts(const Core::AdventureHero& hero, HeroArtSet& artSet) const
{
    for (size_t i = 0; i < artSet.m_mainSlots.size(); ++i) {
        Core::ArtifactSlotType slot  = static_cast<Core::ArtifactSlotType>(i);
        uint16_t&              artId = artSet.m_mainSlots[i];
        auto                   art   = hero.getArtifact(slot);
        if (!art)
            artId = uint16_t(-1);
        else
            artId = uint16_t(art->legacyId);
    }
    auto art5 = hero.getArtifact(Core::ArtifactSlotType::Misc4);
    if (!art5)
        artSet.m_misc5 = uint16_t(-1);
    else
        artSet.m_misc5 = uint16_t(art5->legacyId);

    artSet.m_cata = uint16_t(-1);
    artSet.m_book = hero.hasSpellBook ? uint16_t(0) : uint16_t(-1);

    artSet.m_bagSlots.clear();
    for (auto* art : hero.artifactsBagList) {
        artSet.m_bagSlots.push_back(uint16_t(art->legacyId));
    }
    if (artSet.m_bagSlots.size() > 64) {
        artSet.m_bagSlots.resize(64);
    }
}

}
