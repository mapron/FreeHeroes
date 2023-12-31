/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "H3M2FH.hpp"

#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryPlayer.hpp"
#include "LibraryTerrain.hpp"

#include "MernelPlatform/Logger.hpp"
#include "MernelPlatform/StringUtils.hpp"

#include <functional>

namespace FreeHeroes {
using namespace Mernel;

namespace {

std::string printObjDef(const Core::LibraryObjectDef& fhDef)
{
    std::ostringstream os;
    auto               wstr = [&os](const std::string& str) {
        os << '"' << str << '"';
        for (size_t i = str.size(); i < 14; ++i)
            os << ' ';
    };
    auto warr = [&os](const std::vector<uint8_t>& arr) {
        os << '[';
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0)
                os << ',';
            os << int(arr[i]);
        }
        os << "], ";
    };
    wstr(fhDef.id);
    os << ": [ ";
    wstr(fhDef.defFile);
    os << ", ";
    warr(fhDef.blockMap);
    warr(fhDef.visitMap);
    warr(fhDef.terrainsHard);
    warr(fhDef.terrainsSoft);
    os << fhDef.objId << ", " << fhDef.subId << ", " << fhDef.type << ", " << fhDef.priority << "],";
    return os.str();
}

struct ObjTemplateDiag {
    Core::LibraryObjectDef         m_fhDefFromFile;
    Core::LibraryObjectDefConstPtr m_originalRecord = nullptr;
    Core::LibraryObjectDefConstPtr m_substituteAlt  = nullptr;
    std::string                    m_substitutionId;
};

FHPos posFromH3M(H3Pos pos, int xoffset = 0)
{
    return { (pos.m_x + xoffset), pos.m_y, pos.m_z };
}

}

H3M2FHConverter::H3M2FHConverter(const Core::IGameDatabase* database)
    : m_database(database)
{
    m_factionsContainer = database->factions();

    m_artifactIds = database->artifacts()->legacyOrderedRecords();
    m_buildingIds = database->buildings()->legacyOrderedRecords();
    m_factionIds  = m_factionsContainer->legacyOrderedRecords();
    m_heroIds     = database->heroes()->legacyOrderedRecords();
    m_resourceIds = database->resources()->legacyOrderedRecords();
    m_spellIds    = database->spells()->legacyOrderedRecords();
    m_secSkillIds = database->secSkills()->legacyOrderedRecords();
    m_terrainIds  = database->terrains()->legacyOrderedRecords();
    m_unitIds     = database->units()->legacyOrderedRecords();

    auto players = database->players()->legacyOrderedRecords();
    for (int i = 0; i < (int) players.size(); i++)
        m_playerIds[static_cast<uint8_t>(i)] = players[i];
    m_playerIds[uint8_t(-1)] = database->players()->find(std::string(Core::LibraryPlayer::s_none));
}

