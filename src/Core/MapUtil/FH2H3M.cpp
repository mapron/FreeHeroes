/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "FH2H3M.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryHero.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryMapObstacle.hpp"
#include "LibraryMapVisitable.hpp"
#include "LibraryObjectDef.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryTerrain.hpp"

#include "MernelPlatform/Logger.hpp"

#define assume(cond) \
    if (!(cond)) \
        throw std::runtime_error(#cond " - violated, invalid FH map provided.");

namespace FreeHeroes {

class FH2H3MConverter::ObjectTemplateCache {
public:
    ObjectTemplateCache(const Core::IGameDatabase* database)
        : m_database(database)
    {}

    uint32_t add(ObjectTemplate def)
    {
        auto index = static_cast<uint32_t>(m_objectDefs.size());
        m_objectDefs.push_back(std::move(def));
        return index;
    }

    uint32_t add(Core::LibraryObjectDefConstPtr record, bool skipIndexCheck = false)
    {
        assert(record);
        if (!skipIndexCheck && m_index.contains(record))
            return m_index.at(record);

        ObjectTemplate res = makeTemplateFromRecord(record);
        if (m_replacements.contains(record)) {
            Core::LibraryObjectDef& replacement = m_replacements[record];
            res                                 = makeTemplateFromRecord(&replacement);
        }

        if (res.m_animationFile.find('.') == std::string::npos)
            res.m_animationFile += ".def";
        const size_t terrainsCount = m_database->terrains()->legacyOrderedIds().size();
        res.m_terrainsHard.resize(terrainsCount);
        res.m_terrainsSoft.resize(terrainsCount);

        const auto index = add(res);
        m_index[record]  = index;
        return index;
    }

    uint32_t addId(std::string defId)
    {
        auto* defsContainer = m_database->objectDefs();
        if (defId.ends_with(".def"))
            defId = defId.substr(0, defId.size() - 4);
        std::transform(defId.begin(), defId.end(), defId.begin(), [](unsigned char c) { return std::tolower(c); });
        return add(defsContainer->find(defId));
    }

    uint32_t addHero(Core::LibraryHeroConstPtr hero)
    {
        std::string id = hero->getAdventureSprite() + "e";
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
        return res;
    }

    std::map<Core::LibraryObjectDefConstPtr, Core::LibraryObjectDef> m_replacements;

    std::map<Core::LibraryObjectDefConstPtr, uint32_t> m_index;
    std::vector<ObjectTemplate>                        m_objectDefs;
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

    dest.m_format                 = src.m_version == Core::GameVersion::SOD ? MapFormat::SOD : MapFormat::HOTA3;
    dest.m_anyPlayers             = true;
    dest.m_tiles.m_size           = src.m_tileMap.m_width;
    dest.m_tiles.m_hasUnderground = src.m_tileMap.m_depth > 1;
    dest.m_tiles.updateSize();
    dest.prepareArrays();

    dest.m_hotaVer.m_allowSpecialWeeks = src.m_config.m_allowSpecialWeeks;
    dest.m_hotaVer.m_roundLimit        = src.m_config.m_hasRoundLimit ? src.m_config.m_roundLimit : 0xffffffffU;

    src.m_tileMap.eachPosTile([&dest](const FHPos& pos, const FHTileMap::Tile& tile) {
        auto& destTile          = dest.m_tiles.get(pos.m_x, pos.m_y, pos.m_z);
        destTile.m_terType      = static_cast<uint8_t>(tile.m_terrain->legacyId);
        destTile.m_terView      = tile.m_view;
        destTile.m_riverType    = static_cast<uint8_t>(tile.m_riverType);
        destTile.m_riverDir     = tile.m_riverView;
        destTile.m_roadType     = static_cast<uint8_t>(tile.m_roadType);
        destTile.m_roadDir      = tile.m_roadView;
        destTile.m_extTileFlags = 0;
        if (tile.m_flipHor)
            destTile.m_extTileFlags |= MapTile::TerrainFlipHor;
        if (tile.m_flipVert)
            destTile.m_extTileFlags |= MapTile::TerrainFlipVert;
        if (tile.m_roadFlipHor)
            destTile.m_extTileFlags |= MapTile::RoadFlipHor;
        if (tile.m_roadFlipVert)
            destTile.m_extTileFlags |= MapTile::RoadFlipVert;
        if (tile.m_riverFlipHor)
            destTile.m_extTileFlags |= MapTile::RiverFlipHor;
        if (tile.m_riverFlipVert)
            destTile.m_extTileFlags |= MapTile::RiverFlipVert;
        if (tile.m_coastal)
            destTile.m_extTileFlags |= MapTile::Coastal;
    });

    for (auto& [playerId, fhPlayer] : src.m_players) {
        auto  index    = static_cast<int>(playerId);
        auto& h3player = dest.m_players[index];

        h3player.m_canHumanPlay           = fhPlayer.m_humanPossible;
        h3player.m_canComputerPlay        = fhPlayer.m_aiPossible;
        h3player.m_generateHeroAtMainTown = fhPlayer.m_generateHeroAtMainTown;

        uint16_t factionsBitmask = 0;
        for (Core::LibraryFactionConstPtr faction : fhPlayer.m_startingFactions) {
            assume(faction != nullptr);
            factionsBitmask |= 1U << uint32_t(faction->legacyId);
        }
        h3player.m_allowedFactionsBitmask = factionsBitmask;
    }
    for (auto& bit : dest.m_allowedHeroes)
        bit = 1;

    ObjectTemplateCache tmplCache(m_database);
    tmplCache.m_replacements = src.m_defReplacements;

    for (auto* def : src.m_initialObjectDefs)
        tmplCache.add(def, true);

    tmplCache.addId("avwmrnd0");
    tmplCache.addId("avlholg0");

    for (auto& fhTown : src.m_towns) {
        auto playerIndex = static_cast<int>(fhTown.m_player);
        if (playerIndex >= 0) {
            auto& h3player = dest.m_players[playerIndex];
            if (fhTown.m_isMain) {
                const int townGateOffset            = 2;
                h3player.m_hasMainTown              = true;
                h3player.m_posOfMainTown            = int3fromPos(fhTown.m_pos, -townGateOffset);
                h3player.m_generatedHeroTownFaction = static_cast<uint8_t>(fhTown.m_factionId->legacyId);
            }
        }
        auto cas1               = std::make_unique<MapTown>(dest.m_features);
        cas1->m_playerOwner     = playerIndex;
        cas1->m_hasFort         = fhTown.m_hasFort;
        cas1->m_questIdentifier = fhTown.m_questIdentifier;
        cas1->m_spellResearch   = fhTown.m_spellResearch;

        //cas1->m_formation       = 0xCC;
        cas1->prepareArrays();
        auto* libraryFaction = fhTown.m_factionId;
        assert(libraryFaction);

        auto* def = libraryFaction->objectDefs.get(fhTown.m_defIndex);

        dest.m_objects.push_back(Object{ .m_order = fhTown.m_order, .m_pos = int3fromPos(fhTown.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(cas1) });
    }

    for (auto& fhHero : src.m_wanderingHeroes) {
        auto  playerIndex = static_cast<int>(fhHero.m_player);
        auto* libraryHero = fhHero.m_data.m_army.hero.library;
        assert(libraryHero);

        const uint8_t heroId = libraryHero->legacyId;

        if (playerIndex >= 0) {
            auto& h3player = dest.m_players[playerIndex];
            if (fhHero.m_isMain) {
                h3player.m_mainCustomHeroId         = heroId;
                h3player.m_generatedHeroTownFaction = 0; // @todo:
            }
            h3player.m_heroesNames.push_back(SHeroName{ .m_heroId = heroId, .m_heroName = "" });
        }

        auto hero               = std::make_unique<MapHero>(dest.m_features);
        hero->m_playerOwner     = playerIndex;
        hero->m_subID           = heroId;
        hero->m_questIdentifier = fhHero.m_questIdentifier;
        hero->prepareArrays();
        {
            hero->m_hasExp                             = fhHero.m_data.m_hasExp;
            hero->m_hasSecSkills                       = fhHero.m_data.m_hasSecSkills;
            hero->m_primSkillSet.m_hasCustomPrimSkills = fhHero.m_data.m_hasPrimSkills;
            hero->m_spellSet.m_hasCustomSpells         = fhHero.m_data.m_hasSpells;
            if (hero->m_hasExp)
                hero->m_exp = static_cast<int32_t>(fhHero.m_data.m_army.hero.experience);

            //assert(!hero->m_hasSecSkills);
            //assert(!hero->m_spellSet.m_hasCustomSpells);
            //assert(!hero->m_primSkillSet.m_hasCustomPrimSkills);
        }
        dest.m_allowedHeroes[heroId] = 0;
        if (playerIndex < 0) {
            dest.m_objects.push_back(Object{ .m_order = fhHero.m_order, .m_pos = int3fromPos(fhHero.m_pos), .m_defnum = tmplCache.addId("avxprsn0"), .m_impl = std::move(hero) });
        } else {
            dest.m_objects.push_back(Object{ .m_order = fhHero.m_order, .m_pos = int3fromPos(fhHero.m_pos, +1), .m_defnum = tmplCache.addHero(libraryHero), .m_impl = std::move(hero) });
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

    for (const auto& heroId : src.m_disabledHeroes) {
        const auto legacyId            = (heroId)->legacyId;
        dest.m_allowedHeroes[legacyId] = 0;
    }

    if (src.m_version == Core::GameVersion::HOTA) {
        // these artifact ids are just unexistent. @todo: do I really need this logic?
        dest.m_allowedArtifacts[145] = 1;
        dest.m_allowedArtifacts[144] = 1;
    }
    for (const auto& artId : src.m_disabledArtifacts) {
        const auto legacyId               = (artId)->legacyId;
        dest.m_allowedArtifacts[legacyId] = 0;
    }

    for (const auto& spellId : src.m_disabledSpells) {
        const auto legacyId            = (spellId)->legacyId;
        dest.m_allowedSpells[legacyId] = 0;
    }
    for (const auto& secSkillId : src.m_disabledSkills) {
        const auto legacyId               = (secSkillId)->legacyId;
        dest.m_allowedSecSkills[legacyId] = 0;
    }
    for (auto& customHero : src.m_customHeroes) {
        const auto legacyId = customHero.m_army.hero.library->legacyId;
        auto&      destHero = dest.m_customHeroData[legacyId];
        destHero.prepareArrays();
        destHero.m_enabled                            = true;
        destHero.m_hasExp                             = customHero.m_hasExp;
        destHero.m_hasCustomBio                       = customHero.m_hasCustomBio;
        destHero.m_hasSkills                          = customHero.m_hasSecSkills;
        destHero.m_primSkillSet.m_hasCustomPrimSkills = customHero.m_hasPrimSkills;
        destHero.m_spellSet.m_hasCustomSpells         = customHero.m_hasSpells;
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
    }

    for (auto& fhRes : src.m_objects.m_resources) {
        if (fhRes.m_type == FHResource::Type::Resource) {
            auto res      = std::make_unique<MapResource>(dest.m_features);
            res->m_amount = fhRes.m_amount;

            dest.m_objects.push_back(Object{ .m_order = fhRes.m_order, .m_pos = int3fromPos(fhRes.m_pos), .m_defnum = tmplCache.add(fhRes.m_id->objectDefs.get({})), .m_impl = std::move(res) });
        } else {
            auto* def = fhRes.m_visitableId->objectDefs.get(fhRes.m_defIndex);
            dest.m_objects.push_back(Object{ .m_order = fhRes.m_order, .m_pos = int3fromPos(fhRes.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::make_unique<MapObjectSimple>(dest.m_features) });
        }
    }

    for (auto& fhRes : src.m_objects.m_resourcesRandom) {
        auto res      = std::make_unique<MapResource>(dest.m_features);
        res->m_amount = fhRes.m_amount;
        dest.m_objects.push_back(Object{ .m_order = fhRes.m_order, .m_pos = int3fromPos(fhRes.m_pos), .m_defnum = tmplCache.addId("avtrndm0"), .m_impl = std::move(res) });
    }

    for (auto& fhArt : src.m_objects.m_artifacts) {
        auto art = std::make_unique<MapArtifact>(dest.m_features, false);
        if (fhArt.m_id->scrollSpell) {
            art->m_spellId = fhArt.m_id->scrollSpell->legacyId;
            art->m_isSpell = true;
        }
        dest.m_objects.push_back(Object{ .m_order = fhArt.m_order, .m_pos = int3fromPos(fhArt.m_pos), .m_defnum = tmplCache.add(fhArt.m_id->objectDefs.get({})), .m_impl = std::move(art) });
    }
    for (auto& fhArt : src.m_objects.m_artifactsRandom) {
        auto        art = std::make_unique<MapArtifact>(dest.m_features, false);
        std::string id  = "";
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

        dest.m_objects.push_back(Object{ .m_order = fhArt.m_order, .m_pos = int3fromPos(fhArt.m_pos), .m_defnum = tmplCache.addId(id), .m_impl = std::move(art) });
    }

    for (auto& fhMon : src.m_objects.m_monsters) {
        auto monster               = std::make_unique<MapMonster>(dest.m_features);
        monster->m_count           = static_cast<uint16_t>(fhMon.m_count);
        monster->m_questIdentifier = fhMon.m_questIdentifier;
        if (fhMon.m_agressionMax == fhMon.m_agressionMin) {
            if (fhMon.m_agressionMax == 0)
                monster->m_joinAppeal = 0;
            else if (fhMon.m_agressionMax == 10)
                monster->m_joinAppeal = 4;
            else {
                monster->m_joinAppeal     = 5;
                monster->m_agressionExact = fhMon.m_agressionMax;
            }
        } else if (fhMon.m_agressionMin == 1 && fhMon.m_agressionMax == 7) {
            monster->m_joinAppeal = 1;
        } else if (fhMon.m_agressionMin == 1 && fhMon.m_agressionMax == 10) {
            monster->m_joinAppeal = 2;
        } else if (fhMon.m_agressionMin == 4 && fhMon.m_agressionMax == 10) {
            monster->m_joinAppeal = 3;
        } else {
            throw std::runtime_error("unsupported monster appeal");
        }
        monster->m_joinOnlyForMoney = fhMon.m_joinOnlyForMoney;
        monster->m_joinPercent      = fhMon.m_joinPercent;
        if (fhMon.m_upgradedStack == FHMonster::UpgradedStack::No)
            monster->m_upgradedStack = 0;
        if (fhMon.m_upgradedStack == FHMonster::UpgradedStack::Yes)
            monster->m_upgradedStack = 1;

        auto* def = fhMon.m_id->objectDefs.get({});
        dest.m_objects.push_back(Object{ .m_order = fhMon.m_order, .m_pos = int3fromPos(fhMon.m_pos, dest.m_features->m_monstersMapXOffset), .m_defnum = tmplCache.add(def), .m_impl = std::move(monster) });
    }

    for (auto& fhDwelling : src.m_objects.m_dwellings) {
        std::unique_ptr<IMapObject> impl;
        if (fhDwelling.m_id->hasPlayer) {
            auto dwell     = std::make_unique<MapObjectWithOwner>(dest.m_features);
            dwell->m_owner = static_cast<uint8_t>(fhDwelling.m_player);
            impl           = std::move(dwell);
        } else {
            impl = std::make_unique<MapObjectSimple>(dest.m_features);
        }

        auto* def = fhDwelling.m_id->objectDefs.get(fhDwelling.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhDwelling.m_order, .m_pos = int3fromPos(fhDwelling.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(impl) });
    }
    for (auto& fhMine : src.m_objects.m_mines) {
        auto mine     = std::make_unique<MapObjectWithOwner>(dest.m_features);
        mine->m_owner = static_cast<uint8_t>(fhMine.m_player);

        auto* def = fhMine.m_id->minesDefs.get(fhMine.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhMine.m_order, .m_pos = int3fromPos(fhMine.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(mine) });
    }

    for (auto& fhBank : src.m_objects.m_banks) {
        auto bank = std::make_unique<MapObjectCreatureBank>(dest.m_features);

        if (fhBank.m_guardsVariant != -1) {
            bank->m_content = fhBank.m_guardsVariant;
            if (fhBank.m_id->upgradedStackIndex != -1) {
                if (fhBank.m_upgradedStack == FHBank::UpgradedStack::No)
                    bank->m_upgraded = 0;
                if (fhBank.m_upgradedStack == FHBank::UpgradedStack::Yes)
                    bank->m_upgraded = 1;
            }
        }

        auto* def = fhBank.m_id->objectDefs.get(fhBank.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhBank.m_order, .m_pos = int3fromPos(fhBank.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(bank) });
    }
    for (auto& fhObstacle : src.m_objects.m_obstacles) {
        auto obj = std::make_unique<MapObjectSimple>(dest.m_features);

        auto* def = fhObstacle.m_id->objectDefs.get(fhObstacle.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhObstacle.m_order, .m_pos = int3fromPos(fhObstacle.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(obj) });
    }
    for (auto& fhVisitable : src.m_objects.m_visitables) {
        auto obj = std::make_unique<MapObjectSimple>(dest.m_features);

        auto* def = fhVisitable.m_visitableId->objectDefs.get(fhVisitable.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhVisitable.m_order, .m_pos = int3fromPos(fhVisitable.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(obj) });
    }
    for (auto& fhShrine : src.m_objects.m_shrines) {
        auto obj = std::make_unique<MapShrine>(dest.m_features);
        if (fhShrine.m_spellId)
            obj->m_spell = static_cast<uint8_t>(fhShrine.m_spellId->legacyId);
        else
            obj->m_spell = 0xffU;

        auto* def = fhShrine.m_visitableId->objectDefs.get(fhShrine.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhShrine.m_order, .m_pos = int3fromPos(fhShrine.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(obj) });
    }
    for (auto& fhSkillHut : src.m_objects.m_skillHuts) {
        auto obj = std::make_unique<MapWitchHut>(dest.m_features);
        for (auto* allowedSkill : fhSkillHut.m_skillIds)
            obj->m_allowedSkills[allowedSkill->legacyId] = 1;

        auto* def = fhSkillHut.m_visitableId->objectDefs.get(fhSkillHut.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhSkillHut.m_order, .m_pos = int3fromPos(fhSkillHut.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(obj) });
    }
    for (auto& fhScholar : src.m_objects.m_scholars) {
        auto obj         = std::make_unique<MapScholar>(dest.m_features);
        obj->m_bonusType = fhScholar.m_type == FHScholar::Type::Random ? 0xffU : static_cast<uint8_t>(fhScholar.m_type);
        if (fhScholar.m_type == FHScholar::Type::Primary)
            obj->m_bonusId = static_cast<uint8_t>(fhScholar.m_primaryType);
        else if (fhScholar.m_type == FHScholar::Type::Secondary)
            obj->m_bonusId = static_cast<uint8_t>(fhScholar.m_skillId->legacyId);
        else if (fhScholar.m_type == FHScholar::Type::Spell)
            obj->m_bonusId = static_cast<uint8_t>(fhScholar.m_spellId->legacyId);

        auto* def = fhScholar.m_visitableId->objectDefs.get(fhScholar.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhScholar.m_order, .m_pos = int3fromPos(fhScholar.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(obj) });
    }

    for (auto& fhQuestHut : src.m_objects.m_questHuts) {
        auto obj = std::make_unique<MapSeerHut>(dest.m_features);
        convertRewardHut(fhQuestHut.m_reward, obj.get());
        convertQuest(fhQuestHut.m_quest, obj->m_quest);

        auto* def = fhQuestHut.m_visitableId->objectDefs.get(fhQuestHut.m_defIndex);
        dest.m_objects.push_back(Object{ .m_order = fhQuestHut.m_order, .m_pos = int3fromPos(fhQuestHut.m_pos), .m_defnum = tmplCache.add(def), .m_impl = std::move(obj) });
    }

    for (auto& fhPandora : src.m_objects.m_pandoras) {
        auto obj = std::make_unique<MapPandora>(dest.m_features);
        convertReward(fhPandora.m_reward, obj->m_reward);

        dest.m_objects.push_back(Object{ .m_order = fhPandora.m_order, .m_pos = int3fromPos(fhPandora.m_pos), .m_defnum = tmplCache.addId("ava0128"), .m_impl = std::move(obj) });
    }

    dest.m_objectDefs = std::move(tmplCache.m_objectDefs);

    std::sort(dest.m_objects.begin(), dest.m_objects.end(), [](const auto& lh, const auto& rh) { return lh.m_order < rh.m_order; });
}

std::vector<uint32_t> FH2H3MConverter::convertResource(const Core::ResourceAmount& amount) const
{
    std::vector<uint32_t> res(7);

    res[0] = amount.wood;
    res[1] = amount.mercury;
    res[2] = amount.ore;
    res[3] = amount.sulfur;
    res[4] = amount.crystal;
    res[5] = amount.gems;
    res[6] = amount.gold;
    return res;
}
void FH2H3MConverter::convertReward(const Core::Reward& fhReward, MapReward& reward) const
{
    reward.m_gainedExp  = fhReward.gainedExp;
    reward.m_manaDiff   = fhReward.manaDiff;
    reward.m_luckDiff   = fhReward.rngBonus.luck;
    reward.m_moraleDiff = fhReward.rngBonus.morale;

    reward.m_resourceSet.m_resourceAmount = convertResource(fhReward.resources);
    reward.m_primSkillSet.m_prim          = convertPrimaryStats(fhReward.statBonus);

    // @todo: m_secSkills
    // @todo: m_artifacts
    for (auto* spell : fhReward.spells.onlySpells)
        reward.m_spells.push_back(static_cast<uint8_t>(spell->legacyId));

    reward.m_creatures.m_stacks = convertStacks(fhReward.units);
}

void FH2H3MConverter::convertRewardHut(const Core::Reward& fhReward, MapSeerHut* hut) const
{
    if (fhReward.gainedExp) {
        hut->m_reward = MapSeerHut::RewardType::EXPERIENCE;
        hut->m_rVal   = static_cast<uint32_t>(fhReward.gainedExp);
    }
    if (fhReward.manaDiff) {
        hut->m_reward = MapSeerHut::RewardType::MANA_POINTS;
        hut->m_rVal   = static_cast<uint32_t>(fhReward.manaDiff);
    }
    if (fhReward.rngBonus.morale) {
        hut->m_reward = MapSeerHut::RewardType::MORALE_BONUS;
        hut->m_rVal   = static_cast<uint32_t>(fhReward.rngBonus.morale);
    }
    if (fhReward.rngBonus.luck) {
        hut->m_reward = MapSeerHut::RewardType::LUCK_BONUS;
        hut->m_rVal   = static_cast<uint32_t>(fhReward.rngBonus.luck);
    }
    if (fhReward.resources.nonEmptyAmount()) {
        hut->m_reward = MapSeerHut::RewardType::RESOURCES;
        auto apply    = [&hut](int id, int res) {
            if (res == 0)
                return;
            hut->m_rVal = static_cast<uint32_t>(res);
            hut->m_rID  = static_cast<uint32_t>(id);
        };

        apply(0, fhReward.resources.wood);
        apply(1, fhReward.resources.mercury);
        apply(2, fhReward.resources.ore);
        apply(3, fhReward.resources.sulfur);
        apply(4, fhReward.resources.crystal);
        apply(5, fhReward.resources.gems);
        apply(6, fhReward.resources.gold);
    }

    auto applySkill = [&hut](int id, int val) {
        if (val == 0)
            return;
        hut->m_reward = MapSeerHut::RewardType::PRIMARY_SKILL;
        hut->m_rVal   = static_cast<uint32_t>(val);
        hut->m_rID    = static_cast<uint32_t>(id);
    };

    applySkill(0, fhReward.statBonus.ad.attack);
    applySkill(1, fhReward.statBonus.ad.defense);
    applySkill(2, fhReward.statBonus.magic.spellPower);
    applySkill(3, fhReward.statBonus.magic.intelligence);

    if (!fhReward.secSkills.empty()) {
        hut->m_reward = MapSeerHut::RewardType::SECONDARY_SKILL;
        hut->m_rID    = fhReward.secSkills[0].skill->legacyId;
        hut->m_rVal   = fhReward.secSkills[0].level + 1;
    }
    if (!fhReward.artifacts.empty()) {
        hut->m_reward = MapSeerHut::RewardType::ARTIFACT;
        hut->m_rID    = fhReward.artifacts[0].onlyArtifacts[0]->legacyId;
    }
    if (!fhReward.spells.isDefault()) {
        hut->m_reward = MapSeerHut::RewardType::SPELL;
        hut->m_rID    = fhReward.spells.onlySpells[0]->legacyId;
    }
    if (!fhReward.units.empty()) {
        hut->m_reward = MapSeerHut::RewardType::CREATURE;
        hut->m_rID    = fhReward.units[0].unit->legacyId;
        hut->m_rVal   = fhReward.units[0].count;
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
            quest.m_7resources  = convertResource(fhQuest.m_resources);
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
        default:
            assert(!"Unsupported");
            break;
    }
}

}
