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

struct ObjTemplateDiagContainer {
    std::vector<ObjTemplateDiag> m_records;

    ObjTemplateDiag& add()
    {
        m_records.push_back({});
        return *m_records.rbegin();
    }

    void sort()
    {
        std::sort(m_records.begin(), m_records.end(), [](const ObjTemplateDiag& rh, const ObjTemplateDiag& lh) {
            return rh.m_fhDefFromFile.id < lh.m_fhDefFromFile.id;
        });
    }

    void check() const
    {
        std::string missingDefs;
        for (const auto& diag : m_records) {
            if (!diag.m_originalRecord) {
                missingDefs += printObjDef(diag.m_fhDefFromFile) + "\n";
            }
        }
        if (!missingDefs.empty()) {
            Logger(Logger::Err) << "missing defs:\n"
                                << missingDefs;
            throw std::runtime_error("Some defs are missing, map cannot be loaded. Add them to the database.");
        }
    }
};

FHPos posFromH3M(H3Pos pos, int xoffset = 0)
{
    return { (pos.m_x + xoffset), pos.m_y, pos.m_z };
}

class FloodFiller {
public:
    void fillAdjucent(const FHPos& current, const std::set<FHPos>& exclude, std::set<FHPos>& result, const std::function<bool(const MapTileH3M&)>& pred)
    {
        auto addToResult = [this, &result, &exclude, &current, &pred](int dx, int dy) {
            const FHPos neighbour{ current.m_x + dx, current.m_y + dy, current.m_z };
            auto&       neighbourTile = m_srcTileSet->get(neighbour.m_x, neighbour.m_y, neighbour.m_z);
            if (pred(neighbourTile))
                return;
            if (m_zoned.contains(neighbour))
                return;
            if (exclude.contains(neighbour))
                return;
            result.insert(neighbour);
        };
        if (current.m_x < m_destTileMap->m_width - 1)
            addToResult(+1, 0);
        if (current.m_y < m_destTileMap->m_height - 1)
            addToResult(0, +1);
        if (current.m_x > 0)
            addToResult(-1, 0);
        if (current.m_y > 0)
            addToResult(0, -1);
    };

    std::vector<FHPos> makeNewZone(const FHPos& tilePos, const std::function<bool(const MapTileH3M&)>& pred)
    {
        // now we create new zone using flood-fill;
        std::set<FHPos> newZone;
        std::set<FHPos> newZoneIter;
        newZone.insert(tilePos);
        newZoneIter.insert(tilePos);
        while (true) {
            std::set<FHPos> newFloodTiles;
            for (const FHPos& prevIterTile : newZoneIter) {
                fillAdjucent(prevIterTile, newZone, newFloodTiles, pred);
            }
            if (newFloodTiles.empty())
                break;

            newZoneIter = newFloodTiles;
            newZone.insert(newFloodTiles.cbegin(), newFloodTiles.cend());
        }

        m_zoned.insert(newZone.cbegin(), newZone.cend());
        return std::vector<FHPos>(newZone.cbegin(), newZone.cend());
    }

