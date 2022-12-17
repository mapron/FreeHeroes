/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "H3M2FH.hpp"

#include "FHMap.hpp"
#include "H3MMap.hpp"

#include "IGameDatabase.hpp"

#include "LibraryDwelling.hpp"
#include "LibraryFaction.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryHero.hpp"
#include "LibraryTerrain.hpp"

namespace FreeHeroes {

namespace {

FHPos posFromH3M(H3Pos pos, int xoffset = 0)
{
    return { (uint32_t) (pos.m_x + xoffset), (uint32_t) pos.m_y, pos.m_z };
}

FHPlayerId makePlayerId(int h3Id)
{
    if (h3Id >= 0 && h3Id <= 7)
        return static_cast<FHPlayerId>(h3Id);
    if (h3Id == -1 || h3Id == 255)
        return FHPlayerId::None;
    return FHPlayerId::Invalid;
}

}

void convertH3M2FH(const H3Map& src, FHMap& dest, const Core::IGameDatabase* database)
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

    auto*      factionsContainer = database->factions();
    const auto factionIds        = factionsContainer->legacyOrderedRecords();

    const auto heroIds     = database->heroes()->legacyOrderedRecords();
    const auto artIds      = database->artifacts()->legacyOrderedRecords();
    const auto spellIds    = database->spells()->legacyOrderedRecords();
    const auto secSkillIds = database->secSkills()->legacyOrderedRecords();
    const auto terrainIds  = database->terrains()->legacyOrderedRecords();
    const auto resIds      = database->resources()->legacyOrderedRecords();
    const auto unitIds     = database->units()->legacyOrderedRecords();

    std::map<std::string, std::pair<Core::LibraryDwellingConstPtr, int>> dwellMap;
    {
        for (auto* dwelling : database->dwellings()->records()) {
            for (int i = 0, cnt = dwelling->mapObjectDefs.size(); i < cnt; ++i) {
                dwellMap[dwelling->mapObjectDefs[i]->id] = { dwelling, i };
            }
        }
    }

    std::map<FHPlayerId, FHPos>   mainTowns;
    std::map<FHPlayerId, uint8_t> mainHeroes;

    for (int index = 0; const PlayerInfo& playerInfo : src.m_players) {
        const auto playerId = makePlayerId(index++);
        auto&      fhPlayer = dest.m_players[playerId];

        fhPlayer.m_aiPossible    = playerInfo.m_canComputerPlay;
        fhPlayer.m_humanPossible = playerInfo.m_canHumanPlay;

        if (playerInfo.m_hasMainTown) {
            mainTowns[playerId] = posFromH3M(playerInfo.m_posOfMainTown, +2);
        }
        if (playerInfo.m_mainCustomHeroId != 0xff) {
            mainHeroes[playerId] = playerInfo.m_mainCustomHeroId;
        }

        const uint16_t factionsBitmask = playerInfo.m_allowedFactionsBitmask;
        for (auto* faction : factionsContainer->records()) {
            if (faction->legacyId < 0)
                continue;

            if (factionsBitmask & (1U << uint32_t(faction->legacyId)))
                fhPlayer.m_startingFactions.push_back(faction);
        }
    }