void H3M2FHConverter::convertMap(const H3Map& src, FHMap& dest) const
{
    dest          = {};
    dest.m_format = static_cast<FHMap::MapFormat>(src.m_format);

    dest.m_config.m_hotaVersion.m_ver1 = src.m_hotaVer.m_ver1;
    dest.m_config.m_hotaVersion.m_ver2 = src.m_hotaVer.m_ver2;
    dest.m_config.m_hotaVersion.m_ver3 = src.m_hotaVer.m_ver3;

    dest.m_tileMap.m_height = dest.m_tileMap.m_width = src.m_tiles.m_size;
    dest.m_tileMap.m_depth                           = 1U + src.m_tiles.m_hasUnderground;

    dest.m_name       = src.m_mapName;
    dest.m_descr      = src.m_mapDescr;
    dest.m_difficulty = src.m_difficulty;
    dest.m_anyPlayers = src.m_anyPlayers;

    dest.m_config.m_allowSpecialWeeks = src.m_hotaVer.m_allowSpecialWeeks;
    dest.m_config.m_hasRoundLimit     = src.m_hotaVer.m_roundLimit != 0xffffffffU;
    if (dest.m_config.m_hasRoundLimit)
        dest.m_config.m_roundLimit = src.m_hotaVer.m_roundLimit;
    dest.m_config.m_levelLimit = src.m_levelLimit;

    dest.m_config.m_unknown1 = src.m_hotaVer.m_unknown1;
    dest.m_config.m_unknown2 = src.m_hotaVer.m_unknown2;
    dest.m_config.m_unknown3 = src.m_hotaVer.m_unknown3;
    dest.m_config.m_unknown4 = src.m_hotaVer.m_unknown4;
    {
        auto& srcCond                 = src.m_victoryCondition;
        auto& destCond                = dest.m_victoryCondition;
        bool  hasArt                  = srcCond.m_type == H3Map::VictoryConditionType::ARTIFACT || srcCond.m_type == H3Map::VictoryConditionType::TRANSPORTITEM;
        bool  hasUnit                 = srcCond.m_type == H3Map::VictoryConditionType::GATHERTROOP;
        bool  hasRes                  = srcCond.m_type == H3Map::VictoryConditionType::GATHERRESOURCE;
        destCond.m_type               = static_cast<decltype(destCond.m_type)>(srcCond.m_type);
        destCond.m_allowNormalVictory = srcCond.m_allowNormalVictory;
        destCond.m_appliesToAI        = srcCond.m_appliesToAI;
        destCond.m_artID              = hasArt ? m_artifactIds.at(srcCond.m_artID) : 0;
        destCond.m_creature.unit      = hasUnit ? m_unitIds.at(srcCond.m_creatureID) : 0;
        destCond.m_creature.count     = hasUnit ? static_cast<uint32_t>(srcCond.m_creatureCount) : 0;
        destCond.m_resourceID         = hasRes ? m_resourceIds.at(srcCond.m_resourceID) : 0;
        destCond.m_resourceAmount     = srcCond.m_resourceAmount;
        destCond.m_pos                = posFromH3M(srcCond.m_pos);
        destCond.m_hallLevel          = srcCond.m_hallLevel;
        destCond.m_castleLevel        = srcCond.m_castleLevel;
        destCond.m_days               = srcCond.m_days;
    }
    {
        auto& srcCond   = src.m_lossCondition;
        auto& destCond  = dest.m_lossCondition;
        destCond.m_type = static_cast<decltype(destCond.m_type)>(srcCond.m_type);
        destCond.m_pos  = posFromH3M(srcCond.m_pos);
        destCond.m_days = srcCond.m_days;
    }

    for (auto& rumor : src.m_rumors)
        dest.m_rumors.push_back({ rumor.m_name, rumor.m_text });

    std::map<Core::LibraryPlayerConstPtr, FHPos>   mainTowns;
    std::map<Core::LibraryPlayerConstPtr, uint8_t> mainHeroes;

    for (int index = 0; const PlayerInfo& playerInfo : src.m_players) {
        const auto playerId = m_playerIds.at(index++);

        auto& fhPlayer = dest.m_players[playerId];

        fhPlayer.m_aiPossible               = playerInfo.m_canComputerPlay;
        fhPlayer.m_humanPossible            = playerInfo.m_canHumanPlay;
        fhPlayer.m_generateHeroAtMainTown   = playerInfo.m_generateHeroAtMainTown;
        fhPlayer.m_hasRandomHero            = playerInfo.m_hasRandomHero;
        fhPlayer.m_isFactionRandom          = playerInfo.m_isFactionRandom;
        fhPlayer.m_team                     = playerInfo.m_team == 0xff ? -1 : playerInfo.m_team;
        fhPlayer.m_unused1                  = playerInfo.m_unused1;
        fhPlayer.m_placeholder              = playerInfo.m_placeholder;
        fhPlayer.m_generatedHeroTownFaction = playerInfo.m_generatedHeroTownFaction;
        fhPlayer.m_mainCustomHeroPortrait   = playerInfo.m_mainCustomHeroPortrait;
        fhPlayer.m_mainCustomHeroName       = playerInfo.m_mainCustomHeroName;
        fhPlayer.m_aiTactic                 = static_cast<FHPlayer::AiTactic>(playerInfo.m_aiTactic);

        for (const auto& fhHeroName : playerInfo.m_heroesNames) {
            fhPlayer.m_heroesNames.push_back({ .m_name = fhHeroName.m_heroName, .m_hero = m_heroIds.at(fhHeroName.m_heroId) });
        }

        if (playerInfo.m_hasMainTown) {
            mainTowns[playerId] = posFromH3M(playerInfo.m_posOfMainTown, +2);
        }
        if (playerInfo.m_mainCustomHeroId != 0xff) {
            mainHeroes[playerId] = playerInfo.m_mainCustomHeroId;
        }

        const uint16_t factionsBitmask = playerInfo.m_allowedFactionsBitmask;
        for (auto* faction : m_factionsContainer->records()) {
            if (faction->legacyId < 0)
                continue;

            if (factionsBitmask & (1U << uint32_t(faction->legacyId)))
                fhPlayer.m_startingFactions.push_back(faction);
        }
    }

    using SortingMap = std::map<Core::LibraryObjectDef::SortingTuple, std::vector<Core::LibraryObjectDefConstPtr>>;
    SortingMap allDbDefs;
    for (auto* rec : m_database->objectDefs()->records()) {
        allDbDefs[rec->asUniqueTuple()].push_back(rec);
    }

    std::vector<Core::LibraryObjectDef> objectDefsCorrected;

    for (const ObjectTemplate& objTempl : src.m_objectDefs) {
        Core::LibraryObjectDef fhDef          = convertDef(objTempl);
        Core::LibraryObjectDef fhDefCorrected = fhDef;

        Core::LibraryObjectDefConstPtr record = m_database->objectDefs()->find(fhDef.id);

        if (record && record->type == fhDef.type) {
            // good
        } else {
            Logger(Logger::Warning) << "not good id: " << fhDef.id;
            auto it                  = allDbDefs.find(fhDef.asUniqueTuple());
            bool foundKeyReplacement = it != allDbDefs.cend();

            if (!foundKeyReplacement) {
                if (record) {
                    Logger(Logger::Warning) << "Def is corrupted/used for decoration, making database reference null:" << fhDef.type << " sub:" << fhDef.subId;
                    record = nullptr;
                }

            } else {
                record = nullptr;
                for (auto* rec : it->second) {
                    if (rec->id == fhDef.id) {
                        record = rec;
                        break;
                    }
                }
                for (auto* rec : it->second) {
                    if (rec->terrainsSoft == fhDef.terrainsSoft) {
                        record = rec;
                        break;
                    }
                }

                if (!record) {
                    record = it->second[0];
                    Logger(Logger::Warning) << "no record found for type=" << fhDef.type << ", id=" << fhDef.objId << ", subId=" << fhDef.subId << ", taking " << record->id << " from DB";
                }
            }
        }
        fhDef.substituteFor = record;

        dest.m_objectDefs.push_back(fhDef);
        objectDefsCorrected.push_back(fhDefCorrected);
    }

    for (int index = 0; const Object& obj : src.m_objects) {
        const IMapObject* impl = obj.m_impl.get();
        //const ObjectTemplate&          objTempl       = objectDefsCorrected[obj.m_defnum];
        const Core::LibraryObjectDef& objDefEmbedded  = dest.m_objectDefs[obj.m_defnum];
        const Core::LibraryObjectDef& objDefCorrected = objectDefsCorrected[obj.m_defnum];

        Core::LibraryObjectDefConstPtr objDefDatabase = objDefEmbedded.substituteFor;

        Core::ObjectDefIndex defIndex;
        defIndex.forcedIndex = obj.m_defnum;
        auto initCommon      = [&obj, index, &defIndex](FHCommonObject& fhCommon) {
            fhCommon.m_pos      = posFromH3M(obj.m_pos);
            fhCommon.m_order    = index;
            fhCommon.m_defIndex = defIndex;
        };

        if (!objDefDatabase) {
            FHUnknownObject fhUnknown;
            initCommon(fhUnknown);
            fhUnknown.m_defId = objDefCorrected.id;
            dest.m_objects.m_unknownObjects.push_back(std::move(fhUnknown));
            Logger(Logger::Warning) << "unknown def:" << printObjDef(objDefCorrected);
            continue;
        }

        if (objDefDatabase->substituteFor) {
            defIndex.substitution = objDefDatabase->substituteKey;
            objDefDatabase        = objDefDatabase->substituteFor;
        }

        defIndex.variant    = objDefDatabase->mappings.key;
        auto& mappings      = objDefDatabase->mappings;
        auto  initVisitable = [&mappings, &objDefDatabase](FHCommonVisitable& common) {
            auto* visitableId = mappings.mapVisitable;
            if (visitableId)
                common.m_visitableId = visitableId;
            else if (objDefDatabase)
                common.m_fixedDef = objDefDatabase;
            else
                assert(!"Impossible state");
        };

        /*Logger(Logger::Warning) << "[" << obj.m_defnum << "] pos=" << posFromH3M(obj.m_pos).toPrintableString()

                                << " objTempl.id=" << objTempl.m_id
                                << ", objTempl.ani=" << objTempl.m_animationFile
                                //<< ", fhDef.id=" << fhDef.id
                                //<< ", fhDef.defFile=" << fhDef.defFile
                                //<< ", fhDef.objId=" << fhDef.objId
                                << ", dbDecord.id=" << objDef->id
                                << ", dbDecord.defFile=" << objDef->defFile
                                << ", dbDecord.objId=" << objDef->objId;*/

        //Logger(Logger::Warning) << "objTempl.m_id=" << objTempl.m_id << ", objDef.id=" << objDef->id << ", objDef.objId=" << objDef->objId;

        MapObjectType type = static_cast<MapObjectType>(objDefCorrected.objId);
        switch (type) {
            case MapObjectType::EVENT:
            {
                assert(dynamic_cast<const MapEvent*>(impl) != nullptr);
                const auto*  event = static_cast<const MapEvent*>(impl);
                FHLocalEvent fhEvent;
                initCommon(fhEvent);
                fhEvent.m_players = convertPlayerList(event->m_players);
                fhEvent.m_message = convertMessage(event->m_message);
                fhEvent.m_reward  = convertReward(event->m_reward);

                fhEvent.m_computerActivate = event->m_computerActivate;
                fhEvent.m_removeAfterVisit = event->m_removeAfterVisit;
                fhEvent.m_humanActivate    = event->m_humanActivate;
                dest.m_objects.m_localEvents.push_back(std::move(fhEvent));
            } break;
            case MapObjectType::HERO:
            case MapObjectType::PRISON:
            case MapObjectType::RANDOM_HERO:
            {
                assert(dynamic_cast<const MapHero*>(impl) != nullptr);
                const auto* hero = static_cast<const MapHero*>(impl);

                const auto playerId = m_playerIds.at(hero->m_playerOwner);
                FHHero     fhhero;
                fhhero.m_isRandom = type == MapObjectType::RANDOM_HERO;
                fhhero.m_isPrison = type == MapObjectType::PRISON;
                fhhero.m_player   = playerId;
                initCommon(fhhero);
                fhhero.m_pos = posFromH3M(obj.m_pos);
                {
                    auto blockMapPlanar = Core::LibraryObjectDef::makePlanarMask(objDefCorrected.blockMap, true);
                    auto visitMapPlanar = Core::LibraryObjectDef::makePlanarMask(objDefCorrected.visitMap, false);
                    auto combinedMask   = Core::LibraryObjectDef::makeCombinedMask(blockMapPlanar, visitMapPlanar);
                    fhhero.m_pos.m_x    = fhhero.m_pos.m_x + combinedMask.m_visitable.begin()->m_x;
                }
                fhhero.m_isMain           = mainHeroes.contains(playerId) && mainHeroes[playerId] == hero->m_subID;
                fhhero.m_questIdentifier  = hero->m_questIdentifier;
                fhhero.m_unknown1         = hero->m_unknown1;
                fhhero.m_unknown2         = hero->m_unknown2;
                fhhero.m_patrolRadius     = hero->m_patrolRadius == 0xff ? -1 : hero->m_patrolRadius;
                fhhero.m_groupedFormation = hero->m_formation;

                FHHeroData& destHero = fhhero.m_data;
                if (fhhero.m_isRandom)
                    destHero.m_army.hero = Core::AdventureHero(nullptr);
                else
                    destHero.m_army.hero = Core::AdventureHero(m_heroIds[hero->m_subID]);
                destHero.m_hasExp = hero->m_hasExp;
                if (destHero.m_hasExp) {
                    destHero.m_army.hero.experience = hero->m_exp;
                }
                destHero.m_hasName = hero->m_hasName;
                if (destHero.m_hasName) {
                    destHero.m_name = hero->m_name;
                }
                destHero.m_hasCustomBio = hero->m_hasCustomBio;
                if (destHero.m_hasCustomBio) {
                    destHero.m_bio = hero->m_bio;
                }
                destHero.m_hasPortrait = hero->m_hasPortrait;
                if (destHero.m_hasPortrait) {
                    destHero.m_portrait = hero->m_portrait;
                }
                destHero.m_sex           = hero->m_sex == 0xFFU ? -1 : static_cast<int>(hero->m_sex);
                destHero.m_hasPrimSkills = hero->m_primSkillSet.m_hasCustomPrimSkills;
                destHero.m_hasSpells     = hero->m_spellSet.m_hasCustomSpells;
                destHero.m_hasSecSkills  = hero->m_hasSecSkills;
                destHero.m_hasArmy       = hero->m_hasArmy;
                destHero.m_hasArts       = hero->m_artSet.m_hasArts;

                if (destHero.m_hasSecSkills) {
                    auto& skillList = destHero.m_army.hero.secondarySkills;
                    skillList.clear();
                    for (auto& sk : hero->m_secSkills) {
                        const auto* secSkillId = m_secSkillIds[sk.m_id];
                        skillList.push_back({ secSkillId, sk.m_level - 1 });
                    }
                }
                if (destHero.m_hasPrimSkills) {
                    auto& prim                                              = hero->m_primSkillSet.m_primSkills;
                    destHero.m_army.hero.currentBasePrimary.ad.asTuple()    = std::tie(prim[0], prim[1]);
                    destHero.m_army.hero.currentBasePrimary.magic.asTuple() = std::tie(prim[2], prim[3]);
                }
                if (destHero.m_hasArmy) {
                    destHero.m_army.squad = convertSquad(hero->m_garison);
                }
                if (destHero.m_hasSpells) {
                    destHero.m_army.hero.spellbook.clear();
                    for (size_t spellId = 0; spellId < hero->m_spellSet.m_spells.size(); ++spellId) {
                        if (hero->m_spellSet.m_spells[spellId])
                            destHero.m_army.hero.spellbook.insert(m_spellIds[spellId]);
                    }
                }
                if (destHero.m_hasArts) {
                    convertHeroArtifacts(hero->m_artSet, destHero.m_army.hero);
                }
                //Mernel::Logger(Mernel::Logger::Warning) << "parsed hero/prison " << fhhero.m_pos.toPrintableString() << " def " << fhhero.m_defIndex.forcedIndex;

                dest.m_wanderingHeroes.push_back(std::move(fhhero));
            } break;

            case MapObjectType::MONSTER:
            case MapObjectType::RANDOM_MONSTER:
            case MapObjectType::RANDOM_MONSTER_L1:
            case MapObjectType::RANDOM_MONSTER_L2:
            case MapObjectType::RANDOM_MONSTER_L3:
            case MapObjectType::RANDOM_MONSTER_L4:
            case MapObjectType::RANDOM_MONSTER_L5:
            case MapObjectType::RANDOM_MONSTER_L6:
            case MapObjectType::RANDOM_MONSTER_L7:
            {
                assert(dynamic_cast<const MapMonster*>(impl) != nullptr);
                const auto* monster  = static_cast<const MapMonster*>(impl);
                bool        isRandom = type != MapObjectType::MONSTER;
                FHMonster   fhMonster;
                initCommon(fhMonster);
                fhMonster.m_pos = posFromH3M(obj.m_pos);
                {
                    auto blockMapPlanar = Core::LibraryObjectDef::makePlanarMask(objDefCorrected.blockMap, true);
                    auto visitMapPlanar = Core::LibraryObjectDef::makePlanarMask(objDefCorrected.visitMap, false);
                    auto combinedMask   = Core::LibraryObjectDef::makeCombinedMask(blockMapPlanar, visitMapPlanar);
                    fhMonster.m_pos.m_x = fhMonster.m_pos.m_x + combinedMask.m_visitable.begin()->m_x;
                }
                fhMonster.m_count = monster->m_count;
                if (isRandom) {
                    switch (type) {
                            // clang-format off
                        case MapObjectType::RANDOM_MONSTER   : fhMonster.m_randomLevel = 0; break;
                        case MapObjectType::RANDOM_MONSTER_L1: fhMonster.m_randomLevel = 1; break;
                        case MapObjectType::RANDOM_MONSTER_L2: fhMonster.m_randomLevel = 2; break;
                        case MapObjectType::RANDOM_MONSTER_L3: fhMonster.m_randomLevel = 3; break;
                        case MapObjectType::RANDOM_MONSTER_L4: fhMonster.m_randomLevel = 4; break;
                        case MapObjectType::RANDOM_MONSTER_L5: fhMonster.m_randomLevel = 5; break;
                        case MapObjectType::RANDOM_MONSTER_L6: fhMonster.m_randomLevel = 6; break;
                        case MapObjectType::RANDOM_MONSTER_L7: fhMonster.m_randomLevel = 7; break;
                            // clang-format on

                        default:
                            break;
                    }
                } else {
                    fhMonster.m_id = m_unitIds[objDefCorrected.subId];
                }
                fhMonster.m_questIdentifier = monster->m_questIdentifier;
                switch (monster->m_joinAppeal) {
                    case 0:
                        fhMonster.m_aggressionMin = 0;
                        fhMonster.m_aggressionMax = 0;
                        break;
                    case 1:
                        fhMonster.m_aggressionMin = 1;
                        fhMonster.m_aggressionMax = 7;
                        break;
                    case 2:
                        fhMonster.m_aggressionMin = 1;
                        fhMonster.m_aggressionMax = 10;
                        break;
                    case 3:
                        fhMonster.m_aggressionMin = 4;
                        fhMonster.m_aggressionMax = 10;
                        break;
                    case 4:
                        fhMonster.m_aggressionMin = 10;
                        fhMonster.m_aggressionMax = 10;
                        break;
                    case 5:
                        fhMonster.m_aggressionMin = monster->m_aggressionExact;
                        fhMonster.m_aggressionMax = monster->m_aggressionExact;
                        break;
                    default:
                        break;
                }
                fhMonster.m_joinOnlyForMoney = monster->m_joinOnlyForMoney;
                fhMonster.m_joinPercent      = monster->m_joinPercent;

                fhMonster.m_neverFlees     = monster->m_neverFlees;
                fhMonster.m_notGrowingTeam = monster->m_notGrowingTeam;

                if (monster->m_upgradedStack == 0xffffffffU) {
                    fhMonster.m_upgradedStack = FHMonster::UpgradedStack::Random;
                } else if (monster->m_upgradedStack == 1) {
                    fhMonster.m_upgradedStack = FHMonster::UpgradedStack::Yes;
                } else {
                    fhMonster.m_upgradedStack = FHMonster::UpgradedStack::No;
                }
                if (monster->m_splitStack == 0xffffffffU)
                    fhMonster.m_splitStackType = FHMonster::SplitStack::Invalid;
                else if (monster->m_splitStack == 4294967293U)
                    fhMonster.m_splitStackType = FHMonster::SplitStack::Average;
                else if (monster->m_splitStack == 0)
                    fhMonster.m_splitStackType = FHMonster::SplitStack::OneMore;
                else if (monster->m_splitStack == 4294967294U)
                    fhMonster.m_splitStackType = FHMonster::SplitStack::OneLess;
                else {
                    fhMonster.m_splitStackExact = monster->m_splitStack;
                    fhMonster.m_splitStackType  = FHMonster::SplitStack::Exact;
                }

                fhMonster.m_hasMessage = monster->m_hasMessage;
                if (monster->m_hasMessage) {
                    fhMonster.m_message = monster->m_message;
                    if (monster->m_artID != uint16_t(-1)) {
                        fhMonster.m_reward.artifacts.resize(1);
                        fhMonster.m_reward.artifacts[0].onlyArtifacts.push_back(m_artifactIds.at(monster->m_artID));
                    }
                    fhMonster.m_reward.resources = convertResources(monster->m_resourceSet.m_resourceAmount);
                }

                fhMonster.m_quantityMode      = monster->m_quantityMode;
                fhMonster.m_quantityByAiValue = monster->m_quantityByAiValue;

                dest.m_objects.m_monsters.push_back(std::move(fhMonster));
            } break;
            case MapObjectType::OCEAN_BOTTLE:
            case MapObjectType::SIGN:
            {
                assert(dynamic_cast<const MapSignBottle*>(impl) != nullptr);
                const auto* bottle = static_cast<const MapSignBottle*>(impl);

                FHSign fhVisitable;
                initCommon(fhVisitable);
                initVisitable(fhVisitable);
                fhVisitable.m_text = bottle->m_message;
                dest.m_objects.m_signs.push_back(std::move(fhVisitable));
            } break;
            case MapObjectType::SEER_HUT:
            {
                assert(dynamic_cast<const MapSeerHut*>(impl) != nullptr);
                const auto* hut = static_cast<const MapSeerHut*>(impl);

                FHQuestHut fhQuestHut;
                initCommon(fhQuestHut);
                initVisitable(fhQuestHut);
                fhQuestHut.m_questsOneTime.resize(hut->m_questsOneTime.size());
                fhQuestHut.m_questsRecurring.resize(hut->m_questsRecurring.size());

                for (size_t i = 0; i < hut->m_questsOneTime.size(); ++i) {
                    fhQuestHut.m_questsOneTime[i].m_reward = convertRewardHut(hut->m_questsOneTime[i]);
                    fhQuestHut.m_questsOneTime[i].m_quest  = convertQuest(hut->m_questsOneTime[i].m_quest);
                }
                for (size_t i = 0; i < hut->m_questsRecurring.size(); ++i) {
                    fhQuestHut.m_questsRecurring[i].m_reward = convertRewardHut(hut->m_questsRecurring[i]);
                    fhQuestHut.m_questsRecurring[i].m_quest  = convertQuest(hut->m_questsRecurring[i].m_quest);
                }

                dest.m_objects.m_questHuts.push_back(std::move(fhQuestHut));
            } break;
            case MapObjectType::WITCH_HUT:
            {
                assert(dynamic_cast<const MapWitchHut*>(impl) != nullptr);
                const auto* hut = static_cast<const MapWitchHut*>(impl);

                FHSkillHut fhHut;
                initCommon(fhHut);
                initVisitable(fhHut);
                for (size_t skillIndex = 0; skillIndex < hut->m_allowedSkills.size(); ++skillIndex) {
                    if (!hut->m_allowedSkills[skillIndex])
                        continue;
                    auto* secSkill = m_secSkillIds[skillIndex];
                    fhHut.m_skillIds.push_back(secSkill);
                }
                dest.m_objects.m_skillHuts.push_back(std::move(fhHut));
            } break;
            case MapObjectType::SCHOLAR:
            {
                assert(dynamic_cast<const MapScholar*>(impl) != nullptr);
                const auto* scholar = static_cast<const MapScholar*>(impl);

                FHScholar fhScholar;
                initCommon(fhScholar);
                initVisitable(fhScholar);

                fhScholar.m_type = scholar->m_bonusType == 0xff ? FHScholar::Type::Random : static_cast<FHScholar::Type>(scholar->m_bonusType);
                if (fhScholar.m_type == FHScholar::Type::Primary) {
                    fhScholar.m_primaryType = static_cast<Core::HeroPrimaryParamType>(scholar->m_bonusId);
                } else if (fhScholar.m_type == FHScholar::Type::Secondary) {
                    fhScholar.m_skillId = m_secSkillIds.at(scholar->m_bonusId);
                } else if (fhScholar.m_type == FHScholar::Type::Spell) {
                    fhScholar.m_spellId = m_spellIds.at(scholar->m_bonusId);
                } else {
                    assert(scholar->m_bonusType == 0xFFU);
                }

                dest.m_objects.m_scholars.push_back(std::move(fhScholar));
            } break;
            case MapObjectType::GARRISON:
            case MapObjectType::GARRISON2:
            {
                assert(dynamic_cast<const MapGarison*>(impl) != nullptr);
                const auto* garison = static_cast<const MapGarison*>(impl);

                FHGarison fhVisitable;
                initCommon(fhVisitable);
                initVisitable(fhVisitable);
                fhVisitable.m_player         = m_playerIds.at(garison->m_owner);
                fhVisitable.m_removableUnits = garison->m_removableUnits;
                fhVisitable.m_garison        = convertSquad(garison->m_garison);
                dest.m_objects.m_garisons.push_back(std::move(fhVisitable));

            } break;
            case MapObjectType::ARTIFACT:
            case MapObjectType::SPELL_SCROLL:
            {
                FHArtifact art;
                initCommon(art);
                assert(dynamic_cast<const MapArtifact*>(impl) != nullptr);
                const auto* artifact = static_cast<const MapArtifact*>(impl);

                art.m_messageWithBattle = convertMessage(artifact->m_message);
                if (type == MapObjectType::SPELL_SCROLL) {
                    const auto* spell = m_spellIds.at(artifact->m_spellId);
                    assert(spell);
                    art.m_id = m_database->artifacts()->find("sod.artifact." + spell->id); // @todo: constaint for prefix?
                    assert(art.m_id);
                } else {
                    assert(objDefCorrected.subId != 0);
                    art.m_id = m_artifactIds[objDefCorrected.subId];
                }

                art.m_pickupCondition1 = artifact->m_pickupCondition1;
                art.m_pickupCondition2 = artifact->m_pickupCondition2;
                dest.m_objects.m_artifacts.push_back(std::move(art));
            } break;
            case MapObjectType::RANDOM_ART:
            case MapObjectType::RANDOM_TREASURE_ART:
            case MapObjectType::RANDOM_MINOR_ART:
            case MapObjectType::RANDOM_MAJOR_ART:
            case MapObjectType::RANDOM_RELIC_ART:
            {
                FHRandomArtifact art;
                initCommon(art);
                assert(dynamic_cast<const MapArtifact*>(impl) != nullptr);
                const auto* artifact = static_cast<const MapArtifact*>(impl);

                art.m_messageWithBattle = convertMessage(artifact->m_message);
                if (type == MapObjectType::RANDOM_ART)
                    art.m_type = FHRandomArtifact::Type::Any;
                else if (type == MapObjectType::RANDOM_TREASURE_ART)
                    art.m_type = FHRandomArtifact::Type::Treasure;
                else if (type == MapObjectType::RANDOM_MINOR_ART)
                    art.m_type = FHRandomArtifact::Type::Minor;
                else if (type == MapObjectType::RANDOM_MAJOR_ART)
                    art.m_type = FHRandomArtifact::Type::Major;
                else if (type == MapObjectType::RANDOM_RELIC_ART)
                    art.m_type = FHRandomArtifact::Type::Relic;

                dest.m_objects.m_artifactsRandom.push_back(std::move(art));
            } break;
            case MapObjectType::RANDOM_RESOURCE:
            {
                assert(dynamic_cast<const MapResource*>(impl) != nullptr);
                const auto*      resource = static_cast<const MapResource*>(impl);
                FHRandomResource fhres;
                initCommon(fhres);
                fhres.m_amount            = resource->m_amount;
                fhres.m_messageWithBattle = convertMessage(resource->m_message);
                dest.m_objects.m_resourcesRandom.push_back(std::move(fhres));
            } break;
            case MapObjectType::RESOURCE:
            {
                assert(dynamic_cast<const MapResource*>(impl) != nullptr);
                const auto* resource = static_cast<const MapResource*>(impl);
                FHResource  fhres;
                initCommon(fhres);
                fhres.m_amount = resource->m_amount;
                fhres.m_id     = m_resourceIds[objDefCorrected.subId];

                fhres.m_messageWithBattle = convertMessage(resource->m_message);
                fhres.m_amount *= fhres.m_id->pileSize;
                assert(fhres.m_id);
                dest.m_objects.m_resources.push_back(std::move(fhres));
            } break;
            case MapObjectType::RANDOM_TOWN:
            case MapObjectType::TOWN:
            {
                assert(dynamic_cast<const MapTown*>(impl) != nullptr);
                const auto* town     = static_cast<const MapTown*>(impl);
                const auto  playerId = m_playerIds.at(town->m_playerOwner);
                FHTown      fhtown;
                initCommon(fhtown);
                fhtown.m_player     = playerId;
                fhtown.m_randomTown = type == MapObjectType::RANDOM_TOWN;
                if (!fhtown.m_randomTown) {
                    fhtown.m_factionId = m_factionIds[objDefCorrected.subId];
                    assert(fhtown.m_factionId == mappings.factionTown);
                } else {
                    fhtown.m_randomId = objDefDatabase;
                    assert(objDefDatabase);
                }
                fhtown.m_questIdentifier    = town->m_questIdentifier;
                fhtown.m_hasFort            = town->m_hasFort;
                fhtown.m_spellResearch      = town->m_spellResearch;
                fhtown.m_alignment          = town->m_alignment == 0xFFU ? -1 : town->m_alignment;
                fhtown.m_hasCustomBuildings = town->m_hasCustomBuildings;
                fhtown.m_hasGarison         = town->m_hasGarison;
                fhtown.m_hasName            = town->m_hasName;
                fhtown.m_groupedFormation   = town->m_formation;
                fhtown.m_name               = town->m_name;

                fhtown.m_obligatorySpells = town->m_obligatorySpells;
                fhtown.m_possibleSpells   = town->m_possibleSpells;

                if (mainTowns.contains(playerId) && mainTowns.at(playerId) == fhtown.m_pos)
                    fhtown.m_isMain = true;
                if (fhtown.m_hasCustomBuildings) {
                    for (size_t i = 0; i < town->m_builtBuildings.size(); ++i) {
                        if (town->m_builtBuildings[i])
                            fhtown.m_buildings.push_back(m_buildingIds[i]);
                    }
                    for (size_t i = 0; i < town->m_forbiddenBuildings.size(); ++i) {
                        if (town->m_forbiddenBuildings[i])
                            fhtown.m_forbiddenBuildings.push_back(m_buildingIds[i]);
                    }
                }
                fhtown.m_somethingBuildingRelated = town->m_somethingBuildingRelated;
                for (auto& srcEvent : town->m_events) {
                    FHTownEvent destEvent;

                    destEvent.m_name             = srcEvent.m_name;
                    destEvent.m_message          = srcEvent.m_message;
                    destEvent.m_resources        = convertResources(srcEvent.m_resourceSet.m_resourceAmount);
                    destEvent.m_players          = srcEvent.m_players;
                    destEvent.m_humanAffected    = srcEvent.m_humanAffected;
                    destEvent.m_computerAffected = srcEvent.m_computerAffected;
                    destEvent.m_firstOccurence   = srcEvent.m_firstOccurence;
                    destEvent.m_nextOccurence    = srcEvent.m_nextOccurence;
                    destEvent.m_buildings        = srcEvent.m_buildings;
                    destEvent.m_creaturesAmounts = srcEvent.m_creaturesAmounts;

                    fhtown.m_events.push_back(std::move(destEvent));
                }
                if (fhtown.m_hasGarison) {
                    fhtown.m_garison = convertSquad(town->m_garison);
                }
                dest.m_towns.push_back(std::move(fhtown));
            } break;
            case MapObjectType::ABANDONED_MINE:
            case MapObjectType::MINE:
            {
                if (objDefCorrected.subId < 7) {
                    assert(dynamic_cast<const MapObjectWithOwner*>(impl) != nullptr);
                    const auto* objOwner = static_cast<const MapObjectWithOwner*>(impl);
                    FHMine      mine;
                    initCommon(mine);
                    mine.m_player = m_playerIds.at(objOwner->m_owner);
                    mine.m_id     = mappings.resourceMine;
                    dest.m_objects.m_mines.push_back(std::move(mine));
                } else {
                    assert(dynamic_cast<const MapAbandonedMine*>(impl) != nullptr);
                    const auto*     objOwner = static_cast<const MapAbandonedMine*>(impl);
                    FHAbandonedMine mine;
                    initCommon(mine);
                    initVisitable(mine);
                    for (size_t i = 0; i < m_resourceIds.size(); ++i) {
                        if (objOwner->m_resourceBits[i])
                            mine.m_resources.push_back(m_resourceIds[i]);
                    }

                    dest.m_objects.m_abandonedMines.push_back(std::move(mine));
                }
            } break;
            case MapObjectType::CREATURE_GENERATOR1:
            case MapObjectType::CREATURE_GENERATOR2:
            case MapObjectType::CREATURE_GENERATOR3:
            case MapObjectType::CREATURE_GENERATOR4:
            {
                assert(dynamic_cast<const MapObjectWithOwner*>(impl) != nullptr);
                const auto* objOwner = static_cast<const MapObjectWithOwner*>(impl);
                FHDwelling  dwelling;
                initCommon(dwelling);
                assert(mappings.dwelling);
                dwelling.m_id     = mappings.dwelling;
                dwelling.m_player = m_playerIds.at(objOwner->m_owner);
                dest.m_objects.m_dwellings.push_back(std::move(dwelling));
            } break;
            case MapObjectType::WAR_MACHINE_FACTORY:
            {
                FHDwelling dwelling;
                initCommon(dwelling);
                assert(mappings.dwelling);
                dwelling.m_id = mappings.dwelling;
                dest.m_objects.m_dwellings.push_back(std::move(dwelling));
            } break;
            case MapObjectType::SHIPYARD:
            case MapObjectType::LIGHTHOUSE:
            {
                assert(dynamic_cast<const MapObjectWithOwner*>(impl) != nullptr);
                const auto* objOwner = static_cast<const MapObjectWithOwner*>(impl);

                FHVisitableControlled fhVisitable;
                initCommon(fhVisitable);
                initVisitable(fhVisitable);
                fhVisitable.m_player = m_playerIds.at(objOwner->m_owner);
                dest.m_objects.m_controlledVisitables.push_back(std::move(fhVisitable));

            } break;
            case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
            case MapObjectType::SHRINE_OF_MAGIC_GESTURE:
            case MapObjectType::SHRINE_OF_MAGIC_THOUGHT:
            {
                assert(dynamic_cast<const MapShrine*>(impl) != nullptr);
                const auto* shrine = static_cast<const MapShrine*>(impl);

                FHShrine fhShrine;
                initCommon(fhShrine);
                initVisitable(fhShrine);
                if (shrine->m_spell == 0xffU) {
                    switch (type) {
                        case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
                            fhShrine.m_randomLevel = objDefCorrected.subId == 3 ? 4 : 1;
                            break;
                        case MapObjectType::SHRINE_OF_MAGIC_GESTURE:
                            fhShrine.m_randomLevel = 2;
                            break;
                        case MapObjectType::SHRINE_OF_MAGIC_THOUGHT:
                            fhShrine.m_randomLevel = 3;
                            break;
                        default:
                            break;
                    }
                } else {
                    fhShrine.m_spellId = m_spellIds.at(shrine->m_spell);
                }
                dest.m_objects.m_shrines.push_back(std::move(fhShrine));
            } break;
            case MapObjectType::PANDORAS_BOX:
            {
                assert(dynamic_cast<const MapPandora*>(impl) != nullptr);
                const auto* pandora = static_cast<const MapPandora*>(impl);
                FHPandora   fhPandora;
                initCommon(fhPandora);
                fhPandora.m_reward            = convertReward(pandora->m_reward);
                fhPandora.m_messageWithBattle = convertMessage(pandora->m_message);
                dest.m_objects.m_pandoras.push_back(std::move(fhPandora));
            } break;
            case MapObjectType::GRAIL:
            {
                assert(dynamic_cast<const MapGrail*>(impl) != nullptr);
                const auto* grail = static_cast<const MapGrail*>(impl);

                FHGrail fhObj;
                initCommon(fhObj);
                fhObj.m_radius = grail->m_radius;
                dest.m_objects.m_grails.push_back(std::move(fhObj));
            } break;
            case MapObjectType::QUEST_GUARD:
            case MapObjectType::BORDER_GATE:
            {
                if (type == MapObjectType::BORDER_GATE && objDefCorrected.subId != 1000) {
                    FHVisitable fhVisitable;
                    initCommon(fhVisitable);
                    initVisitable(fhVisitable);
                    dest.m_objects.m_visitables.push_back(std::move(fhVisitable));
                    break;
                }
                assert(dynamic_cast<const MapQuestGuard*>(impl) != nullptr);
                const auto*  questGuard = static_cast<const MapQuestGuard*>(impl);
                FHQuestGuard fhQuestGuard;
                initCommon(fhQuestGuard);
                initVisitable(fhQuestGuard);

                fhQuestGuard.m_quest = convertQuest(questGuard->m_quest);
                dest.m_objects.m_questGuards.push_back(std::move(fhQuestGuard));
            } break;
            case MapObjectType::RANDOM_DWELLING:         //same as castle + level range  216
            case MapObjectType::RANDOM_DWELLING_LVL:     //same as castle, fixed level   217
            case MapObjectType::RANDOM_DWELLING_FACTION: //level range, fixed faction    218
            {
                assert(dynamic_cast<const MapDwelling*>(impl) != nullptr);
                const auto*      mapDwelling = static_cast<const MapDwelling*>(impl);
                FHRandomDwelling dwelling;
                initCommon(dwelling);

                assert(objDefDatabase);
                dwelling.m_id     = objDefDatabase;
                dwelling.m_player = m_playerIds.at(mapDwelling->m_owner);

                dwelling.m_hasFaction = mapDwelling->m_hasFaction;
                dwelling.m_hasLevel   = mapDwelling->m_hasLevel;

                dwelling.m_factionId   = mapDwelling->m_factionId;
                dwelling.m_factionMask = mapDwelling->m_factionMask;
                dwelling.m_minLevel    = mapDwelling->m_minLevel;
                dwelling.m_maxLevel    = mapDwelling->m_maxLevel;

                dest.m_objects.m_randomDwellings.push_back(std::move(dwelling));

            } break;

            case MapObjectType::HERO_PLACEHOLDER:
            {
                assert(dynamic_cast<const MapHeroPlaceholder*>(impl) != nullptr);
                const auto* mapPlaceholder = static_cast<const MapHeroPlaceholder*>(impl);

                FHHeroPlaceholder fhObj;
                initCommon(fhObj);
                fhObj.m_player    = m_playerIds.at(mapPlaceholder->m_owner);
                fhObj.m_hero      = mapPlaceholder->m_hero;
                fhObj.m_powerRank = mapPlaceholder->m_powerRank;
                dest.m_objects.m_heroPlaceholders.push_back(std::move(fhObj));

            } break;
            case MapObjectType::CREATURE_BANK:
            case MapObjectType::DERELICT_SHIP:
            case MapObjectType::DRAGON_UTOPIA:
            case MapObjectType::CRYPT:
            case MapObjectType::SHIPWRECK:
            {
                assert(dynamic_cast<const MapObjectCreatureBank*>(impl) != nullptr);
                const auto* bank = static_cast<const MapObjectCreatureBank*>(impl);
                auto*       id   = mappings.mapBank;
                FHBank      fhBank;
                if (!id)
                    throw std::runtime_error("Missing bank def mapping:" + objDefEmbedded.id);
                initCommon(fhBank);
                fhBank.m_id = id;
                if (bank->m_content != 0xffffffffu) {
                    fhBank.m_guardsVariant = bank->m_content;

                    if (id->upgradedStackIndex != -1) {
                        if (bank->m_upgraded == 0xffu) {
                            fhBank.m_upgradedStack = FHBank::UpgradedStack::Random;
                        } else if (bank->m_upgraded == 1) {
                            fhBank.m_upgradedStack = FHBank::UpgradedStack::Yes;
                        } else {
                            fhBank.m_upgradedStack = FHBank::UpgradedStack::No;
                        }
                    }
                }
                for (uint32_t artId : bank->m_artifacts) {
                    if (artId == uint32_t(-1))
                        fhBank.m_artifacts.push_back(nullptr);
                    else
                        fhBank.m_artifacts.push_back(m_artifactIds[artId]);
                }
                dest.m_objects.m_banks.push_back(std::move(fhBank));
            } break;
            default:
            {
                if (mappings.mapObstacle) {
                    FHObstacle fhObstacle;
                    initCommon(fhObstacle);
                    fhObstacle.m_id = mappings.mapObstacle;
                    dest.m_objects.m_obstacles.push_back(std::move(fhObstacle));
                    break;
                }
                if (mappings.mapVisitable) {
                    FHVisitable fhVisitable;
                    initCommon(fhVisitable);
                    fhVisitable.m_visitableId = mappings.mapVisitable;
                    dest.m_objects.m_visitables.push_back(std::move(fhVisitable));
                    break;
                }

                {
                    FHUnknownObject fhUnknown;
                    initCommon(fhUnknown);
                    fhUnknown.m_defId = objDefCorrected.id;
                    dest.m_objects.m_unknownObjects.push_back(std::move(fhUnknown));
                    Logger(Logger::Warning) << "unknown def:" << printObjDef(objDefCorrected);
                }
            } break;
        }

        index++;
    }

    for (int index = 0; auto& allowedFlag : src.m_allowedHeroes) {
        const auto& heroId = m_heroIds[index++];
        dest.m_disabledHeroes.setDisabled(dest.m_isWaterMap, heroId, !allowedFlag);
    }

    for (int index = 0; auto& allowedFlag : src.m_allowedArtifacts) {
        const auto& artId = m_artifactIds[index++];
        dest.m_disabledArtifacts.setDisabled(dest.m_isWaterMap, artId, !allowedFlag);
    }

    for (int index = 0; auto& allowedFlag : src.m_allowedSpells) {
        const auto& spellId = m_spellIds[index++];
        dest.m_disabledSpells.setDisabled(dest.m_isWaterMap, spellId, !allowedFlag);
    }
    for (int index = 0; auto& allowedFlag : src.m_allowedSecSkills) {
        const auto& secSkillId = m_secSkillIds[index++];
        dest.m_disabledSkills.setDisabled(dest.m_isWaterMap, secSkillId, !allowedFlag);
    }
    for (uint8_t heroId : src.m_placeholderHeroes) {
        dest.m_placeholderHeroes.push_back(m_heroIds[heroId]);
    }
    for (auto& srcHero : src.m_disposedHeroes) {
        FHDisposedHero destHero;
        destHero.m_players  = convertPlayerList(srcHero.m_players);
        destHero.m_heroId   = m_heroIds.at(srcHero.m_heroId);
        destHero.m_portrait = srcHero.m_portrait == 0xffU ? -1 : srcHero.m_portrait;
        destHero.m_name     = srcHero.m_name;

        dest.m_disposedHeroes.push_back(std::move(destHero));
    }

    for (int index = 0; auto& customHero : src.m_customHeroData) {
        const auto& heroId = m_heroIds[index++];
        if (!customHero.m_enabled)
            continue;
        FHHeroData destHero;
        destHero.m_army.hero     = Core::AdventureHero(heroId);
        destHero.m_hasExp        = customHero.m_hasExp;
        destHero.m_hasCustomBio  = customHero.m_hasCustomBio;
        destHero.m_hasSecSkills  = customHero.m_hasSkills;
        destHero.m_hasPrimSkills = customHero.m_primSkillSet.m_hasCustomPrimSkills;
        destHero.m_hasSpells     = customHero.m_spellSet.m_hasCustomSpells;
        destHero.m_hasArts       = customHero.m_artSet.m_hasArts;
        destHero.m_sex           = customHero.m_sex == 0xFFU ? -1 : static_cast<int>(customHero.m_sex);

        if (destHero.m_hasSecSkills) {
            auto& skillList = destHero.m_army.hero.secondarySkills;
            skillList.clear();
            for (auto& sk : customHero.m_skills) {
                const auto* secSkillId = m_secSkillIds[sk.m_id];
                skillList.push_back({ secSkillId, sk.m_level - 1 });
            }
        }
        if (destHero.m_hasPrimSkills) {
            auto& prim                                              = customHero.m_primSkillSet.m_primSkills;
            destHero.m_army.hero.currentBasePrimary.ad.asTuple()    = std::tie(prim[0], prim[1]);
            destHero.m_army.hero.currentBasePrimary.magic.asTuple() = std::tie(prim[2], prim[3]);
        }
        if (destHero.m_hasSpells) {
            destHero.m_army.hero.spellbook.clear();
            for (size_t spellId = 0; spellId < customHero.m_spellSet.m_spells.size(); ++spellId) {
                if (customHero.m_spellSet.m_spells[spellId])
                    destHero.m_army.hero.spellbook.insert(m_spellIds[spellId]);
            }
        }
        if (destHero.m_hasArts) {
            convertHeroArtifacts(customHero.m_artSet, destHero.m_army.hero);
        }
        if (destHero.m_hasCustomBio) {
            destHero.m_bio = customHero.m_bio;
        }

        if (destHero.m_hasExp)
            destHero.m_army.hero.experience = customHero.m_exp;
        dest.m_customHeroes.push_back(std::move(destHero));
    }
    for (auto& event : src.m_globalEvents) {
        dest.m_globalEvents.push_back(convertEvent(event));
    }
    for (auto& data : src.m_customHeroDataExt) {
        dest.m_customHeroDataExt.push_back({ data.m_unknown1, data.m_unknown2 });
    }

    convertTileMap(src, dest);
    assert(dest.m_tileMap.m_width > 0);
    assert(dest.m_tileMap.m_width == dest.m_tileMap.m_height);
}

