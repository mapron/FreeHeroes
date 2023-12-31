/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHPos.hpp"

#include "LibraryObjectDef.hpp"
#include "AdventureSquad.hpp"
#include "Reward.hpp"
#include "MapScore.hpp"

#include <map>

#include "MapUtilExport.hpp"

namespace FreeHeroes {

struct FHCommonObject {
    FHPos                m_pos{ g_invalidPos };
    int                  m_order = 0;
    Core::ObjectDefIndex m_defIndex;
    int64_t              m_guard = 0;
    Core::MapScore       m_score;
    std::string          m_generationId;

    bool operator==(const FHCommonObject&) const noexcept = default;
};

struct FHCommonVisitable : public FHCommonObject {
    Core::LibraryMapVisitableConstPtr m_visitableId = nullptr;
    Core::LibraryObjectDefConstPtr    m_fixedDef    = nullptr;

    bool operator==(const FHCommonVisitable&) const noexcept = default;
};

struct FHPlayerControlledObject : public FHCommonObject {
    Core::LibraryPlayerConstPtr m_player = nullptr;

    bool operator==(const FHPlayerControlledObject&) const noexcept = default;
};

struct FHTownEvent {
    std::string          m_name;
    std::string          m_message;
    Core::ResourceAmount m_resources;

    uint8_t  m_players          = 0;
    bool     m_humanAffected    = true;
    bool     m_computerAffected = false;
    uint16_t m_firstOccurence   = 0;
    uint8_t  m_nextOccurence    = 0;

    std::vector<uint8_t>  m_buildings;
    std::vector<uint16_t> m_creaturesAmounts;

    bool operator==(const FHTownEvent&) const noexcept = default;
};

struct FHTown : public FHPlayerControlledObject {
    bool                         m_isMain{ false };
    Core::LibraryFactionConstPtr m_factionId = nullptr;

    bool        m_hasFort{ false };
    uint32_t    m_questIdentifier = 0;
    bool        m_spellResearch{ true };
    bool        m_hasCustomBuildings{ false };
    bool        m_hasGarison{ false };
    bool        m_hasName          = false;
    bool        m_randomTown       = false;
    bool        m_groupedFormation = false;
    int         m_alignment        = -1;
    std::string m_name;

    std::vector<Core::LibraryBuildingConstPtr> m_buildings;
    std::vector<Core::LibraryBuildingConstPtr> m_forbiddenBuildings;
    std::vector<FHTownEvent>                   m_events;

    std::vector<uint8_t> m_somethingBuildingRelated;

    Core::LibraryObjectDefConstPtr m_randomId = nullptr;

    // @todo: investigate what stored there
    std::vector<uint8_t> m_obligatorySpells;
    std::vector<uint8_t> m_possibleSpells;

    Core::AdventureSquad m_garison;

    Core::UnitByValueList m_garisonRmg;

    bool operator==(const FHTown&) const noexcept = default;
};

MAPUTIL_EXPORT std::ostream& operator<<(std::ostream& stream, const Core::MapScore& score);

MAPUTIL_EXPORT Core::MapScore operator+(const Core::MapScore& l, const Core::MapScore& r);
MAPUTIL_EXPORT Core::MapScore operator-(const Core::MapScore& l, const Core::MapScore& r);

MAPUTIL_EXPORT int64_t maxScoreValue(const Core::MapScore& score);
MAPUTIL_EXPORT int64_t totalScoreValue(const Core::MapScore& score);

}
