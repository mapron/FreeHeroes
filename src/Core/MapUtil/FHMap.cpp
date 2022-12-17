/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "FHMap.hpp"

#include "IRandomGenerator.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

#include "AdventureReflection.hpp"

namespace FreeHeroes {

namespace Core::Reflection {

// clang-format off
template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHPlayerId> = EnumTraits::make(
    FHPlayerId::Invalid,
    "Invalid"  , FHPlayerId::Invalid,
    "None"     , FHPlayerId::None,
    "Red"      , FHPlayerId::Red,
    "Blue"     , FHPlayerId::Blue,
    "Tan"      , FHPlayerId::Tan,
    "Green"    , FHPlayerId::Green,
    "Orange"   , FHPlayerId::Orange,
    "Purple"   , FHPlayerId::Purple,
    "Teal"     , FHPlayerId::Teal,
    "Pink"     , FHPlayerId::Pink
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<Core::GameVersion> = EnumTraits::make(
    Core::GameVersion::Invalid,
    "SOD"  , Core::GameVersion::SOD,
    "HOTA" , Core::GameVersion::HOTA
    );

template<>
inline constexpr const auto EnumTraits::s_valueMapping<FHResource::Type> = EnumTraits::make(
    FHResource::Type::Resource,
    "Resource"      , FHResource::Type::Resource,
    "TreasureChest" , FHResource::Type::TreasureChest
    );
// clang-format on

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHPos>{
    Field("x", &FHPos::m_x),
    Field("y", &FHPos::m_y),
    Field("z", &FHPos::m_z),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHPlayer>{
    Field("ai", &FHPlayer::m_aiPossible),
    Field("human", &FHPlayer::m_humanPossible),
    Field("factions", &FHPlayer::m_startingFactions),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHHeroData>{
    Field("hasExp", &FHHeroData::m_hasExp),
    Field("hasSecSkills", &FHHeroData::m_hasSecSkills),
    Field("hasPrimSkills", &FHHeroData::m_hasPrimSkills),
    Field("hasCustomBio", &FHHeroData::m_hasCustomBio),
    Field("hasSpells", &FHHeroData::m_hasSpells),
    Field("army", &FHHeroData::m_army),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHHero>{
    Field("pos", &FHHero::m_pos),
    Field("order", &FHHero::m_order),
    Field("player", &FHHero::m_player),

    Field("main", &FHHero::m_isMain),
    Field("id", &FHHero::m_id),
    Field("questId", &FHHero::m_questIdentifier),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHTown>{
    Field("pos", &FHTown::m_pos),
    Field("order", &FHTown::m_order),
    Field("player", &FHTown::m_player),

    Field("main", &FHTown::m_isMain),
    Field("factionId", &FHTown::m_factionId),
    Field("hasFort", &FHTown::m_hasFort),
    Field("questId", &FHTown::m_questIdentifier),
    Field("spellResearch", &FHTown::m_spellResearch),
    Field("defFile", &FHTown::m_defFile),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHZone::Rect>{
    Field("pos", &FHZone::Rect::m_pos),
    Field("w", &FHZone::Rect::m_width),
    Field("h", &FHZone::Rect::m_height),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHZone>{
    Field("id", &FHZone::m_id),
    Field("terrainId", &FHZone::m_terrainId),
    Field("tiles", &FHZone::m_tiles),
    Field("tilesVariants", &FHZone::m_tilesVariants),
    Field("rect", &FHZone::m_rect),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHResource>{
    Field("pos", &FHResource::m_pos),
    Field("order", &FHResource::m_order),

    Field("amount", &FHResource::m_amount),
    Field("id", &FHResource::m_id),
    Field("type", &FHResource::m_type),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHArtifact>{
    Field("pos", &FHArtifact::m_pos),
    Field("order", &FHArtifact::m_order),

    Field("id", &FHArtifact::m_id),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMonster>{
    Field("pos", &FHMonster::m_pos),
    Field("order", &FHMonster::m_order),

    Field("id", &FHMonster::m_id),
    Field("count", &FHMonster::m_count),
};
template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHDwelling>{
    Field("pos", &FHDwelling::m_pos),
    Field("order", &FHDwelling::m_order),
    Field("player", &FHDwelling::m_player),

    Field("id", &FHDwelling::m_id),
    Field("variant", &FHDwelling::m_variant),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMap::Objects>{
    Field("resources", &FHMap::Objects::m_resources),
    Field("artifacts", &FHMap::Objects::m_artifacts),
    Field("monsters", &FHMap::Objects::m_monsters),
    Field("dwellings", &FHMap::Objects::m_dwellings),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHTileMap>{
    Field("width", &FHTileMap::m_width),
    Field("height", &FHTileMap::m_height),
    Field("depth", &FHTileMap::m_depth),
};

template<>
inline constexpr const std::tuple MetaInfo::s_fields<FHMap>{
    Field("version", &FHMap::m_version),
    Field("seed", &FHMap::m_seed),
    Field("tileMap", &FHMap::m_tileMap),
    Field("name", &FHMap::m_name),
    Field("descr", &FHMap::m_descr),
    Field("difficulty", &FHMap::m_difficulty),
    Field("players", &FHMap::m_players),
    Field("wanderingHeroes", &FHMap::m_wanderingHeroes),
    Field("towns", &FHMap::m_towns),
    Field("zones", &FHMap::m_zones),
    Field("objects", &FHMap::m_objects),
    Field("defaultTerrain", &FHMap::m_defaultTerrain),
    Field("disabledHeroes", &FHMap::m_disabledHeroes),
    Field("disabledArtifacts", &FHMap::m_disabledArtifacts),
    Field("disabledSpells", &FHMap::m_disabledSpells),
    Field("disabledSkills", &FHMap::m_disabledSkills),
    Field("customHeroes", &FHMap::m_customHeroes),
};

template<>
inline constexpr const bool MetaInfo::s_isStringMap<FHMap::PlayersMap>{ true };

}

void FHMap::toJson(PropertyTree& data) const
{
    Core::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void FHMap::fromJson(const PropertyTree& data, const Core::IGameDatabase* database)
{
    Core::Reflection::PropertyTreeReader reader(database);
    *this = {};
    reader.jsonToValue(data, *this);
}

void FHZone::placeOnMap(FHTileMap& map) const
{
    if (m_rect.has_value()) {
        auto& rect = m_rect.value();
        for (uint32_t x = 0; x < rect.m_width; ++x) {
            for (uint32_t y = 0; y < rect.m_height; ++y) {
                map.get(FHPos{ .m_x = x + rect.m_pos.m_x,
                               .m_y = y + rect.m_pos.m_y,
                               .m_z = rect.m_pos.m_z })
                    .m_terrain
                    = m_terrainId;
            }
        }
        return;
    }
    if (!m_tiles.empty() && m_tilesVariants.size() == m_tiles.size()) {
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            auto& tile     = map.get(m_tiles[i]);
            tile.m_terrain = m_terrainId;
            tile.m_view    = m_tilesVariants[i];
        }
        return;
    }
    if (!m_tiles.empty()) {
        for (auto& pos : m_tiles) {
            map.get(pos).m_terrain = m_terrainId;
        }
    }
}

void FHTileMap::correctTerrainTypes(Core::LibraryTerrainConstPtr dirtTerrain,
                                    Core::LibraryTerrainConstPtr sandTerrain,
                                    Core::LibraryTerrainConstPtr waterTerrain)
{
    // true = pattern found
    auto correctTile = [this, sandTerrain, waterTerrain](const FHPos& pos, bool flipHor, bool flipVert, int order) -> bool {
        /* TL  T  TR
         *  L  X   R
         * BL  B  BR
         */
        const auto& TL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +1 : -1);
        const auto& T  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? +1 : -1);
        const auto& TR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +1 : -1);
        const auto& L  = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? +0 : +0);
        auto&       X  = get(pos);
        const auto& R  = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? +0 : +0);
        const auto& BL = getNeighbour(pos, flipHor ? +1 : -1, flipVert ? -1 : +1);
        const auto& B  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -1 : +1);
        const auto& BR = getNeighbour(pos, flipHor ? -1 : +1, flipVert ? -1 : +1);

        const bool waterX = X.m_terrain == waterTerrain;
        const auto dR     = X.m_terrain != R.m_terrain;
        const auto dL     = X.m_terrain != L.m_terrain;
        const auto dT     = X.m_terrain != T.m_terrain;
        const auto dB     = X.m_terrain != B.m_terrain;

        //const auto dTL = X.m_terrain != TL.m_terrain;
        const auto dTR = X.m_terrain != TR.m_terrain;
        const auto dBL = X.m_terrain != BL.m_terrain;
        const auto dBR = X.m_terrain != BR.m_terrain;

        const auto sandR  = R.m_terrain == sandTerrain || (!waterX && R.m_terrain == waterTerrain);
        const auto sandL  = L.m_terrain == sandTerrain || (!waterX && L.m_terrain == waterTerrain);
        const auto sandT  = T.m_terrain == sandTerrain || (!waterX && T.m_terrain == waterTerrain);
        const auto sandB  = B.m_terrain == sandTerrain || (!waterX && B.m_terrain == waterTerrain);
        const auto sandBR = BR.m_terrain == sandTerrain || (!waterX && BR.m_terrain == waterTerrain);
        const auto sandBL = BL.m_terrain == sandTerrain || (!waterX && BL.m_terrain == waterTerrain);
        const auto sandTR = TR.m_terrain == sandTerrain || (!waterX && TR.m_terrain == waterTerrain);

        if (!waterX) {
            X.m_coastal = false
                          || R.m_terrain == waterTerrain
                          || L.m_terrain == waterTerrain
                          || T.m_terrain == waterTerrain
                          || B.m_terrain == waterTerrain
                          || BR.m_terrain == waterTerrain
                          || TR.m_terrain == waterTerrain
                          || TL.m_terrain == waterTerrain
                          || BL.m_terrain == waterTerrain;
        }
        using BT = Core::LibraryTerrain::BorderType;

        auto setView = [&X, flipHor, flipVert, waterTerrain](BT borderType, bool dirt, bool sand) {
            if (dirt && !sand && X.m_terrain == waterTerrain) {
                dirt = false;
                sand = true;
            }
            X.calculateOffsets(borderType, dirt, sand);
            X.m_viewMin  = static_cast<uint8_t>(X.m_tileOffset);
            X.m_viewMax  = static_cast<uint8_t>(X.m_viewMin + X.m_tileCount - 1);
            X.m_flipHor  = flipHor;
            X.m_flipVert = flipVert;
        };

        if (false) {
        } else if (order == 0 && !waterX) {
            if (false) {
                /// @todo:
                setView(BT::ThreeWay_DD, true, true);
                setView(BT::ThreeWay_DS, true, true);
                setView(BT::ThreeWay_SS, true, true);
            } else if (sandBL && dR && !sandR && !sandB) {
                setView(BT::ThreeWay_RD_BLS, true, true);
            } else if (sandTR && dB && !sandB && !sandR) {
                setView(BT::ThreeWay_BD_TRS, true, true); // !<
            } else if (sandR && dB && !sandB) {
                setView(BT::ThreeWay_RS_BD, true, true);
            } else if (sandB && dR && !sandR) {
                setView(BT::ThreeWay_BS_RD, true, true);
            } else if (sandBR && dR && !dT && !sandR) {
                setView(BT::ThreeWay_TRD_BRS, true, true);
            } else if (sandBR && dB && !dR && !sandB) {
                setView(BT::ThreeWay_BRS_BLD, true, true);
            } else {
                return false;
            }
        } else if (order == 1 && dL && dT) {
            setView(BT::TL, !sandL, sandL);
            if (!dTR || !dBL) {
                setView(BT::TLS, !sandL, sandL);
            }
        } else if (order == 2 && dL) {
            setView(BT::L, !sandL, sandL);
        } else if (order == 2 && dT) {
            setView(BT::T, !sandT, sandT);
        } else if (order == 3 && dBR) {
            setView(BT::BR, !sandBR, sandBR);

            const auto& B2  = getNeighbour(pos, flipHor ? +0 : +0, flipVert ? -2 : +2);
            const auto& R2  = getNeighbour(pos, flipHor ? -2 : +2, flipVert ? +0 : +0);
            const auto  dB2 = X.m_terrain != B2.m_terrain;
            const auto  dR2 = X.m_terrain != R2.m_terrain;
            if (dB2 || dR2) {
                setView(BT::BRS, !sandBR, sandBR);
            }
        } else {
            return false;
        }
        return true;
    };

    for (uint8_t z = 0; z < m_depth; ++z) {
        for (uint32_t y = 0; y < m_height; ++y) {
            for (uint32_t x = 0; x < m_width; ++x) {
                const FHPos pos{ x, y, z };
                auto&       X = get(pos);
                if (X.m_terrain == sandTerrain || X.m_terrain == dirtTerrain)
                    continue;

                const bool tileCorrected = [&correctTile, &pos]() {
                    for (int order = 0; order <= 3; ++order) {
                        for (int flipHor = 0; flipHor <= 1; ++flipHor) {
                            for (int flipVert = 0; flipVert <= 1; ++flipVert) {
                                if (correctTile(pos, flipHor, flipVert, order))
                                    return true;
                            }
                        }
                    }
                    return false;
                }();
                if (tileCorrected)
                    continue;

                const uint8_t offset     = X.m_terrain->presentationParams.centerTilesOffset;
                const auto    centerSize = X.m_terrain->presentationParams.centerTilesCount;
                X.m_viewMin              = offset;
                X.m_viewMax              = offset + (centerSize <= 0 ? 0 : centerSize - 1);
            }
        }
    }
}

void FHTileMap::rngTiles(Core::IRandomGenerator* rng)
{
    auto rngView = [&rng](uint8_t min, uint8_t max) -> uint8_t {
        if (min == max)
            return min;
        uint8_t diff   = max - min;
        uint8_t result = rng->genSmall(diff);
        if (result >= 20)
            result = rng->genSmall(diff);
        return min + result;
    };

    for (uint8_t z = 0; z < m_depth; ++z) {
        for (uint32_t y = 0; y < m_height; ++y) {
            for (uint32_t x = 0; x < m_width; ++x) {
                auto& X = get(x, y, z);
                //if (X.m_view != 0xff)
                //    continue;
                // todo: alternative. what's better?
                if (X.m_view >= X.m_viewMin && X.m_view <= X.m_viewMax)
                    continue;

                X.m_view = rngView(X.m_viewMin, X.m_viewMax);
            }
        }
    }
}

void FHTileMap::Tile::calculateOffsets(Core::LibraryTerrain::BorderType borderType, bool dirtBorder, bool sandBorder)
{
    auto& pp = m_terrain->presentationParams;
    if (dirtBorder && sandBorder) {
        if (pp.sandDirtBorderTilesOffset < 0)
            return calculateOffsets(Core::LibraryTerrain::BorderType::Center, false, false);

        m_tileOffset = pp.sandDirtBorderTilesOffset + pp.borderThreeWayOffsets.at(borderType);
        m_tileCount  = pp.borderThreeWayCounts.at(borderType);
    } else if (dirtBorder) {
        if (pp.dirtBorderTilesOffset < 0)
            return calculateOffsets(Core::LibraryTerrain::BorderType::Center, false, false);

        m_tileOffset = pp.dirtBorderTilesOffset + pp.borderOffsets.at(borderType);
        m_tileCount  = pp.borderCounts.at(borderType);
    } else if (sandBorder) {
        if (pp.sandBorderTilesOffset < 0)
            return calculateOffsets(Core::LibraryTerrain::BorderType::Center, false, false);

        m_tileOffset = pp.sandBorderTilesOffset + pp.borderOffsets.at(borderType);
        m_tileCount  = pp.borderCounts.at(borderType);
    } else {
        m_tileOffset = pp.centerTilesOffset;
        m_tileCount  = pp.centerTilesCount;
    }
}

}