Core::ResourceAmount H3M2FHConverter::convertResources(const std::vector<uint32_t>& resourceAmount) const
{
    Core::ResourceAmount resources;
    for (size_t legacyId = 0; legacyId < m_resourceIds.size(); ++legacyId) {
        auto       resId = m_resourceIds[legacyId];
        const auto count = resourceAmount[legacyId];
        if (count)
            resources.data[resId] = count;
    }
    return resources;
}

Core::HeroPrimaryParams H3M2FHConverter::convertPrim(const std::vector<uint8_t>& arr) const
{
    Core::HeroPrimaryParams result;
    result.ad.attack          = arr[0];
    result.ad.defense         = arr[1];
    result.magic.spellPower   = arr[2];
    result.magic.intelligence = arr[3];
    return result;
}

std::vector<Core::UnitWithCount> H3M2FHConverter::convertStacks(const std::vector<StackBasicDescriptor>& stacks) const
{
    std::vector<Core::UnitWithCount> result;
    for (auto& stack : stacks)
        result.push_back({ m_unitIds[stack.m_id], stack.m_count });
    return result;
}

Core::AdventureSquad H3M2FHConverter::convertSquad(const StackSetFixed& fixedStacks) const
{
    Core::AdventureSquad squad;
    for (const auto& stack : fixedStacks.m_stacks) {
        if (stack.m_count && stack.m_id <= uint16_t(65534) && stack.m_id >= uint16_t(65520)) {
            Core::AdventureStack as;
            as.count      = stack.m_count;
            as.randomTier = 65534 - stack.m_id;
            squad.stacks.push_back(as);
        } else if (stack.m_count && stack.m_id != uint16_t(-1)) {
            squad.stacks.push_back(Core::AdventureStack(m_unitIds[stack.m_id], stack.m_count));
        } else {
            squad.stacks.push_back(Core::AdventureStack());
        }
    }
    return squad;
}