    for (int index = 0; const Object& obj : src.m_objects) {
        const IMapObject*     impl         = obj.m_impl.get();
        const ObjectTemplate& objTempl     = src.m_objectDefs[obj.m_defnum];
        std::string           defObjectKey = objTempl.m_animationFile;
        {
            if (defObjectKey.ends_with(".def"))
                defObjectKey = defObjectKey.substr(0, defObjectKey.size() - 4);
            std::transform(defObjectKey.begin(), defObjectKey.end(), defObjectKey.begin(), [](unsigned char c) { return std::tolower(c); });
        }
        MapObjectType type = static_cast<MapObjectType>(objTempl.m_id);
        switch (type) {
            case MapObjectType::EVENT:
            {
                const auto* event = static_cast<const MapEvent*>(impl);
                assert(0 && event);
            } break;
            case MapObjectType::HERO:
            case MapObjectType::RANDOM_HERO:
            case MapObjectType::PRISON:
            {
                const auto* hero = static_cast<const MapHero*>(impl);

                const auto playerId = makePlayerId(hero->m_playerOwner);
                FHHero     fhhero;
                fhhero.m_player          = playerId;
                fhhero.m_order           = index;
                fhhero.m_pos             = posFromH3M(obj.m_pos, -1);
                fhhero.m_id              = heroIds[hero->m_subID];
                fhhero.m_isMain          = mainHeroes.contains(playerId) && mainHeroes[playerId] == hero->m_subID;
                fhhero.m_questIdentifier = hero->m_questIdentifier;
                dest.m_wanderingHeroes.push_back(fhhero);
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
                fhMonster.m_order = index;
                fhMonster.m_pos   = posFromH3M(obj.m_pos, -src.m_features->m_monstersMapXOffset);
                fhMonster.m_count = monster->m_count;
                fhMonster.m_id    = unitIds[objTempl.m_subid];
                dest.m_objects.m_monsters.push_back(fhMonster);
            } break;
            case MapObjectType::OCEAN_BOTTLE:
            case MapObjectType::SIGN:
            {
                const auto* bottle = static_cast<const MapSignBottle*>(impl);
                assert(0 && bottle);
            } break;
            case MapObjectType::SEER_HUT:
            {
                const auto* hut = static_cast<const MapSeerHut*>(impl);
                assert(1 && hut);
            } break;
            case MapObjectType::WITCH_HUT:
            {
                const auto* hut = static_cast<const MapWitchHut*>(impl);
                assert(1 && hut);
            } break;
            case MapObjectType::SCHOLAR:
            {
                const auto* scholar = static_cast<const MapScholar*>(impl);
                assert(0 && scholar);
            } break;
            case MapObjectType::GARRISON:
            case MapObjectType::GARRISON2:
            {
                const auto* garison = static_cast<const MapGarison*>(impl);
                assert(0 && garison);
            } break;
            case MapObjectType::ARTIFACT:
            case MapObjectType::RANDOM_ART:
            case MapObjectType::RANDOM_TREASURE_ART:
            case MapObjectType::RANDOM_MINOR_ART:
            case MapObjectType::RANDOM_MAJOR_ART:
            case MapObjectType::RANDOM_RELIC_ART:
            case MapObjectType::SPELL_SCROLL:
            {
                const auto* artifact = static_cast<const MapArtifact*>(impl);
                assert(!artifact->m_isSpell);
                if (artifact->m_isSpell) {
                } else {
                    FHArtifact art;
                    art.m_order = index;
                    art.m_pos   = posFromH3M(obj.m_pos);
                    art.m_id    = artIds[objTempl.m_subid];
                    dest.m_objects.m_artifacts.push_back(art);
                }
            } break;
            case MapObjectType::RANDOM_RESOURCE:
            case MapObjectType::RESOURCE:
            {
                const auto* resource = static_cast<const MapResource*>(impl);
                FHResource  fhres;
                fhres.m_order  = index;
                fhres.m_pos    = posFromH3M(obj.m_pos);
                fhres.m_amount = resource->m_amount;
                fhres.m_id     = resIds[objTempl.m_subid];
                assert(fhres.m_id);
                dest.m_objects.m_resources.push_back(fhres);
            } break;
            case MapObjectType::TREASURE_CHEST:
            {
                FHResource fhres;
                fhres.m_order  = index;
                fhres.m_pos    = posFromH3M(obj.m_pos);
                fhres.m_amount = 0;
                fhres.m_id     = nullptr;
                fhres.m_type   = FHResource::Type::TreasureChest;
                dest.m_objects.m_resources.push_back(fhres);
            } break;
            case MapObjectType::RANDOM_TOWN:
            case MapObjectType::TOWN:
            {
                const auto* town     = static_cast<const MapTown*>(impl);
                const auto  playerId = makePlayerId(town->m_playerOwner);
                FHTown      fhtown;
                fhtown.m_player          = playerId;
                fhtown.m_order           = index;
                fhtown.m_pos             = posFromH3M(obj.m_pos);
                fhtown.m_factionId       = factionIds[objTempl.m_subid];
                fhtown.m_questIdentifier = town->m_questIdentifier;
                fhtown.m_hasFort         = town->m_hasFort;
                fhtown.m_spellResearch   = town->m_spellResearch;
                fhtown.m_defFile         = objTempl.m_animationFile;
                if (mainTowns.contains(playerId) && mainTowns.at(playerId) == fhtown.m_pos)
                    fhtown.m_isMain = true;
                dest.m_towns.push_back(fhtown);
            } break;
            case MapObjectType::MINE:
            case MapObjectType::ABANDONED_MINE:
            case MapObjectType::CREATURE_GENERATOR1:
            case MapObjectType::CREATURE_GENERATOR2:
            case MapObjectType::CREATURE_GENERATOR3:
            case MapObjectType::CREATURE_GENERATOR4:
            case MapObjectType::SHIPYARD:
            case MapObjectType::LIGHTHOUSE:
            {
                const auto* objOwner = static_cast<const MapObjectWithOwner*>(impl);
                FHDwelling  dwelling;
                const auto [id, variant] = dwellMap.at(defObjectKey);
                dwelling.m_id            = id;
                dwelling.m_variant       = variant;
                dwelling.m_order         = index;
                dwelling.m_pos           = posFromH3M(obj.m_pos);
                dwelling.m_player        = makePlayerId(objOwner->m_owner);
                dest.m_objects.m_dwellings.push_back(dwelling);

            } break;
            case MapObjectType::SHRINE_OF_MAGIC_INCANTATION:
            case MapObjectType::SHRINE_OF_MAGIC_GESTURE:
            case MapObjectType::SHRINE_OF_MAGIC_THOUGHT:
            {
                const auto* shrine = static_cast<const MapShrine*>(impl);
                assert(1 && shrine);
            } break;
            case MapObjectType::PANDORAS_BOX:
            {
                const auto* pandora = static_cast<const MapPandora*>(impl);
                assert(0 && pandora);
            } break;
            case MapObjectType::GRAIL:
            {
                const auto* grail = static_cast<const MapGrail*>(impl);
                assert(0 && grail);
            } break;
            case MapObjectType::QUEST_GUARD:
            {
                const auto* questGuard = static_cast<const MapQuestGuard*>(impl);
                assert(0 && questGuard);
            } break;
            case MapObjectType::RANDOM_DWELLING:         //same as castle + level range  216
            case MapObjectType::RANDOM_DWELLING_LVL:     //same as castle, fixed level   217
            case MapObjectType::RANDOM_DWELLING_FACTION: //level range, fixed faction    218
            {
                const auto* mapDwelling = static_cast<const MapDwelling*>(impl);
                assert(0 && mapDwelling);
            } break;

            case MapObjectType::HERO_PLACEHOLDER:
            {
                assert(!"Unsupported");
            } break;
            case MapObjectType::CREATURE_BANK:
            case MapObjectType::DERELICT_SHIP:
            case MapObjectType::DRAGON_UTOPIA:
            case MapObjectType::CRYPT:
            case MapObjectType::SHIPWRECK:
            {
                const auto* bank = static_cast<const MapObjectCreatureBank*>(impl);
                assert(1 && bank);
            } break;
            default:
            {
                // simple object.
            } break;
        }

        index++;
    }

