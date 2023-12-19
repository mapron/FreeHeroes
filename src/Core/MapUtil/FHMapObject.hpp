/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FHPos.hpp"

#include "LibraryObjectDef.hpp"
#include "AdventureStack.hpp"
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

    bool operator==(const FHCommonVisitable&) const noexcept = default;
};

struct FHPlayerControlledObject : public FHCommonObject {
    Core::LibraryPlayerConstPtr m_player = nullptr;

    bool operator==(const FHPlayerControlledObject&) const noexcept = default;
};

struct FHTown : public FHPlayerControlledObject {
    bool                         m_isMain{ false };
    Core::LibraryFactionConstPtr m_factionId = nullptr;
    bool                         m_hasFort{ false };
    uint32_t                     m_questIdentifier = 0;
    bool                         m_spellResearch{ true };
    bool                         m_hasCustomBuildings{ false };
    bool                         m_hasGarison{ false };

    std::vector<Core::LibraryBuildingConstPtr> m_buildings;

    std::vector<Core::AdventureStack> m_garison;

    Core::UnitByValueList m_garisonRmg;

    bool operator==(const FHTown&) const noexcept = default;
};

MAPUTIL_EXPORT std::ostream& operator<<(std::ostream& stream, const Core::MapScore& score);

MAPUTIL_EXPORT Core::MapScore operator+(const Core::MapScore& l, const Core::MapScore& r);
MAPUTIL_EXPORT Core::MapScore operator-(const Core::MapScore& l, const Core::MapScore& r);

MAPUTIL_EXPORT int64_t maxScoreValue(const Core::MapScore& score);
MAPUTIL_EXPORT int64_t totalScoreValue(const Core::MapScore& score);

}