Core::Reward H3M2FHConverter::convertRewardHut(const MapSeerHut::MapQuestWithReward& questWithReward) const
{
    using RewardType = MapSeerHut::MapQuestWithReward::RewardType;
    Core::Reward fhReward;
    switch (questWithReward.m_reward) {
        case RewardType::EXPERIENCE:
            fhReward.gainedExp = questWithReward.m_rVal;
            break;
        case RewardType::MANA_POINTS:
            fhReward.manaDiff = questWithReward.m_rVal;
            break;
        case RewardType::MORALE_BONUS:
            fhReward.rngBonus.morale = questWithReward.m_rVal;
            break;
        case RewardType::LUCK_BONUS:

            fhReward.rngBonus.luck = questWithReward.m_rVal;
            break;

        case RewardType::RESOURCES:
        {
            std::vector<uint32_t> res(7);
            res[questWithReward.m_rID] = questWithReward.m_rVal;
            fhReward.resources         = convertResources(res);

            break;
        }
        case RewardType::PRIMARY_SKILL:
        {
            // clang-format off
        switch (questWithReward.m_rID)  {
            case 0: fhReward.statBonus.ad.attack          = questWithReward.m_rVal; break;
            case 1: fhReward.statBonus.ad.defense         = questWithReward.m_rVal; break;
            case 2: fhReward.statBonus.magic.spellPower   = questWithReward.m_rVal; break;
            case 3: fhReward.statBonus.magic.intelligence = questWithReward.m_rVal; break;
        }
        break;
            // clang-format on
        }

        case RewardType::SECONDARY_SKILL:
        {
            fhReward.secSkills.push_back({ m_secSkillIds[questWithReward.m_rID], (int) questWithReward.m_rVal - 1 });
            break;
        }
        case RewardType::ARTIFACT:
        {
            fhReward.artifacts.push_back(Core::ArtifactFilter{ .onlyArtifacts = { m_artifactIds[questWithReward.m_rID] } });
            break;
        }
        case RewardType::SPELL:
        {
            fhReward.spells.onlySpells.push_back({ m_spellIds[questWithReward.m_rID] });
            break;
        }
        case RewardType::CREATURE:
        {
            fhReward.units.push_back({ m_unitIds[questWithReward.m_rID], (int) questWithReward.m_rVal });
            break;
        }
        case RewardType::NOTHING:
        {
            break;
        }
    }

    return fhReward;
}