    FloodFiller(const MapTileSet* src, const FHTileMap* dest)
        : m_srcTileSet(src)
        , m_destTileMap(dest)
    {
    }
    const MapTileSet* const m_srcTileSet;
    const FHTileMap* const  m_destTileMap;
    std::set<FHPos>         m_zoned;
};

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
    dest = {};
    if (src.m_format >= MapFormat::HOTA1)
        dest.m_version = Core::GameVersion::HOTA;
    else
        dest.m_version = Core::GameVersion::SOD;

    dest.m_tileMap.m_height = dest.m_tileMap.m_width = src.m_tiles.m_size;
    dest.m_tileMap.m_depth                           = 1U + src.m_tiles.m_hasUnderground;

    dest.m_name       = src.m_mapName;
    dest.m_descr      = src.m_mapDescr;
    dest.m_difficulty = src.m_difficulty;

    dest.m_config.m_allowSpecialWeeks = src.m_hotaVer.m_allowSpecialWeeks;
    dest.m_config.m_hasRoundLimit     = src.m_hotaVer.m_roundLimit != 0xffffffffU;
    if (dest.m_config.m_hasRoundLimit)
        dest.m_config.m_roundLimit = src.m_hotaVer.m_roundLimit;
    dest.m_config.m_levelLimit = src.m_levelLimit;
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

    std::map<Core::LibraryPlayerConstPtr, FHPos>   mainTowns;
    std::map<Core::LibraryPlayerConstPtr, uint8_t> mainHeroes;

    for (int index = 0; const PlayerInfo& playerInfo : src.m_players) {
        const auto playerId = m_playerIds.at(index++);
        auto&      fhPlayer = dest.m_players[playerId];

        fhPlayer.m_aiPossible             = playerInfo.m_canComputerPlay;
        fhPlayer.m_humanPossible          = playerInfo.m_canHumanPlay;
        fhPlayer.m_generateHeroAtMainTown = playerInfo.m_generateHeroAtMainTown;
        fhPlayer.m_team                   = playerInfo.m_team == 0xff ? -1 : playerInfo.m_team;

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

    ObjTemplateDiagContainer diag;

    using SortingMap = std::map<Core::LibraryObjectDef::SortingTuple, std::vector<Core::LibraryObjectDefConstPtr>>;
    SortingMap allDbDefs;
    for (auto* rec : m_database->objectDefs()->records()) {
        allDbDefs[rec->asUniqueTuple()].push_back(rec);
    }

    for (const ObjectTemplate& objTempl : src.m_objectDefs) {
        Core::LibraryObjectDef fhDef      = convertDef(objTempl);
        auto&                  diagRecord = diag.add();
        diagRecord.m_fhDefFromFile        = fhDef;
        diagRecord.m_originalRecord       = nullptr;

        Core::LibraryObjectDefConstPtr record = m_database->objectDefs()->find(fhDef.id);

        if (record && record->type == fhDef.type) {
            // good
        } else {
            Logger(Logger::Warning) << "not good id:" << fhDef.id;
            auto it = allDbDefs.find(fhDef.asUniqueTuple());
            //auto* record                =
            if (it == allDbDefs.cend()) {
                continue;
            }
            record = nullptr;
            for (auto* rec : it->second) {
                if (rec->id == fhDef.id) {
                    record = rec;
                    break;
                }
            }

            if (!record) {
                record = it->second[0];
                Logger(Logger::Warning) << "no record found for type=" << fhDef.type << ", id=" << fhDef.objId << ", subId=" << fhDef.subId << ", taking " << record->id << " from DB";
            }
        }
        if (!record) {
            continue;
            // will throw after loop
        }
        diagRecord.m_originalRecord = record;
        fhDef.substituteFor         = record;
        /*
        Logger(Logger::Warning) << "[" << dest.m_initialObjectDefs.size() << "] "

                                << "objTempl.id=" << objTempl.m_id
                                << ", objTempl.ani=" << objTempl.m_animationFile
                                << ", fhDef.id=" << fhDef.id
                                << ", fhDef.defFile=" << fhDef.defFile
                                << ", fhDef.objId=" << fhDef.objId
                                << ", dbDecord.id=" << record->id
                                << ", dbDecord.defFile=" << record->defFile
                                << ", dbDecord.objId=" << record->objId;*/

        // couple objdefs like sulfur/mercury mines have several def with same unique def name but different bitmask for terrain.
        /*for (const auto& [subId, altRec] : record->substitutions) {
            if (*altRec == fhDef) {
                diagRecord.m_substitutionId = subId;
                diagRecord.m_substituteAlt  = altRec;
                record                      = altRec;
                break;
            }
        }*/

        dest.m_objectDefs.push_back(fhDef);
    }
    diag.sort();
    diag.check();

    std::set<uint32_t> skipped;

    for (int index = 0; const Object& obj : src.m_objects) {
        const IMapObject*              impl           = obj.m_impl.get();
        const ObjectTemplate&          objTempl       = src.m_objectDefs[obj.m_defnum];
        const Core::LibraryObjectDef&  objDefEmbedded = dest.m_objectDefs[obj.m_defnum];
        Core::LibraryObjectDefConstPtr objDefDatabase = objDefEmbedded.substituteFor;
        Core::ObjectDefIndex           defIndex;
        defIndex.forcedIndex = obj.m_defnum;
        //defIndex.substitution = objDef->substituteKey;
        if (objDefDatabase->substituteFor)
            objDefDatabase = objDefDatabase->substituteFor;
        //defIndex.variant = objDef->mappings.key;
        auto& mappings = objDefDatabase->mappings;

        auto initCommon = [&obj, index, &defIndex](FHCommonObject& fhCommon) {
            fhCommon.m_pos      = posFromH3M(obj.m_pos);
            fhCommon.m_order    = index;
            fhCommon.m_defIndex = defIndex;
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

        MapObjectType type = static_cast<MapObjectType>(objTempl.m_id);
        switch (type) {
            case MapObjectType::EVENT:
            {
                const auto*  event = static_cast<const MapEvent*>(impl);
                FHLocalEvent fhEvent;
                fhEvent.m_order   = index;
                fhEvent.m_pos     = posFromH3M(obj.m_pos);
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
            {
                const auto* hero = static_cast<const MapHero*>(impl);

                const auto playerId = m_playerIds.at(hero->m_playerOwner);
                FHHero     fhhero;
                fhhero.m_player          = playerId;
                fhhero.m_order           = index;
                fhhero.m_pos             = posFromH3M(obj.m_pos, type == MapObjectType::PRISON ? 0 : -1);
                fhhero.m_isMain          = mainHeroes.contains(playerId) && mainHeroes[playerId] == hero->m_subID;
                fhhero.m_questIdentifier = hero->m_questIdentifier;

                FHHeroData& destHero = fhhero.m_data;
                destHero.m_army.hero = Core::AdventureHero(m_heroIds[hero->m_subID]);
                destHero.m_hasExp    = hero->m_hasExp;
                if (destHero.m_hasExp) {
                    destHero.m_army.hero.experience = hero->m_exp;
                }
                destHero.m_hasPrimSkills = hero->m_primSkillSet.m_hasCustomPrimSkills;
                destHero.m_hasSpells     = hero->m_spellSet.m_hasCustomSpells;
                destHero.m_hasSecSkills  = hero->m_hasSecSkills;
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
                dest.m_wanderingHeroes.push_back(std::move(fhhero));
            } break;

            case MapObjectType::RANDOM_HERO:
            {
                assert(!"unsupported");
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
                const auto* monster = static_cast<const MapMonster*>(impl);
                FHMonster   fhMonster;
                fhMonster.m_order           = index;
                fhMonster.m_pos             = posFromH3M(obj.m_pos, -src.m_features->m_monstersMapXOffset);
                fhMonster.m_count           = monster->m_count;
                fhMonster.m_id              = m_unitIds[objTempl.m_subid];
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
                fhMonster.m_hasMessage = monster->m_hasMessage;
                if (monster->m_hasMessage) {
                    fhMonster.m_message = monster->m_message;
                    if (monster->m_artID != uint16_t(-1)) {
                        fhMonster.m_reward.artifacts.resize(1);
                        fhMonster.m_reward.artifacts[0].onlyArtifacts.push_back(m_artifactIds.at(monster->m_artID));
                    }
                    fhMonster.m_reward.resources = convertResources(monster->m_resourceSet.m_resourceAmount);
                }

                dest.m_objects.m_monsters.push_back(std::move(fhMonster));
            } break;
            case MapObjectType::OCEAN_BOTTLE:
            case MapObjectType::SIGN:
            {
                const auto* bottle = static_cast<const MapSignBottle*>(impl);

                if (!mappings.mapVisitable)
                    throw std::runtime_error("Unknown def for visitable:" + objDefEmbedded.id);

                FHSign fhVisitable;
                initCommon(fhVisitable);
                fhVisitable.m_text        = bottle->m_message;
                fhVisitable.m_visitableId = mappings.mapVisitable;
                dest.m_objects.m_signs.push_back(std::move(fhVisitable));
            } break;
            case MapObjectType::SEER_HUT:
            {
                const auto* hut = static_cast<const MapSeerHut*>(impl);

                auto* visitableId = mappings.mapVisitable;
                if (!visitableId)
                    throw std::runtime_error("Unknown def for seer hut:" + objDefEmbedded.id);

                FHQuestHut fhQuestHut;
                initCommon(fhQuestHut);
                fhQuestHut.m_visitableId = visitableId;
                fhQuestHut.m_questsOneTime.resize(hut->m_questsOneTime.size());
                fhQuestHut.m_questsRecurring.resize(hut->m_questsRecurring.size());

                for (size_t i = 0; i < hut->m_questsOneTime.size(); ++i) {
                    fhQuestHut.m_questsOneTime[i].m_reward = convertRewardHut(hut->m_questsOneTime[i]);
                    fhQuestHut.m_questsOneTime[i].m_quest  = convertQuest(hut->m_questsOneTime[i].m_quest);
                }

                dest.m_objects.m_questHuts.push_back(std::move(fhQuestHut));
            } break;
            case MapObjectType::WITCH_HUT:
            {
                const auto* hut         = static_cast<const MapWitchHut*>(impl);
                auto*       visitableId = mappings.mapVisitable;
                if (!visitableId)
                    throw std::runtime_error("Unknown def for witch hut:" + objDefEmbedded.id);

                FHSkillHut fhHut;
                initCommon(fhHut);
                fhHut.m_visitableId = visitableId;
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
                const auto* scholar = static_cast<const MapScholar*>(impl);

                auto* visitableId = mappings.mapVisitable;
                assert(visitableId);

                FHScholar fhScholar;
                initCommon(fhScholar);
                fhScholar.m_visitableId = visitableId;
                fhScholar.m_type        = scholar->m_bonusType == 0xff ? FHScholar::Type::Random : static_cast<FHScholar::Type>(scholar->m_bonusType);
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
                const auto* garison = static_cast<const MapGarison*>(impl);
                if (!mappings.mapVisitable)
                    throw std::runtime_error("Unknown def for visitable:" + objDefEmbedded.id);

                FHGarison fhVisitable;
                initCommon(fhVisitable);
                fhVisitable.m_player         = m_playerIds.at(garison->m_owner);
                fhVisitable.m_removableUnits = garison->m_removableUnits;
                fhVisitable.m_visitableId    = mappings.mapVisitable;
                fhVisitable.m_garison        = convertSquad(garison->m_garison);
                dest.m_objects.m_garisons.push_back(std::move(fhVisitable));

            } break;
            case MapObjectType::ARTIFACT:
            case MapObjectType::SPELL_SCROLL:
            {
                FHArtifact art;
                initCommon(art);
                if (type == MapObjectType::SPELL_SCROLL) {
                    const auto* artifact = static_cast<const MapArtifact*>(impl);
                    const auto* spell    = m_spellIds.at(artifact->m_spellId);
                    assert(spell);
                    art.m_id = m_database->artifacts()->find("sod.artifact." + spell->id); // @todo: constaint for prefix?
                    assert(art.m_id);
                } else {
                    assert(objTempl.m_subid != 0);
                    art.m_id = m_artifactIds[objTempl.m_subid];
                }
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
                const auto*      resource = static_cast<const MapResource*>(impl);
                FHRandomResource fhres;
                initCommon(fhres);
                fhres.m_amount = resource->m_amount;
                dest.m_objects.m_resourcesRandom.push_back(std::move(fhres));
            } break;
            case MapObjectType::RESOURCE:
            {
                const auto* resource = static_cast<const MapResource*>(impl);
                FHResource  fhres;
                initCommon(fhres);
                fhres.m_amount = resource->m_amount;
                fhres.m_id     = m_resourceIds[objTempl.m_subid];
                fhres.m_amount *= fhres.m_id->pileSize;
                assert(fhres.m_id);
                dest.m_objects.m_resources.push_back(std::move(fhres));
            } break;
            case MapObjectType::RANDOM_TOWN:
            case MapObjectType::TOWN:
            {
                const auto* town     = static_cast<const MapTown*>(impl);
                const auto  playerId = m_playerIds.at(town->m_playerOwner);
                FHTown      fhtown;
                initCommon(fhtown);
                fhtown.m_player    = playerId;
                fhtown.m_factionId = m_factionIds[objTempl.m_subid];
                assert(fhtown.m_factionId == mappings.factionTown);
                fhtown.m_questIdentifier    = town->m_questIdentifier;
                fhtown.m_hasFort            = town->m_hasFort;
                fhtown.m_spellResearch      = town->m_spellResearch;
                fhtown.m_hasCustomBuildings = town->m_hasCustomBuildings;
                fhtown.m_hasGarison         = town->m_hasGarison;
                if (mainTowns.contains(playerId) && mainTowns.at(playerId) == fhtown.m_pos)
                    fhtown.m_isMain = true;
                if (fhtown.m_hasCustomBuildings) {
                    for (size_t i = 0; i < town->m_builtBuildings.size(); ++i) {
                        if (town->m_builtBuildings[i])
                            fhtown.m_buildings.push_back(m_buildingIds[i]);
                    }
                }
                if (fhtown.m_hasGarison) {
                    fhtown.m_garison = convertSquad(town->m_garison);
                }
                dest.m_towns.push_back(std::move(fhtown));
            } break;
            case MapObjectType::ABANDONED_MINE:
            case MapObjectType::MINE:
            {
                if (objTempl.m_subid < 7) {
                    const auto* objOwner = static_cast<const MapObjectWithOwner*>(impl);
                    FHMine      mine;
                    initCommon(mine);
                    mine.m_player = m_playerIds.at(objOwner->m_owner);
                    mine.m_id     = mappings.resourceMine;
                    dest.m_objects.m_mines.push_back(std::move(mine));
                } else {
                    const auto*     objOwner = static_cast<const MapAbandonedMine*>(impl);
                    FHAbandonedMine mine;
                    initCommon(mine);
                    assert(!"Unsupported");
                    (void) objOwner;
                }
            } break;
            case MapObjectType::CREATURE_GENERATOR1:
            case MapObjectType::CREATURE_GENERATOR2:
            case MapObjectType::CREATURE_GENERATOR3:
            case MapObjectType::CREATURE_GENERATOR4:
            {
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
                const auto* objOwner = static_cast<const MapObjectWithOwner*>(impl);

                if (!mappings.mapVisitable)
                    throw std::runtime_error("Unknown def for visitable:" + objDefEmbedded.id);

                FHVisitableControlled fhVisitable;
                initCommon(fhVisitable);
                fhVisitable.m_player      = m_playerIds.at(objOwner->m_owner);
                fhVisitable.m_visitableId = mappings.mapVisitable;
                dest.m_objects.m_controlledVisitables.push_back(std::move(fhVisitable));

            } break;
            case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
            case MapObjectType::SHRINE_OF_MAGIC_GESTURE:
            case MapObjectType::SHRINE_OF_MAGIC_THOUGHT:
            {
                const auto* shrine      = static_cast<const MapShrine*>(impl);
                auto*       visitableId = mappings.mapVisitable;
                assert(visitableId);

                FHShrine fhShrine;
                initCommon(fhShrine);
                fhShrine.m_visitableId = visitableId;
                if (shrine->m_spell == 0xffU) {
                    switch (type) {
                        case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
                            fhShrine.m_randomLevel = objTempl.m_subid == 3 ? 4 : 1;
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
                const auto* pandora = static_cast<const MapPandora*>(impl);
                FHPandora   fhPandora;
                initCommon(fhPandora);
                fhPandora.m_reward = convertReward(pandora->m_reward);
                dest.m_objects.m_pandoras.push_back(std::move(fhPandora));
            } break;
            case MapObjectType::GRAIL:
            {
                const auto* grail = static_cast<const MapGrail*>(impl);
                (void) grail;
                assert(!"unsupported");
            } break;
            case MapObjectType::QUEST_GUARD:
            {
                const auto*  questGuard = static_cast<const MapQuestGuard*>(impl);
                FHQuestGuard fhQuestGuard;
                initCommon(fhQuestGuard);
                auto* visitableId = mappings.mapVisitable;
                if (!visitableId)
                    throw std::runtime_error("Unknown def for quest guard:" + objDefEmbedded.id);

                fhQuestGuard.m_visitableId = visitableId;

                fhQuestGuard.m_quest = convertQuest(questGuard->m_quest);
                dest.m_objects.m_questGuards.push_back(std::move(fhQuestGuard));
            } break;
            case MapObjectType::RANDOM_DWELLING:         //same as castle + level range  216
            case MapObjectType::RANDOM_DWELLING_LVL:     //same as castle, fixed level   217
            case MapObjectType::RANDOM_DWELLING_FACTION: //level range, fixed faction    218
            {
                const auto* mapDwelling = static_cast<const MapDwelling*>(impl);
                (void) mapDwelling;
                assert(!"unsupported");
            } break;

            case MapObjectType::HERO_PLACEHOLDER:
            {
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
                if (!skipped.contains(objTempl.m_id)) {
                    skipped.insert(objTempl.m_id);
                    Logger(Logger::Warning) << "Skipping unsupported object def: " << objDefEmbedded.id << " of type " << objTempl.m_id;
                    assert(!"unsupported");
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
        dest.m_customHeroes.push_back(std::move(destHero));
    }
    for (auto& event : src.m_globalEvents) {
        dest.m_globalEvents.push_back(convertEvent(event));
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
        if (stack.m_count && stack.m_id != uint16_t(-1))
            squad.stacks.push_back(Core::AdventureStack(m_unitIds[stack.m_id], stack.m_count));
        else
            squad.stacks.push_back(Core::AdventureStack());
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
        default:
            Logger(Logger::Warning) << "Unsupported mission type:" << int(quest.m_missionType);
            assert(!"Unsupported");
            break;
    }
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
    const MapTileSet& tileSet     = src.m_tiles;
    FHTileMap&        destTileMap = dest.m_tileMap;

    FloodFiller terrainFiller(&tileSet, &destTileMap);

    auto visitTerrain = [this, &terrainFiller, &tileSet, &dest](const MapTileH3M& tile, const FHPos& tilePos) {
        if (terrainFiller.m_zoned.contains(tilePos))
            return;

        auto tiles = terrainFiller.makeNewZone(tilePos, [&tile](const MapTileH3M& neighbourTile) -> bool {
            if (neighbourTile.m_terType != tile.m_terType)
                return true;
            return false;
        });

        FHZone fhZone;
        fhZone.m_tiles = std::move(tiles);
        for (auto& pos : fhZone.m_tiles) {
            auto terVariant = tileSet.get(pos.m_x, pos.m_y, pos.m_z).m_terView;
            fhZone.m_tilesVariants.push_back(terVariant);
        }
        fhZone.m_terrainId = m_terrainIds[tile.m_terType];
        assert(fhZone.m_terrainId);
        dest.m_zones.push_back(std::move(fhZone));
    };

    FloodFiller roadFiller(&tileSet, &destTileMap);

    auto visitRoad = [&roadFiller, &tileSet, &dest](const MapTileH3M& tile, const FHPos& tilePos) {
        if (tile.m_roadType == 0)
            return;

        if (roadFiller.m_zoned.contains(tilePos))
            return;

        auto tiles = roadFiller.makeNewZone(tilePos, [&tile](const MapTileH3M& neighbourTile) -> bool {
            return (neighbourTile.m_roadType != tile.m_roadType);
        });

        FHRoad fhRoad;
        fhRoad.m_tiles = std::move(tiles);
        for (auto& pos : fhRoad.m_tiles) {
            auto roadVariant = tileSet.get(pos.m_x, pos.m_y, pos.m_z).m_roadDir;
            fhRoad.m_tilesVariants.push_back(roadVariant);
        }
        fhRoad.m_type = static_cast<FHRoadType>(tile.m_roadType);
        dest.m_roads.push_back(std::move(fhRoad));
    };

    FloodFiller riverFiller(&tileSet, &destTileMap);

    auto visitRiver = [&riverFiller, &tileSet, &dest](const MapTileH3M& tile, const FHPos& tilePos) {
        if (tile.m_riverType == 0)
            return;

        if (riverFiller.m_zoned.contains(tilePos))
            return;

        auto tiles = riverFiller.makeNewZone(tilePos, [&tile](const MapTileH3M& neighbourTile) -> bool {
            return (neighbourTile.m_riverType != tile.m_riverType);
        });

        FHRiver fhRiver;
        fhRiver.m_tiles = std::move(tiles);
        for (auto& pos : fhRiver.m_tiles) {
            auto riverVariant = tileSet.get(pos.m_x, pos.m_y, pos.m_z).m_riverDir;
            fhRiver.m_tilesVariants.push_back(riverVariant);
        }
        fhRiver.m_type = static_cast<FHRiverType>(tile.m_riverType);
        dest.m_rivers.push_back(std::move(fhRiver));
    };
    for (int z = 0; z < dest.m_tileMap.m_depth; ++z) {
        for (int y = 0; y < dest.m_tileMap.m_height; ++y) {
            for (int x = 0; x < dest.m_tileMap.m_width; ++x) {
                auto&       tile = tileSet.get(x, y, z);
                const FHPos tilePos{ x, y, z };
                visitTerrain(tile, tilePos);
                visitRoad(tile, tilePos);
                visitRiver(tile, tilePos);
            }
        }
    }

    assert(terrainFiller.m_zoned.size() == dest.m_tileMap.totalSize());
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