    for (int index = 0; auto& allowedFlag : src.m_allowedHeroes) {
        const auto& heroId = heroIds[index++];
        if (!allowedFlag)
            dest.m_disabledHeroes.push_back(heroId);
    }

    for (int index = 0; auto& allowedFlag : src.m_allowedArtifacts) {
        const auto& artId = artIds[index++];
        if (!allowedFlag && artId)
            dest.m_disabledArtifacts.push_back(artId);
    }

    for (int index = 0; auto& allowedFlag : src.m_allowedSpells) {
        const auto& spellId = spellIds[index++];
        if (!allowedFlag)
            dest.m_disabledSpells.push_back(spellId);
    }
    for (int index = 0; auto& allowedFlag : src.m_allowedSecSkills) {
        const auto& secSkillId = secSkillIds[index++];
        if (!allowedFlag)
            dest.m_disabledSkills.push_back(secSkillId);
    }

    for (int index = 0; auto& customHero : src.m_customHeroData) {
        const auto& heroId = heroIds[index++];
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
                const auto* secSkillId = secSkillIds[sk.m_id];
                skillList.push_back({ secSkillId, sk.m_level });
            }
        }
        if (destHero.m_hasPrimSkills) {
            auto& prim                                              = customHero.m_primSkillSet.m_primSkills;
            destHero.m_army.hero.currentBasePrimary.ad.asTuple()    = std::tie(prim[0], prim[1]);
            destHero.m_army.hero.currentBasePrimary.magic.asTuple() = std::tie(prim[2], prim[3]);
        }
        dest.m_customHeroes.push_back(std::move(destHero));
    }

    std::set<FHPos> zoned;
    auto            defTerrainType = src.m_tiles.get(0, 0, 0).m_terType;
    dest.m_defaultTerrain          = terrainIds[defTerrainType];

    if (1) {
        dest.m_defaultTerrain = database->terrains()->find("hota.terrain.wasteland");
        defTerrainType        = 0xff;
    }

    auto fillAdjucent = [&zoned, &dest, &src, defTerrainType](const FHPos& current, uint8_t curType, const std::set<FHPos>& exclude, std::set<FHPos>& result) {
        auto addToResult = [&zoned, &result, &exclude, &src, &current, curType, defTerrainType](int dx, int dy) {
            const FHPos neighbour{ current.m_x + dx, current.m_y + dy, current.m_z };
            auto&       neighbourTile = src.m_tiles.get(neighbour.m_x, neighbour.m_y, neighbour.m_z);
            if (neighbourTile.m_terType == defTerrainType || neighbourTile.m_terType != curType)
                return;
            if (zoned.contains(neighbour))
                return;
            if (exclude.contains(neighbour))
                return;
            result.insert(neighbour);
        };
        if (current.m_x < dest.m_tileMap.m_width - 1)
            addToResult(+1, 0);
        if (current.m_y < dest.m_tileMap.m_height - 1)
            addToResult(0, +1);
        if (current.m_x > 0)
            addToResult(-1, 0);
        if (current.m_y > 0)
            addToResult(0, -1);
    };

    for (uint8_t z = 0; z < dest.m_tileMap.m_depth; ++z) {
        for (uint32_t y = 0; y < dest.m_tileMap.m_height; ++y) {
            for (uint32_t x = 0; x < dest.m_tileMap.m_width; ++x) {
                auto& tile = src.m_tiles.get(x, y, z);
                if (tile.m_terType == defTerrainType)
                    continue;

                const FHPos tilePos{ x, y, z };
                if (zoned.contains(tilePos))
                    continue;

                // now we create new zone using flood-fill;
                std::set<FHPos> newZone;
                std::set<FHPos> newZoneIter;
                newZone.insert(tilePos);
                newZoneIter.insert(tilePos);
                while (true) {
                    std::set<FHPos> newFloodTiles;
                    for (const FHPos& prevIterTile : newZoneIter) {
                        fillAdjucent(prevIterTile, tile.m_terType, newZone, newFloodTiles);
                    }
                    if (newFloodTiles.empty())
                        break;

                    newZoneIter = newFloodTiles;
                    newZone.insert(newFloodTiles.cbegin(), newFloodTiles.cend());
                }

                zoned.insert(newZone.cbegin(), newZone.cend());
                FHZone fhZone;
                fhZone.m_tiles = std::vector<FHPos>(newZone.cbegin(), newZone.cend());
                for (auto& pos : fhZone.m_tiles) {
                    auto terVariant = src.m_tiles.get(pos.m_x, pos.m_y, pos.m_z).m_terView;
                    fhZone.m_tilesVariants.push_back(terVariant);
                }
                fhZone.m_terrainId = terrainIds[tile.m_terType];
                dest.m_zones.push_back(std::move(fhZone));
            }
        }
    }
}

}