Core::Reward H3M2FHConverter::convertReward(const MapReward& reward) const
{
    Core::Reward fhReward;
    fhReward.gainedExp       = reward.m_gainedExp;
    fhReward.manaDiff        = reward.m_manaDiff;
    fhReward.rngBonus.luck   = reward.m_luckDiff;
    fhReward.rngBonus.morale = reward.m_moraleDiff;

    fhReward.statBonus = convertPrim(reward.m_primSkillSet.m_prim);

    fhReward.resources = convertResources(reward.m_resourceSet.m_resourceAmount);

    for (auto& skill : reward.m_secSkills)
        fhReward.secSkills.push_back({ m_secSkillIds[skill.m_id], skill.m_level - 1 });

    for (auto artId : reward.m_artifacts) {
        fhReward.artifacts.push_back(Core::ArtifactFilter{ .onlyArtifacts = { m_artifactIds[artId] } });
    }

    for (uint8_t spellId : reward.m_spells)
        fhReward.spells.onlySpells.push_back(m_spellIds.at(spellId));

    fhReward.units = convertStacks(reward.m_creatures.m_stacks);
    return fhReward;
}

FHQuest H3M2FHConverter::convertQuest(const MapQuest& quest) const
{
    FHQuest fhQuest;
    switch (quest.m_missionType) {
        case MapQuest::Mission::PRIMARY_STAT:
        {
            fhQuest.m_type    = FHQuest::Type::GetPrimaryStat;
            fhQuest.m_primary = convertPrim(quest.m_2stats);
        } break;
        case MapQuest::Mission::LEVEL:
        {
            fhQuest.m_type  = FHQuest::Type::GetHeroLevel;
            fhQuest.m_level = quest.m_134val;
        } break;
        case MapQuest::Mission::ART:
        {
            fhQuest.m_type = FHQuest::Type::BringArtifacts;
            for (uint16_t id : quest.m_5arts)
                fhQuest.m_artifacts.push_back(m_artifactIds[id]);
        } break;
        case MapQuest::Mission::ARMY:
        {
            fhQuest.m_type  = FHQuest::Type::BringCreatures;
            fhQuest.m_units = convertStacks(quest.m_6creatures.m_stacks);
        } break;
        case MapQuest::Mission::RESOURCES:
        {
            fhQuest.m_type      = FHQuest::Type::BringResource;
            fhQuest.m_resources = convertResources(quest.m_7resources);
        } break;
        case MapQuest::Mission::KILL_CREATURE:
        {
            fhQuest.m_type          = FHQuest::Type::KillCreature;
            fhQuest.m_targetQuestId = quest.m_134val;
        } break;
        case MapQuest::Mission::KILL_HERO:
        {
            fhQuest.m_type          = FHQuest::Type::KillHero;
            fhQuest.m_targetQuestId = quest.m_134val;
        } break;
        case MapQuest::Mission::HERO:
        {
            fhQuest.m_type          = FHQuest::Type::BeHero;
            fhQuest.m_targetQuestId = quest.m_89val;
        } break;
        case MapQuest::Mission::PLAYER:
        {
            fhQuest.m_type          = FHQuest::Type::BePlayer;
            fhQuest.m_targetQuestId = quest.m_89val;
        } break;
        case MapQuest::Mission::NONE:
        {
            fhQuest.m_type = FHQuest::Type::Invalid;
        } break;
        default:
            Logger(Logger::Warning) << "Unsupported mission type:" << int(quest.m_missionType);
            assert(!"Unsupported");
            break;
    }

    fhQuest.m_firstVisitText = quest.m_firstVisitText;
    fhQuest.m_nextVisitText  = quest.m_nextVisitText;
    fhQuest.m_completedText  = quest.m_completedText;

    fhQuest.m_lastDay = quest.m_lastDay;
    return fhQuest;
}

FHGlobalMapEvent H3M2FHConverter::convertEvent(const GlobalMapEvent& event) const
{
    FHGlobalMapEvent fhEvent;

    fhEvent.m_name      = event.m_name;
    fhEvent.m_message   = event.m_message;
    fhEvent.m_resources = convertResources(event.m_resourceSet.m_resourceAmount);
    fhEvent.m_players   = convertPlayerList(event.m_players);

    fhEvent.m_humanAffected    = event.m_humanAffected;
    fhEvent.m_computerAffected = event.m_computerAffected;
    fhEvent.m_firstOccurence   = event.m_firstOccurence;
    fhEvent.m_nextOccurence    = event.m_nextOccurence;
    return fhEvent;
}

FHMessageWithBattle H3M2FHConverter::convertMessage(const MapMessage& message) const
{
    FHMessageWithBattle fhMessage;
    fhMessage.m_hasMessage = message.m_hasMessage;
    if (fhMessage.m_hasMessage) {
        fhMessage.m_message            = message.m_message;
        fhMessage.m_guards.m_hasGuards = message.m_guards.m_hasGuards;
        fhMessage.m_guards.m_creatures = convertSquad(message.m_guards.m_creatures);
    }
    return fhMessage;
}

void H3M2FHConverter::convertHeroArtifacts(const HeroArtSet& artSet, Core::AdventureHero& hero) const
{
    for (size_t i = 0; i < artSet.m_mainSlots.size(); ++i) {
        Core::ArtifactSlotType slot  = static_cast<Core::ArtifactSlotType>(i);
        uint16_t               artId = artSet.m_mainSlots[i];
        if (artId == uint16_t(-1))
            continue;
        hero.artifactsOn[slot] = m_artifactIds.at(artId);
    }
    if (artSet.m_misc5 != uint16_t(-1))
        hero.artifactsOn[Core::ArtifactSlotType::Misc4] = m_artifactIds.at(artSet.m_misc5);

    hero.artifactsBagList.clear();
    for (uint16_t artId : artSet.m_bagSlots)
        hero.artifactsBagList.push_back(m_artifactIds.at(artId));

    hero.hasSpellBook = artSet.m_book != uint16_t(-1);
}

std::vector<Core::LibraryPlayerConstPtr> H3M2FHConverter::convertPlayerList(const std::vector<uint8_t>& players) const
{
    std::vector<Core::LibraryPlayerConstPtr> result;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i])
            result.push_back(m_playerIds.at(static_cast<uint8_t>(i)));
    }
    return result;
}

void H3M2FHConverter::convertTileMap(const H3Map& src, FHMap& dest) const
{
    dest.m_tileMap.updateSize();
    dest.m_tileMap.eachPosTile([&src, this](const FHPos& tilePos, FHTileMap::Tile& destTile, size_t) {
        const auto& tile              = src.m_tiles.get(tilePos.m_x, tilePos.m_y, tilePos.m_z);
        destTile.m_terrainId          = m_terrainIds[tile.m_terType];
        destTile.m_terrainView.m_view = tile.m_terView;

        destTile.m_riverType        = static_cast<FHRiverType>(tile.m_riverType);
        destTile.m_riverView.m_view = tile.m_riverDir;

        destTile.m_roadType        = static_cast<FHRoadType>(tile.m_roadType);
        destTile.m_roadView.m_view = tile.m_roadDir;

        destTile.m_terrainView.m_flipHor  = tile.m_flipHor;
        destTile.m_terrainView.m_flipVert = tile.m_flipVert;

        destTile.m_riverView.m_flipHor  = tile.m_riverFlipHor;
        destTile.m_riverView.m_flipVert = tile.m_riverFlipVert;

        destTile.m_roadView.m_flipHor  = tile.m_roadFlipHor;
        destTile.m_roadView.m_flipVert = tile.m_roadFlipVert;

        destTile.m_coastal = tile.m_coastal;
    });
}

Core::LibraryObjectDef H3M2FHConverter::convertDef(const ObjectTemplate& objTempl) const
{
    Core::LibraryObjectDef fhDef;
    fhDef.defFile      = objTempl.m_animationFile;
    fhDef.id           = fhDef.defFile;
    fhDef.blockMap     = objTempl.m_blockMask;
    fhDef.visitMap     = objTempl.m_visitMask;
    fhDef.terrainsHard = objTempl.m_terrainsHard;
    fhDef.terrainsSoft = objTempl.m_terrainsSoft;

    fhDef.objId    = objTempl.m_id;
    fhDef.subId    = objTempl.m_subid;
    fhDef.type     = static_cast<int>(objTempl.m_type);
    fhDef.priority = objTempl.m_drawPriority;

    fhDef.id = Mernel::strToLower(fhDef.id);

    if (fhDef.id.ends_with(".def"))
        fhDef.id = fhDef.id.substr(0, fhDef.id.size() - 4);
    if (fhDef.defFile.ends_with(".def"))
        fhDef.defFile = fhDef.defFile.substr(0, fhDef.defFile.size() - 4);

    return fhDef;
}

}
